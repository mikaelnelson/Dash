
/*********************
 *      INCLUDES
 *********************/
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>

#include <pubsub.h>
#include <j1939.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "driver/gpio.h"
#include "driver/can.h"

#include "can_j1939.h"

/*********************
 *      DEFINES
 *********************/
#define TAG "CAN_J1939"

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
    bool            success;
    int             rc;
    const uint8_t   src = 0x80;

    ESP_LOGI(TAG, "can j1939 start");

    ecu_name_t name = {
            .fields.arbitrary_address_capable = J1939_NO_ADDRESS_CAPABLE,
            .fields.industry_group = J1939_INDUSTRY_GROUP_INDUSTRIAL,
            .fields.vehicle_system_instance = 1,
            .fields.vehicle_system = 1,
            .fields.function = 1,
            .fields.reserved = 0,
            .fields.function_instance = 1,
            .fields.ecu_instance = 1,
            .fields.manufacturer_code = 1,
            .fields.identity_number = 1,
    };

    /* Configure Can Bus*/
    ESP_LOGI(TAG, "configure CAN bus");
    can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(GPIO_NUM_2, GPIO_NUM_4, CAN_MODE_NORMAL);
    can_timing_config_t t_config = CAN_TIMING_CONFIG_1MBITS();
    can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();



    /* Install CAN Driver */
    ESP_LOGI(TAG, "Install CAN driver");
    success = (can_driver_install(&g_config, &t_config, &f_config) == ESP_OK);
    ESP_LOGI(TAG, "success: %d", success);

    /* Start CAN Driver */
    if( success ) {
        ESP_LOGI(TAG, "Start CAN driver");
        success = (can_start() == ESP_OK);
    }

    /* Claim Address */
    if( success ) {
        ESP_LOGI(TAG, "can j1939 claim address");
        rc = j1939_address_claim(src, name);
        success = ( -1 != rc );

        if( success ) {
            ESP_LOGI(TAG, "can j1939 claimed");
            j1939_address_claimed(src, name);
        }
    }
}

void can_j1939_stop( void )
{
    bool    success;

    /* Stop CAN Driver */
    success = (can_stop() == ESP_OK);

    /* Uninstall CAN Driver */
    if( success ) {
        success = (can_driver_uninstall() == ESP_OK);
    }
}


/********************************************
 *      LIBRARY EXTERN IMPLEMENTATIONS
 ********************************************/
int j1939_cansend( uint32_t id, uint8_t * data, uint8_t len )
{
    can_message_t message;

    // Setup Message
    message.extd = 1;
    message.identifier = id;
    message.data_length_code = len;
    memcpy(message.data, data, message.data_length_code);

    if (can_transmit(&message, pdMS_TO_TICKS(100)) == ESP_OK) {
        return -1;
    }
    return message.data_length_code;
}

int j1939_canrcv( uint32_t * id, uint8_t * data )
{
    can_message_t message;

    if (can_receive(&message, pdMS_TO_TICKS(100)) != ESP_OK) {
        return - 1;
    }

    memcpy(data, message.data, message.data_length_code);
    *id = message.identifier;
    return message.data_length_code;
}

uint32_t j1939_get_time(void)
{
    struct timespec tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_nsec / 1000000;
}

int j1939_filter(struct j1939_pgn_filter *filter, uint32_t num_filters)
{
    return 1;
}
