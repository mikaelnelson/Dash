
/*********************
 *      INCLUDES
 *********************/
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include <pubsub.h>

#include "esp_log.h"

#include "speedometer_gauge.h"
#include "stepper_gauge.h"

/*********************
 *      DEFINES
 *********************/
#define TAG "SPEEDO"

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *     GLOBALS
 **********************/

static bool             g_stepper_init_finished;

/**********************
 *     CONSTANTS
 **********************/

/**********************
 *    PROTOTYPES
 **********************/
_Noreturn static void msg_task( void * params );

void speedometer_gauge_start( void )
{
    ESP_LOGI(TAG, "Speedo Started");

    g_stepper_init_finished = false;

    xTaskCreatePinnedToCore(msg_task, "speedometer_gauge_msg_task", 4096*2, NULL, 0, NULL, 1);

    // Start Stepper Init By Going to Gauge Max
    stepper_gauge_set_degree( STEPPER_DEGREE_MAX );
}

void speedometer_gauge_stop( void )
{
}

_Noreturn static void msg_task( void * params )
{
    ps_subscriber_t *s = ps_new_subscriber(10, STRLIST( "stepper.started",  "stepper.finished" ));

    ps_msg_t *msg = NULL;

    while(true) {
        msg = ps_get( s, 5000 );
        if( msg != NULL) {
            if( 0 == strcmp( "stepper.started", msg->topic ) ) {
                ESP_LOGI(TAG, "Speedo Stepper Started");
            }
            else if( 0 == strcmp( "stepper.finished", msg->topic ) ) {
                ESP_LOGI(TAG, "Speedo Stepper Finished");

                if( false == g_stepper_init_finished ) {
                    g_stepper_init_finished = true;

                    // Return to Gauge Min
                    stepper_gauge_set_degree( STEPPER_DEGREE_MIN );
                }
            }
        }
    }
}
