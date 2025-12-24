#ifndef __M5GFX_LVGL_H__
#define __M5GFX_LVGL_H__

#include "lvgl.h"
#include "M5Unified.h"
#include "M5GFX.h"

void m5gfx_lvgl_init(void);

// Optional: lock/unlock display bus when sharing with SD
void lvgl_bus_lock(void);
void lvgl_bus_unlock(void);

#endif  // __M5GFX_LVGL_H__
