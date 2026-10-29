#ifndef APP_LCD_STATUS_H
#define APP_LCD_STATUS_H

#ifdef __cplusplus
extern "C" {
#endif

#define LCD_WIDTH  240U
#define LCD_HEIGHT 320U

/** Initialize the on-board 240x320 TFT, LVGL, and the status screen. */
void LCD_Status_Init(void);

/** Update the card-presence line: 1 = OUI, 0 = NON. */
void LCD_Status_SetCardPresent(int present);

/** Run the LVGL timer handler; call regularly from the main loop. */
void LCD_Status_Process(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_LCD_STATUS_H */
