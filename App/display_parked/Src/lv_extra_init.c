#include "lvgl.h"

#include "extra/layouts/flex/lv_flex.h"
#include "extra/layouts/grid/lv_grid.h"

/* LVGL calls this from lv_init(). Only the features compiled in this project
 * need to be initialized here. */
void lv_extra_init(void)
{
#if LV_USE_FLEX
    lv_flex_init();
#endif

#if LV_USE_GRID
    lv_grid_init();
#endif
}
