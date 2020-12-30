
/*********************
 *      INCLUDES
 *********************/
#include <j1939.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "driver/gpio.h"
#include "driver/can.h"

#include "can_j1939.h"

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

/**********************
 *    PROTOTYPES
 **********************/


void can_j1939_start( void )
{
    bool    success;

    /* Configure Can Bus*/
    can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(GPIO_NUM_21, GPIO_NUM_22, CAN_MODE_NORMAL);
    can_timing_config_t t_config = CAN_TIMING_CONFIG_1MBITS();
    can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();

    /* Install CAN Driver */
    success = (can_driver_install(&g_config, &t_config, &f_config) == ESP_OK);
    assert( success );

    /* Start CAN Driver */
    if( success ) {
        success = (can_start() == ESP_OK);
        assert( success );
    }
}

void can_j1939_stop( void )
{
    bool    success;

    /* Stop CAN Driver */
    success = (can_stop() == ESP_OK);
    assert( success );

    /* Uninstall CAN Driver */
    if( success ) {
        success = (can_driver_uninstall() == ESP_OK);
        assert( success );
    }
}
