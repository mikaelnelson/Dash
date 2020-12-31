/**
 * @file disp_driver.c
 */

#include "disp_driver.h"

void disp_driver_init(void)
{
    ssd1306_init();
}

void disp_driver_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_map)
{
    ssd1306_flush(drv, area, color_map);
}

void disp_driver_rounder(lv_disp_drv_t * disp_drv, lv_area_t * area)
{
    ssd1306_rounder(disp_drv, area);
}

void disp_driver_set_px(lv_disp_drv_t * disp_drv, uint8_t * buf, lv_coord_t buf_w, lv_coord_t x, lv_coord_t y,
        lv_color_t color, lv_opa_t opa)
{
    ssd1306_set_px_cb(disp_drv, buf, buf_w, x, y, color, opa);
}
