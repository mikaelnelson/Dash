/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <time.h>
#include <pubsub.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "console_intf.h"
#include "display.h"
#include "gps.h"
#include "stepper_gauge.h"
#include "speedometer_gauge.h"
#include "can_j1939.h"


void app_main()
{
    // Initialize PubSub
    ps_init();

    // Setup Time Zone
    setenv("TZ", "CST6CDT,M3.2.0,M11.1.0", 1);
    tzset();

    // Init Modules
    console_intf_init();
    speedometer_gauge_init();

    // Start Modules
    console_intf_start();
    display_start();
    gps_start();
    speedometer_gauge_start();
    stepper_gauge_start();

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
