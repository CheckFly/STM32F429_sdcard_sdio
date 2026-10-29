#ifndef APP_LVGL_PORT_H
#define APP_LVGL_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize LVGL and register the 240x320 RGB565 display driver. */
void LVGL_Port_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_LVGL_PORT_H */
