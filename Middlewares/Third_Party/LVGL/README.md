# LVGL middleware

LVGL is the selected GUI stack for the LTDC display.

## Status

- [ ] Add LVGL as a Git submodule.
- [ ] Create `lv_conf.h` (`LV_COLOR_DEPTH = 16`).
- [ ] Implement the LTDC flush callback.
- [ ] Allocate the frame buffer in SDRAM.
- [ ] Add `lv_timer_handler()` to the main loop.

## Add the submodule

```powershell
git submodule add https://github.com/lvgl/lvgl.git Middlewares/Third_Party/LVGL
git submodule update --init --recursive
```

## Recommended `lv_conf.h`

```c
#define LV_COLOR_DEPTH 16
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (48U * 1024U)
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE "main.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (HAL_GetTick())
```

## Display port

- Use the LTDC RGB565 layer as the display.
- Copy the rendered area into the frame buffer in the flush callback.
- Call `lv_disp_flush_ready()` when the copy is complete.
- Confirm the panel resolution and timings before enabling the layer.

## Do not

- Do not hardcode the frame-buffer address until the SDRAM/FMC path is verified.
- Do not put the frame buffer on the stack.
