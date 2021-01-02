/*
 * Includes
 */
#include <string.h>

#include <minmea/minmea.h>
#include <pubsub.h>

#include "esp_system.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "driver/uart.h"

#include "gps.h"


/*
 * Defines
 */
#define TAG                 "GPS"

#define RX_BUF_SZ           (256)
#define GPS_SENTENCE_MAX_SZ (128)

/*
 * Types
 */
typedef struct
    {
    // UART Data
    uart_port_t             uart_port;
    int                     tx_pin;
    int                     rx_pin;
    int                     cts_pin;
    int                     rts_pin;

    // Task Data
    TaskHandle_t            gps_read_task;
    TaskHandle_t            gps_parse_task;

    // GPS Data
    char                    gps_sentence[GPS_SENTENCE_MAX_SZ + 1];
    SemaphoreHandle_t       gps_sentence_lock;
    SemaphoreHandle_t       gps_sentence_trigger;
    } gps_intf_priv_t;

/*
 * Globals
 */
static gps_intf_priv_t      g_priv; //This should only be temporary, eventually allocate this

/*
 * Prototypes
 */
_Noreturn static void gps_read_task( void * params );
_Noreturn static void gps_parse_task( void * params );

void gps_start( void )
{

    /* Configure parameters of an UART driver,
    * communication pins and install the driver */
    uart_config_t uart_config = {
            .baud_rate = 9600,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_APB,
    };

    //Initialize Globals
    g_priv.uart_port            = UART_NUM_2;
    g_priv.tx_pin               = 2;
    g_priv.rx_pin               = 15;
    g_priv.cts_pin              = UART_PIN_NO_CHANGE;
    g_priv.rts_pin              = UART_PIN_NO_CHANGE;

    g_priv.gps_sentence_lock    = xSemaphoreCreateMutex();
    g_priv.gps_sentence_trigger = xSemaphoreCreateBinary();

    //Install UART driver, and get the queue.
    uart_driver_install( g_priv.uart_port, 2 * RX_BUF_SZ, 0, 0, NULL, 0);
    uart_param_config(g_priv.uart_port, &uart_config);

    //Set UART log level
    esp_log_level_set(TAG, ESP_LOG_INFO);

    //Set UART pins (using UART0 default pins ie no changes.)
    uart_set_pin(g_priv.uart_port, g_priv.tx_pin, g_priv.rx_pin, g_priv.rts_pin, g_priv.cts_pin);

    //Create a task to handler UART event from ISR
    xTaskCreate( gps_read_task, "gps_read_task", 2048, &g_priv, 12, NULL );

    xTaskCreate( gps_parse_task, "gps_parse_task", 2048, &g_priv, 12, NULL );
}

void gps_stop( void )
{
}

//We should change this back into using a queue, and that queued data goes into a stream buffer
// We then have a task that wakes on stream buffer and processes the GPS data.

_Noreturn static void gps_read_task( void *params )
{
    gps_intf_priv_t       * priv = (gps_intf_priv_t *)params;
    char                    gps_sentence[GPS_SENTENCE_MAX_SZ + 1];
    char                    ch;

    for(;;)
    {
        // Local Variables
        int             gps_sentence_idx;
        bool            trigger;

        // Initialize
        trigger         = false;
        gps_sentence_idx= 0;

        // If Sentence Buffer Index Is At 0, Read Byte By Byte Until You Find A '$'
        while( 1 == uart_read_bytes( priv->uart_port, (uint8_t *) &ch, 1, 200 / portTICK_RATE_MS ))
        {
            if( '$' == gps_sentence[0] )
            {
                gps_sentence[gps_sentence_idx] = ch;
                gps_sentence_idx++;
                break;
            }
        }

        // Continue Reading Data To GPS Sentence Until '<CR> <LF>'
        while( 1 == uart_read_bytes( priv->uart_port, (uint8_t *) &ch, 1, portMAX_DELAY ))
        {
            // Add Data To Sentence
            if( gps_sentence_idx < GPS_SENTENCE_MAX_SZ)
            {
                gps_sentence[gps_sentence_idx] = ch;
                gps_sentence_idx++;
            }

            // Detect Line Feed
            if( '\n' == ch )
            {
                // Null Terminate GPS Sentence
                gps_sentence[gps_sentence_idx] = '\0';

                trigger = true;
                break;
            }
        }

        // Trigger
        if( trigger )
        {
            // Lock Mutex
            xSemaphoreTake( priv->gps_sentence_lock, portMAX_DELAY );

            // Copy Data
            strncpy( priv->gps_sentence, (const char *)gps_sentence, sizeof(priv->gps_sentence) );

            // Unlock Mutex
            xSemaphoreGive( priv->gps_sentence_lock );

            // Trigger Processing
            xSemaphoreGive( priv->gps_sentence_trigger );
        }
    }

    vTaskDelete(NULL);
}

_Noreturn static void gps_parse_task( void * params )
{
    gps_intf_priv_t       * priv = (gps_intf_priv_t *)params;
    char                    gps_sentence[GPS_SENTENCE_MAX_SZ + 1];
    enum minmea_sentence_id id;

    for(;;)
    {
        // Wait For Data
        xSemaphoreTake( priv->gps_sentence_trigger, portMAX_DELAY );

        // Get Data
        xSemaphoreTake( priv->gps_sentence_lock, portMAX_DELAY );
        memcpy( gps_sentence, priv->gps_sentence, sizeof(gps_sentence) );
        xSemaphoreGive( priv->gps_sentence_lock );

        id = minmea_sentence_id((const char *)gps_sentence, false);
        switch( id )
            {
                case MINMEA_SENTENCE_RMC:
                    {
                        struct minmea_sentence_rmc frame;

                        if( minmea_parse_rmc( &frame, (const char *)gps_sentence ) )
                        {
                            struct timespec ts;

                            // Update Time
                            if( 0 == minmea_gettime(&ts, &frame.date, &frame.time) )
                            {
                                ESP_LOGI(TAG, "Time: %ld", ts.tv_sec);
                                PUB_INT("gps.time", ts.tv_sec);
                            }
                        }
                    }
                    break;

                case MINMEA_SENTENCE_VTG:
                    {
                        struct minmea_sentence_vtg frame;

                        if( minmea_parse_vtg( &frame, (const char *)gps_sentence ) )
                        {
                            ESP_LOGI(TAG, "Speed (KPH): %f", minmea_tofloat( &frame.speed_kph ));
                            PUB_DBL("gps.speed", minmea_tofloat( &frame.speed_kph ));
                        }
                    }
                    break;

                default:
                    break;
            }
    }

    vTaskDelete(NULL);
}