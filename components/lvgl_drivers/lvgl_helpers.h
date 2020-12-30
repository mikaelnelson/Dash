/**
 * @file lvgl_helpers.h
 */

#ifndef LVGL_HELPERS_H
#define LVGL_HELPERS_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdbool.h>

#include "lvgl_tft/disp_driver.h"

/*********************
 *      DEFINES
 *********************/
#if defined CONFIG_LVGL_TFT_DISPLAY_CONTROLLER_SSD1306
#define DISP_BUF_SIZE  (CONFIG_LVGL_DISPLAY_WIDTH*CONFIG_LVGL_DISPLAY_HEIGHT)
#else
#error "No display controller selected"
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/* Initialize detected SPI and I2C bus and devices */
void lvgl_driver_init(void);

/* Initialize I2C master  */
bool lvgl_i2c_driver_init(int port, int sda_pin, int scl_pin, int speed);

/**********************
 *      MACROS
 **********************/


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LVGL_HELPERS_H */
