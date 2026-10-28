# Screen — LTDC + LVGL

## Hardware

| Item | Value |
|------|-------|
| Interface | LTDC RGB565 |
| Resolution | **to confirm with the physical panel** |
| Pixel format | RGB565 |
| Frame buffer | SDRAM preferred (8 MB available) |
| Touch | not configured |

## LTDC pins

See [../../PINOUT.md](../../PINOUT.md) for the full pin table.

Key signals:

- `PE14` — LTDC_CLK
- `PC6` — LTDC_HSYNC
- `PA4` — LTDC_VSYNC
- `PF10` — LTDC_DE
- `PC7` — LTDC_G6
- `PB9` — LTDC_B7
- `PE15` — LTDC_R7

## Current state

`MX_LTDC_Init` in `Core/Src/main.c`:

- Sync timings: HSYNC 7, VSYNC 3, HBP 14, VBP 5, active width 654, active height 485, total 660 × 487.
- Both layers are configured with `WindowX0 = WindowX1 = 0`, `ImageWidth = 0`, `ImageHeight = 0`, and `FBStartAdress = 0`.
- Result: no visible output.

## LVGL integration plan

1. Confirm panel resolution and timings.
2. Allocate the frame buffer in SDRAM:

   ```c
   #define LCD_WIDTH   480
   #define LCD_HEIGHT  272
   #define LCD_FB_ADDR 0xC0000000U
   ```

3. Configure LTDC layer 0:

   ```c
   pLayerCfg.WindowX0 = 0;
   pLayerCfg.WindowX1 = LCD_WIDTH;
   pLayerCfg.WindowY0 = 0;
   pLayerCfg.WindowY1 = LCD_HEIGHT;
   pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
   pLayerCfg.FBStartAdress = LCD_FB_ADDR;
   pLayerCfg.ImageWidth = LCD_WIDTH;
   pLayerCfg.ImageHeight = LCD_HEIGHT;
   ```

4. Add LVGL:

   ```powershell
   git submodule add https://github.com/lvgl/lvgl.git Middlewares/Third_Party/LVGL
   ```

5. Create `lv_conf.h`:

   ```c
   #define LV_COLOR_DEPTH 16
   #define LV_MEM_CUSTOM 0
   #define LV_MEM_SIZE (48U * 1024U)
   #define LV_TICK_CUSTOM 1
   #define LV_TICK_CUSTOM_INCLUDE "main.h"
   #define LV_TICK_CUSTOM_SYS_TIME_EXPR (HAL_GetTick())
   ```

6. Implement the flush callback:

   ```c
   static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
   {
       /* Copy color_p to the LTDC frame buffer at area->x1/area->y1. */
       lv_disp_flush_ready(drv);
   }
   ```

7. Call `lv_init()` once and `lv_timer_handler()` from the main loop.

## Display test

- Fill the screen with red, green, blue.
- Draw a rectangle and some text.
- Verify no tearing and correct RGB order.
- Record the result in `docs/TESTS.md`.

## Known issues

- LTDC layers are zero-sized.
- No frame buffer is allocated.
- The panel resolution/timings are not confirmed.
- The current clock is 16 MHz HSI; higher pixel clocks may require the PLL.
- The board's on-board LCD is SPI, not LTDC; the external panel wiring must be verified.
