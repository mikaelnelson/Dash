/*
 * Based on SwitecX25 Library
 *  github.com/clearwater/SwitecX25
 */

/*********************
 *      INCLUDES
 *********************/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include <pubsub.h>

#include "esp_log.h"

#include "driver/gpio.h"

#include "stepper_gauge.h"

/*********************
 *      DEFINES
 *********************/
#define TAG "STEPPER"

#define DEGREE_CNT              STEPPER_DEGREE_MAX
#define STEPS_PER_DEGREE_CNT    3
#define MICROSTEP_PER_STEP_CNT  4

#define STEP_CNT_MIN            0
#define STEP_CNT_MAX            ( DEGREE_CNT * STEPS_PER_DEGREE_CNT * MICROSTEP_PER_STEP_CNT )

#define PIN_STEP                GPIO_NUM_18
#define PIN_DIR                 GPIO_NUM_19

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *     GLOBALS
 **********************/
static esp_timer_handle_t   g_update_timer;

static uint32_t             g_current_step;
static uint32_t             g_target_step;
static uint32_t             g_vel;
static int8_t               g_dir;

#define VELOCITY_MAX        300
static uint16_t defaultAccelTable[][2] = {
        {   20,             800 / MICROSTEP_PER_STEP_CNT},
        {   50,             400 / MICROSTEP_PER_STEP_CNT},
        {  100,             200 / MICROSTEP_PER_STEP_CNT},
        {  150,             150 / MICROSTEP_PER_STEP_CNT},
        {  VELOCITY_MAX,    90 / MICROSTEP_PER_STEP_CNT}
};
#define DEFAULT_ACCEL_TABLE_SIZE (sizeof(defaultAccelTable)/sizeof(*defaultAccelTable))

/**********************
 *     CONSTANTS
 **********************/

/**********************
 *    PROTOTYPES
 **********************/
static void stepper_gauge_update_timer( void * params );
static void stepper_step( int dir );
static void stepper_zero( void );
static void stepper_advance( void );
static void stepper_set_position( uint32_t position );

void stepper_gauge_start( void )
{
    gpio_config_t config;

    // Initialize Variables
    g_current_step      = 0;
    g_target_step       = 0;
    g_vel               = 0;
    g_dir               = 0;

    // Configure GPIOs
    config.intr_type    = GPIO_INTR_DISABLE;
    config.mode         = GPIO_MODE_OUTPUT;
    config.pin_bit_mask = ( ( 1ULL << PIN_STEP ) |
                            ( 1ULL << PIN_DIR ) );
    config.pull_down_en = 0;
    config.pull_up_en   = 0;

    gpio_config(&config);

    // Setup Stepper Tick Timer
    const esp_timer_create_args_t stepper_tick_timer_args =
            {
            .callback = &stepper_gauge_update_timer,
            .name = "stepper_gauge_tick_timer"
            };
    esp_timer_create(&stepper_tick_timer_args, &g_update_timer);
}

void stepper_gauge_stop( void )
{
}

void stepper_gauge_set_degree( float degree )
{
    // Convert Degrees to Step Position
    int position = ( degree * STEPS_PER_DEGREE_CNT * MICROSTEP_PER_STEP_CNT );

    // Set Position
    stepper_set_position( position );
}


static void stepper_gauge_update_timer( void * params )
{
    (void) params;
    stepper_advance();
}

static void stepper_step( int dir )
{
    if( ( g_current_step == STEP_CNT_MIN ) && ( -1 == dir ) ) {
        return;
    }

    if( ( g_current_step >= STEP_CNT_MAX ) && ( 1 == dir ) ) {
        return;
    }

    gpio_set_level(PIN_DIR, dir > 0 ? 0 : 1);
    gpio_set_level(PIN_STEP, 1);
    vTaskDelay(pdMS_TO_TICKS(1));
    gpio_set_level(PIN_STEP, 0);
    g_current_step += dir;

    ESP_LOGI(TAG, "%u", g_current_step);

}

static void stepper_zero( void )
{
    g_current_step = STEP_CNT_MAX - 1;
    for (unsigned int i=0;i<STEP_CNT_MAX;i++) {
        stepper_step(-1);
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    g_current_step = 0;
    g_target_step = 0;
    g_vel = 0;
    g_dir = 0;
}

static void stepper_advance( void )
{
    // detect stopped state
    if( g_current_step == g_target_step && g_vel == 0) {
        // Announce Finished
        PUB_NIL("stepper.finished");

        g_dir = 0;
        return;
    }

    // if stopped, determine direction
    if( g_vel == 0 ) {
        g_dir = g_current_step < g_target_step ? 1 : -1;
        // do not set to 0 or it could go negative in case 2 below
        g_vel = 1;
    }

    stepper_step( g_dir );

    // determine delta, number of steps in current direction to target.
    // may be negative if we are headed away from target
    int delta = g_dir > 0 ? g_target_step - g_current_step : g_current_step - g_target_step;

    if( delta > 0 ) {
        // case 1 : moving towards target (maybe under accel or decel)
        if ( delta < g_vel ) {
            // time to declerate
            g_vel--;
        } else if( g_vel < VELOCITY_MAX ) {
            // accelerating
            g_vel++;
        } else {
            // at full speed - stay there
        }
    } else {
        // case 2 : at or moving away from target (slow down!)
        g_vel--;
    }

    // vel now defines delay
    unsigned char i = 0;

    // this is why vel must not be greater than the last vel in the table.
    while( (i < DEFAULT_ACCEL_TABLE_SIZE) && (defaultAccelTable[i][0] < g_vel) ) {
        i++;
    }

    // Schedule Next Timer
    esp_timer_start_once( g_update_timer, defaultAccelTable[i][1]);
}

static void stepper_set_position( uint32_t position )
{
    // pos is unsigned so don't need to check for <0
    if( position >= STEP_CNT_MAX ) {
        position = STEP_CNT_MAX - 1;
    }

    // Set Target
    g_target_step = position;

    // Announce Started
    PUB_NIL("stepper.started");

    // Start Advancing Stepper
    esp_timer_start_once( g_update_timer, 0);
}
