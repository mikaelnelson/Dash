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

#include "driver/gpio.h"

#include "stepper_gauge.h"

/*********************
 *      DEFINES
 *********************/
#define RESET_STEP_MICROSEC 800

#define STEP_CNT_MIN        0
#define STEP_CNT_MAX        ( 315 * 3 )

#define PIN_CNT             4

#define PIN_1               GPIO_NUM_18
#define PIN_2               GPIO_NUM_19
#define PIN_3               GPIO_NUM_21
#define PIN_4               GPIO_NUM_23

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
static uint32_t             g_current_step;
static uint8_t              g_current_state;
static uint32_t             g_target_step;
static uint32_t             g_vel;
static uint8_t              g_dir;
static bool                 g_stopped;
static uint32_t             g_time0;
static uint32_t             g_micro_delay;

#define VELOCITY_MAX        300
static uint16_t defaultAccelTable[][2] = {
        {   20,             3000},
        {   50,             1500},
        {  100,             1000},
        {  150,             800},
        {  VELOCITY_MAX,    600}
};
#define DEFAULT_ACCEL_TABLE_SIZE (sizeof(defaultAccelTable)/sizeof(*defaultAccelTable))

static uint8_t stateMap[] = {
        0x9, 0x1, 0x7, 0x6, 0xE, 0x8
};
#define STATE_CNT (sizeof(stateMap)/sizeof(*stateMap))


/**********************
 *     CONSTANTS
 **********************/
static const uint8_t    gc_pin_map[PIN_CNT] = { PIN_1, PIN_2, PIN_3, PIN_4 };

/**********************
 *    PROTOTYPES
 **********************/
static void stepper_gauge_update_timer( TimerHandle_t pxTimer );
static void stepper_write( void );
static void stepper_up( void );
static void stepper_down( void );
static void stepper_zero( void );
static void stepper_advance( void );
static void stepper_set_position( uint32_t position );
static void stepper_update( void );

void stepper_gauge_start( void )
{
    gpio_config_t config;

    // Initialize Variables
    g_current_step      = 0;
    g_current_state     = 0;
    g_target_step       = 0;
    g_vel               = 0;
    g_dir               = 0;
    g_stopped           = true;
    g_time0             = 0;
    g_micro_delay       = 0;

    // Configure GPIOs
    config.intr_type    = GPIO_INTR_DISABLE;
    config.mode         = GPIO_MODE_OUTPUT;
    config.pin_bit_mask = ( ( 1ULL << PIN_1 ) |
                            ( 1ULL << PIN_2 ) |
                            ( 1ULL << PIN_3 ) |
                            ( 1ULL << PIN_4 ) );
    config.pull_down_en = 0;
    config.pull_up_en   = 0;

    gpio_config(&config);

    g_update_timer = xTimerCreate
        (
            "stepper_gauge_update_timer",
            (100 / portTICK_PERIOD_MS),
            pdTRUE,
            0,
            stepper_gauge_update_timer
        );

    xTimerStart( g_update_timer, 0 );
}

void stepper_gauge_stop( void )
{
    xTimerStop( g_update_timer, 0 );
}

static void stepper_gauge_update_timer( TimerHandle_t pxTimer )
{
    stepper_update();
}

static void stepper_write( void )
{
    uint8_t mask = stateMap[g_current_state];

    for( int i = 0; i < PIN_CNT; i++ ) {
        gpio_set_level(gc_pin_map[i], mask & 0x1);
        mask >>= 1;
    }
}

static void stepper_up( void )
{
    if (g_current_step < STEP_CNT_MAX) {
        g_current_step++;
        g_current_state = (g_current_state + 1) % STATE_CNT;
        stepper_write();
    }
}

static void stepper_down( void )
{
    if (g_current_step > STEP_CNT_MIN) {
        g_current_step--;
        g_current_state = (g_current_state + 5) % STATE_CNT;
        stepper_write();
    }
}

static void stepper_zero( void )
{
    g_current_step = STEP_CNT_MAX - 1;
    for (unsigned int i=0;i<STEP_CNT_MAX;i++) {
        stepper_down();
        vTaskDelay(RESET_STEP_MICROSEC);
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
        g_stopped = true;
        g_dir = 0;
        g_time0 = xTaskGetTickCount();
        return;
    }

    // if stopped, determine direction
    if( g_vel == 0 ) {
        g_dir = g_current_step < g_target_step ? 1 : -1;
        // do not set to 0 or it could go negative in case 2 below
        g_vel = 1;
    }

    if( g_dir > 0 ) {
        stepper_up();
    } else {
        stepper_down();
    }

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
    while (defaultAccelTable[i][0]<g_vel) {
        i++;
    }
    g_micro_delay = defaultAccelTable[i][1];
    g_time0 = xTaskGetTickCount();
}

static void stepper_set_position( uint32_t position )
{
    // pos is unsigned so don't need to check for <0
    if( position >= STEP_CNT_MAX ) {
        position = STEP_CNT_MAX - 1;
    }

    g_target_step = position;

    if( g_stopped ) {
        // reset the timer to avoid possible time overflow giving spurious deltas
        g_stopped = false;
        g_time0 = xTaskGetTickCount();
        g_micro_delay = 0;
    }
}
static void stepper_update( void )
{
    if( !g_stopped ) {
        unsigned long delta = xTaskGetTickCount() - g_time0;
        if( delta >= g_micro_delay ) {
            stepper_advance();
        }
    }
}