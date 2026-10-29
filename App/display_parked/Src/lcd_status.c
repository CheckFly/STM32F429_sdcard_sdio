#include "lcd_status.h"

#include <string.h>

#include "app_info.h"
#include "log_uart.h"
#include "lvgl.h"
#include "lvgl_port.h"
#include "stm32f429i_discovery_lcd.h"

static lv_obj_t *presence_label;
static int last_present = -1;

static void LCD_SDRAM_Test(void)
{
    uint32_t pattern[4] = {0x12345678U, 0xA5A5A5A5U, 0xDEADBEEFU, 0x55AA55AAU};
    uint32_t readback[4] = {0U, 0U, 0U, 0U};
    uint32_t address = 0xD0100000U;
    uint8_t write_result = BSP_SDRAM_WriteData(address, pattern, 4U);
    uint8_t read_result = BSP_SDRAM_ReadData(address, readback, 4U);
    int ok = (write_result == 0U) && (read_result == 0U) &&
             (memcmp(pattern, readback, sizeof(pattern)) == 0);

    LOG_Printf("SDRAM:%s\r\n", ok ? "OK" : "FAIL");
}

static void LCD_ConfigureRGB565Layer(void)
{
    LTDC_HandleTypeDef hltdc = {0};
    LTDC_LayerCfgTypeDef layer = {0};

    hltdc.Instance = LTDC;

    layer.WindowX0 = 0U;
    layer.WindowX1 = LCD_WIDTH;
    layer.WindowY0 = 0U;
    layer.WindowY1 = LCD_HEIGHT;
    layer.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
    layer.Alpha = 0xFFU;
    layer.Alpha0 = 0U;
    layer.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
    layer.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
    layer.FBStartAdress = LCD_FRAME_BUFFER;
    layer.ImageWidth = LCD_WIDTH;
    layer.ImageHeight = LCD_HEIGHT;
    layer.Backcolor.Red = 0U;
    layer.Backcolor.Green = 0U;
    layer.Backcolor.Blue = 0U;

    if (HAL_LTDC_ConfigLayer(&hltdc, &layer, 0U) != HAL_OK)
    {
        Error_Handler();
    }
}

static void LCD_BuildUI(void)
{
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_scr_load(screen);

    lv_obj_t *version_label = lv_label_create(screen);
    lv_label_set_text(version_label, "F429 SD v" APP_VERSION);
    lv_obj_set_style_text_font(version_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(version_label, lv_color_make(0, 0, 255), 0);
    lv_obj_align(version_label, LV_ALIGN_TOP_MID, 0, 20);

    presence_label = lv_label_create(screen);
    lv_label_set_text(presence_label, "Presence carte : NON");
    lv_obj_set_style_text_font(presence_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(presence_label, lv_color_make(255, 0, 0), 0);
    lv_obj_align(presence_label, LV_ALIGN_TOP_MID, 0, 60);
}

void LCD_Status_Init(void)
{
    (void)BSP_LCD_Init();
    LCD_SDRAM_Test();
    LCD_ConfigureRGB565Layer();
    LVGL_Port_Init();
    LCD_BuildUI();
    LCD_Status_SetCardPresent(0);
    lv_timer_handler();
}

void LCD_Status_SetCardPresent(int present)
{
    if (presence_label == NULL || present == last_present)
    {
        return;
    }

    last_present = present;
    lv_label_set_text(
        presence_label,
        present ? "Presence carte : OUI" : "Presence carte : NON");
    lv_obj_set_style_text_color(
        presence_label,
        present ? lv_color_make(0, 128, 0) : lv_color_make(255, 0, 0),
        0);
}

void LCD_Status_Process(void)
{
    lv_timer_handler();
}
