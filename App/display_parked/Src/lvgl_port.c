#include "lvgl_port.h"

#include "lcd_status.h"
#include "lvgl.h"
#include "stm32f429i_discovery_lcd.h"

#define LVGL_FRAME_BUFFER ((lv_color_t *)LCD_FRAME_BUFFER)

static lv_disp_draw_buf_t lvgl_disp_buf;
static lv_disp_drv_t lvgl_disp_drv;

static void LVGL_Flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    /* Direct mode: LVGL renders into the LTDC frame buffer, so there is
     * nothing to copy here. */
    (void)area;
    (void)color_p;
    lv_disp_flush_ready(drv);
}

void LVGL_Port_Init(void)
{
    lv_init();

    lv_disp_draw_buf_init(
        &lvgl_disp_buf,
        LVGL_FRAME_BUFFER,
        NULL,
        LCD_WIDTH * LCD_HEIGHT);

    lv_disp_drv_init(&lvgl_disp_drv);
    lvgl_disp_drv.hor_res = LCD_WIDTH;
    lvgl_disp_drv.ver_res = LCD_HEIGHT;
    lvgl_disp_drv.flush_cb = LVGL_Flush;
    lvgl_disp_drv.draw_buf = &lvgl_disp_buf;
    lv_disp_drv_register(&lvgl_disp_drv);
}
