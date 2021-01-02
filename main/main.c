/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <pubsub.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "display.h"
#include "gps.h"
#include "stepper_gauge.h"
#include "can_j1939.h"


void app_main()
{
    // Initialize PubSub
    ps_init();

    // Start Modules
    display_start();
    gps_start();
//    stepper_gauge_start();
//    can_j1939_start();

//    // Handle Messages
//
//    // Stop Modules
//    can_j1939_stop();
//    stepper_gauge_stop();
//    display_stop();
//
//    esp_restart();
}
