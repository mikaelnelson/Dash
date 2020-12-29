
/*********************
 *      INCLUDES
 *********************/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "stepper_gauge.h"

/*********************
 *      DEFINES
 *********************/

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

/**********************
 *    PROTOTYPES
 **********************/
static void stepper_gauge_update_timer( TimerHandle_t pxTimer );


void stepper_gauge_start( void )
{
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

}