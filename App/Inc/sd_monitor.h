#ifndef APP_SD_MONITOR_H
#define APP_SD_MONITOR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the SD card presence monitor.
 *
 * The card-detect input is PA10, active low, with the internal pull-up enabled
 * by MX_GPIO_Init().
 */
void SD_Monitor_Init(void);

/**
 * @brief Poll the card-detect input and emit state changes on COM39.
 *
 * Call this periodically from the main loop. The function is non-blocking.
 */
void SD_Monitor_Process(void);

/**
 * @brief Return 1 when the debounced card-detect state is "present", else 0.
 */
int SD_Monitor_IsPresent(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_SD_MONITOR_H */
