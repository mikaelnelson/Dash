/*********************
 *      INCLUDES
 *********************/
#include <string.h>
#include <time.h>
#include <math.h>

#include "esp_system.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"

#include <lvgl/lvgl.h>
#include <lvgl_helpers.h>

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
static TimerHandle_t        g_update_timer;

static SemaphoreHandle_t    g_display_lock;
//static lv_color_t           g_display_buf1[DISP_BUF_SIZE];
//static lv_color_t           g_display_buf2[DISP_BUF_SIZE];
static uint8_t              g_display_buf1[CONFIG_LVGL_DISPLAY_WIDTH*CONFIG_LVGL_DISPLAY_HEIGHT/8];
static uint8_t              g_display_buf2[CONFIG_LVGL_DISPLAY_WIDTH*CONFIG_LVGL_DISPLAY_HEIGHT/8];
static lv_disp_buf_t        g_display_disp_buf;
static lv_disp_drv_t        g_display_disp_drv;
static uint32_t             g_display_size_in_px;

static esp_timer_handle_t   g_display_periodic_timer;

static lv_obj_t           * g_test_label;


/**********************
 *     CONSTANTS
 **********************/

/**********************
 *    PROTOTYPES
 **********************/
static void display_update_timer( TimerHandle_t pxTimer );
static void display_tick_timer( void * params );
_Noreturn static void display_task( void * params );

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

    // Setup Test Label
    g_test_label = lv_label_create( lv_scr_act(), NULL );
    lv_obj_set_size( g_test_label, 32, 32 );
    lv_obj_set_pos( g_test_label, 0, 0 );
    lv_label_set_text( g_test_label, "####" );

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

//    g_update_timer = xTimerCreate
//        (
//        "update_timer",
//        (1000 / portTICK_PERIOD_MS),
//        pdTRUE,
//        0,
//        display_update_timer
//        );
//
//    xTimerStart( g_update_timer, 0 );
}

void display_stop( void )
{
    xTimerStop( g_update_timer, 0 );
}

static void display_update_timer( TimerHandle_t pxTimer )
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