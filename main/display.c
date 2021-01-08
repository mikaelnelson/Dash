/*********************
 *      INCLUDES
 *********************/
#include <string.h>
#include <time.h>
#include <math.h>

#include <pubsub.h>
#include <lvgl/lvgl.h>
#include <lvgl_helpers.h>

#include "esp_system.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"

#include "display.h"

/*********************
 *      DEFINES
 *********************/
#define TAG                 "DISPLAY"
#define LV_TICK_PERIOD_MS   10

#define SCREEN_WDTH         CONFIG_LVGL_DISPLAY_WIDTH       // 128
#define SCREEN_HGHT         CONFIG_LVGL_DISPLAY_HEIGHT      // 32

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *     GLOBALS
 **********************/
static SemaphoreHandle_t    g_display_lock;
static uint8_t              g_display_buf1[CONFIG_LVGL_DISPLAY_WIDTH*CONFIG_LVGL_DISPLAY_HEIGHT/8];
static lv_disp_buf_t        g_display_disp_buf;
static lv_disp_drv_t        g_display_disp_drv;
static uint32_t             g_display_size_in_px;

static esp_timer_handle_t   g_display_periodic_timer;

/*
 * Pages:
 *  Default
 *      - Time
 *      - Range
 *  Odometer
 *  Time
 *  Power
 *      - Voltage
 *      - Current
 *      - Range
 */
static lv_obj_t           * g_time_label;
static lv_obj_t           * g_range_label;
static lv_obj_t           * g_odometer_label;
static lv_obj_t           * g_voltage_label;
static lv_obj_t           * g_current_label;

/**********************
 *     CONSTANTS
 **********************/

/**********************
 *    PROTOTYPES
 **********************/
static void display_tick_timer( void * params );
_Noreturn static void display_task( void * params );
_Noreturn static void display_msg_task( void * params );


void log_callback(lv_log_level_t level, const char * file, uint32_t line, const char * description, const char * message)
{
    ESP_LOGI(TAG, "%s %d: %s %s", file, line, description, message);
}


void display_start( void )
{
    g_display_size_in_px = CONFIG_LVGL_DISPLAY_WIDTH*CONFIG_LVGL_DISPLAY_HEIGHT;
    g_display_lock = xSemaphoreCreateMutex();

    // Initialize LVGL
    lv_init();

    lv_log_register_print_cb(log_callback);

    // Initialize Interface Driver
    lvgl_driver_init();

    // Initialize Display Buffer
    lv_disp_buf_init(&g_display_disp_buf, g_display_buf1, NULL, g_display_size_in_px);

    // Initialize Display Driver
    lv_disp_drv_init(&g_display_disp_drv);
    g_display_disp_drv.flush_cb = disp_driver_flush;
    g_display_disp_drv.set_px_cb = disp_driver_set_px;
    g_display_disp_drv.rounder_cb = disp_driver_rounder;

    g_display_disp_drv.buffer = &g_display_disp_buf;
    lv_disp_drv_register(&g_display_disp_drv);


    // Create Time
    g_time_label = lv_label_create( lv_scr_act(), NULL );
    lv_label_set_align( g_time_label, LV_LABEL_ALIGN_CENTER );
    lv_obj_align(g_time_label, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);
    lv_obj_set_auto_realign( g_time_label, true);
    lv_label_set_text( g_time_label, "TIME" );

    // Create Voltage Label
    g_voltage_label = lv_label_create( lv_scr_act(), NULL );
    lv_label_set_align( g_voltage_label, LV_LABEL_ALIGN_CENTER );
    lv_obj_align(g_voltage_label, NULL, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_auto_realign( g_voltage_label, true);
    lv_label_set_text( g_voltage_label, "44 V" );

    // Create Current Label
    g_current_label = lv_label_create( lv_scr_act(), NULL );
    lv_label_set_align( g_current_label, LV_LABEL_ALIGN_CENTER );
    lv_obj_align(g_current_label, NULL, LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_auto_realign( g_current_label, true);
    lv_label_set_text( g_current_label, "0 A" );

    // Setup GUI Tick Timer
    const esp_timer_create_args_t lvgl_tick_timer_args =
            {
            .callback = &display_tick_timer,
            .name = "display_tick_timer"
            };
    esp_timer_create(&lvgl_tick_timer_args, &g_display_periodic_timer);
    esp_timer_start_periodic(g_display_periodic_timer, LV_TICK_PERIOD_MS * 1000);

    //If you want to use a task to create the graphic, you NEED to create a Pinned task
    //Otherwise there can be problem such as memory corruption and so on
    xTaskCreatePinnedToCore(display_task, "display_task", 4096*2, NULL, 0, NULL, 1);
    xTaskCreatePinnedToCore(display_msg_task, "display_msg_task", 4096*2, NULL, 0, NULL, 1);


}

void display_stop( void )
{
}

static void display_tick_timer( void * params )
{
    (void) params;
    lv_tick_inc(LV_TICK_PERIOD_MS);
}

_Noreturn static void display_task( void * params )
{
    while (1) {
        vTaskDelay(1);

        //Try to lock the semaphore, if success, call lvgl stuff
        if (xSemaphoreTake(g_display_lock, (TickType_t)10) == pdTRUE) {
            lv_task_handler();
            xSemaphoreGive(g_display_lock);
        }
    }

    //A task should NEVER return
    vTaskDelete(NULL);
}

_Noreturn static void display_msg_task( void * params )
{
    ps_subscriber_t *s = ps_new_subscriber(10, STRLIST( "gps.time",  "gps.speed" ));

    ps_msg_t *msg = NULL;

    while(true) {
        msg = ps_get(s, 5000);
        if (msg != NULL) {
            if( 0 == strcmp("gps.time", msg->topic ) ) {
                if (xSemaphoreTake(g_display_lock, (TickType_t)10) == pdTRUE) {
                    struct tm ts;
                    char time_buf[80];
                    ts = *localtime((time_t *) &msg->int_val );
                    strftime( time_buf, sizeof( time_buf ), "%I:%M:%S %p", &ts );

                    lv_label_set_text_fmt( g_time_label, "%s", time_buf );

                    xSemaphoreGive(g_display_lock);
                }
            }

            ps_unref_msg(msg);
        }
    }
    ps_free_subscriber(s);
}
