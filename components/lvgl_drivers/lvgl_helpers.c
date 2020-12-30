/**
 * @file lvgl_helpers.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "sdkconfig.h"
#include "lvgl_helpers.h"
#include "esp_log.h"

#include "lvgl_i2c_conf.h"

#include "driver/i2c.h"

#include "lvgl/src/lv_core/lv_refr.h"

/*********************
 *      DEFINES
 *********************/

#define TAG "lvgl_helpers"

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/* Interface and driver initialization */
void lvgl_driver_init(void)
{
    ESP_LOGI(TAG, "Display hor size: %d, ver size: %d", LV_HOR_RES_MAX, LV_VER_RES_MAX);
    ESP_LOGI(TAG, "Display buffer size: %d", DISP_BUF_SIZE);

#if defined (SHARED_I2C_BUS)
    ESP_LOGI(TAG, "Initializing shared I2C master");

    lvgl_i2c_driver_init(DISP_I2C_PORT,
        DISP_I2C_SDA, DISP_I2C_SCL,
        DISP_I2C_SPEED_HZ);

    disp_driver_init();
    touch_driver_init();

    return;
#endif

/* Display controller initialization */
#if defined (CONFIG_LVGL_TFT_DISPLAY_PROTOCOL_I2C)
    ESP_LOGI(TAG, "Initializing I2C master for display");
    /* Init the i2c master on the display driver code */
    lvgl_i2c_driver_init(DISP_I2C_PORT,
        DISP_I2C_SDA, DISP_I2C_SCL,
        DISP_I2C_SPEED_HZ);

    disp_driver_init();
#else
#error "No protocol defined for display controller"
#endif

/* Touch controller initialization */
#if CONFIG_LVGL_TOUCH_CONTROLLER != TOUCH_CONTROLLER_NONE
    #if defined (CONFIG_LVGL_TOUCH_DRIVER_PROTOCOL_SPI)
        ESP_LOGI(TAG, "Initializing SPI master for touch");

        lvgl_spi_driver_init(TOUCH_SPI_HOST,
            TP_SPI_MISO, TP_SPI_MOSI, TP_SPI_CLK,
            0 /* Defaults to 4094 */, 2,
            -1, -1);

        tp_spi_add_device(TOUCH_SPI_HOST);

        touch_driver_init();
    #elif defined (CONFIG_LVGL_TOUCH_DRIVER_PROTOCOL_I2C)
        ESP_LOGI(TAG, "Initializing I2C master for touch");

        lvgl_i2c_driver_init(TOUCH_I2C_PORT,
            TOUCH_I2C_SDA, TOUCH_I2C_SCL,
            TOUCH_I2C_SPEED_HZ);

        touch_driver_init();
    #elif defined (CONFIG_LVGL_TOUCH_DRIVER_ADC)
        touch_driver_init();
    #elif defined (CONFIG_LVGL_TOUCH_DRIVER_DISPLAY)
        touch_driver_init();
    #else
    #error "No protocol defined for touch controller"
    #endif
#else
#endif
}

/* Config the i2c master
 *
 * This should init the i2c master to be used on display and touch controllers.
 * So we should be able to know if the display and touch controllers shares the
 * same i2c master.
 */
bool lvgl_i2c_driver_init(int port, int sda_pin, int scl_pin, int speed_hz)
{
    esp_err_t err;

    ESP_LOGI(TAG, "Initializing I2C master port %d...", port);
    ESP_LOGI(TAG, "SDA pin: %d, SCL pin: %d, Speed: %d (Hz)",
             sda_pin, scl_pin, speed_hz);

    i2c_config_t conf = {
            .mode               = I2C_MODE_MASTER,
            .sda_io_num         = sda_pin,
            .sda_pullup_en      = GPIO_PULLUP_ENABLE,
            .scl_io_num         = scl_pin,
            .scl_pullup_en      = GPIO_PULLUP_ENABLE,
            .master.clk_speed   = speed_hz,
    };

    ESP_LOGI(TAG, "Setting I2C master configuration...");
    err = i2c_param_config(port, &conf);
    assert(ESP_OK == err);

    ESP_LOGI(TAG, "Installing I2C master driver...");
    err = i2c_driver_install(port,
                             I2C_MODE_MASTER,
                             0, 0 /*I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE */,
                             0 /* intr_alloc_flags */);
    assert(ESP_OK == err);

    return ESP_OK != err;
}

