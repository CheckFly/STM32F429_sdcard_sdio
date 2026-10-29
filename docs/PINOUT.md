# Pinout

Source: `F429_sdcard_sdio_ecran.ioc` and `Core/Src/stm32f4xx_hal_msp.c`.

## LTDC — RGB565 display

| Pin | Signal | Direction | Notes |
|-----|--------|-----------|-------|
| PF10 | LTDC_DE | AF14 | Data enable |
| PA3 | LTDC_B5 | AF14 | Blue |
| PA4 | LTDC_VSYNC | AF14 | Vertical sync |
| PA6 | LTDC_G2 | AF14 | Green |
| PB0 | LTDC_R3 | AF9 | Red |
| PB1 | LTDC_R6 | AF9 | Red |
| PE11 | LTDC_G3 | AF14 | Green |
| PE12 | LTDC_B4 | AF14 | Blue |
| PE14 | LTDC_CLK | AF14 | Pixel clock |
| PE15 | LTDC_R7 | AF14 | Red |
| PB10 | LTDC_G4 | AF14 | Green |
| PB11 | LTDC_G5 | AF14 | Green |
| PD10 | LTDC_B3 | AF14 | Blue |
| PC6 | LTDC_HSYNC | AF14 | Horizontal sync |
| PC7 | LTDC_G6 | AF14 | Green |
| PA11 | LTDC_R4 | AF14 | Red |
| PA12 | LTDC_R5 | AF14 | Red |
| PD3 | LTDC_G7 | AF14 | Green |
| PB8 | LTDC_B6 | AF14 | Blue |
| PB9 | LTDC_B7 | AF14 | Blue |

> The `.ioc` uses `RGB565` mode, but some RGB565 pins (for example R3/R6) use `GPIO_AF9_LTDC` instead of `AF14` because of the STM32F429 alternate-function mapping. Keep the generated values; do not “fix” them without checking the datasheet.

## SDIO — SD card

> **PC10 note:** On the 32F429IDISCOVERY board, **PC10 is LCD_TFT R2** (on-board 2.4" LCD, see the official BSP). The display stack is parked, so PC10 is currently used as `SDIO_D2`. Re-validate signal integrity before restoring LTDC.
>
> The firmware uses **4-bit SDIO**:
>
> - `PC8` = SDIO_D0, `PC9` = SDIO_D1, `PC10` = SDIO_D2, `PC11` = SDIO_D3
> - `PC12` = SDIO_CK
> - `PD2` = SDIO_CMD
>
> The card is initialized in 1-bit mode, then switched to 4-bit with
> `HAL_SD_ConfigWideBusOperation()`; the driver falls back to 1-bit on failure.

| Pin | Signal | Direction | Notes |
|-----|--------|-----------|-------|
| PC8 | SDIO_D0 | AF12 | Data 0 |
| PC9 | SDIO_D1 | AF12 | Data 1 |
| PC10 | SDIO_D2 | AF12 | Data 2 (also LTDC_R2 when the display is restored) |
| PC11 | SDIO_D3 | AF12 | Data 3 |
| PC12 | SDIO_CK | AF12 | Clock, no pull |
| PD2 | SDIO_CMD | AF12 | Command, pull-up |

## Card detect and serial

| Pin | Signal | Current mode | Required mode | Notes |
|-----|--------|--------------|---------------|-------|
| PA10 | `SD_CARD_DET` | GPIO input, pull-down | GPIO input, pull-down | Active-high card detect; validated over UART; conflicts with `USART1_RX` |
| PA9 | `USART1_TX` | AF7 push-pull | AF7 push-pull | TX-only log to COM39; confirmed working |
| PA0 | `UART4_TX` (alternative) | free | AF8 | Alternative if COM39 is wired to UART4 |
| PA1 | `UART4_RX` (alternative) | free | AF8 | Alternative if COM39 is wired to UART4 |

## UART conflicts

The LTDC and SDIO assignments consume most UART pins:

| UART | TX pin | RX pin | Conflict |
|------|--------|--------|----------|
| USART1 | PA9 | PA10 | RX used by `SD_CARD_DET` |
| USART2 | PA2 | PA3 | PA3 used by LTDC_B5 |
| USART3 | PB10 | PB11 | Both used by LTDC_G4/G5 |
| UART4 | PA0 | PA1 | Free on this pinout |
| UART5 | PC12 | PD2 | Both used by SDIO |
| USART6 | PC6 | PC7 | Both used by LTDC_HSYNC/G6 |

> **Decision required:** confirm whether COM39 is the ST-Link VCP (USART1) or an external USB-UART adapter. If USART1 is used for TX only, PA9 is sufficient and PA10 can remain the card-detect input. If bidirectional serial is required, use UART4 on PA0/PA1 and rewire COM39.

## Clock

| Item | Current value |
|------|---------------|
| SYSCLK source | HSI 16 MHz → PLL |
| PLL | PLLM 16, PLLN 360, PLLP 2 |
| SYSCLK | 180 MHz |
| AHB / APB1 / APB2 | 180 MHz / 45 MHz / 90 MHz |
| PLL48 (SDIO) | 45 MHz (PLLQ 8) |
| Flash latency | 5 WS |
| Voltage scaling | Scale 1 |

> `SystemClock_Config()` is customized in `main.c`; CubeMX regeneration will revert
> it unless the RCC/PLL settings are updated in the `.ioc`.
