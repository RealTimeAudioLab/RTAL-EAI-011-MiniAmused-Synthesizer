#ifndef LV_CONF_H
#define LV_CONF_H

/* RTAL MiniMoog v0.5.5b2
 *
 * ESP32-S3 stability configuration:
 * - LVGL is serviced from Arduino loop() via lv_timer_handler().
 * - No LVGL OS worker threads are required.
 * - One software draw unit prevents creation of parallel swdraw workers.
 */

#define LV_COLOR_DEPTH 16

/* DISPLAY TURBO3 - aggressive UI timing, 16 ms parameter redraw coalescing */
#define LV_DISP_DEF_REFR_PERIOD 16
#define LV_INDEV_DEF_READ_PERIOD 5

/* 0.6.8a CENTER-SNAP1
 * Use the ESP32/Arduino heap instead of LVGL 8.x internal static pool.
 * This is important for the complete six-page UI plus the shared overlay.
 */
#define LV_MEM_CUSTOM 1
#define LV_MEM_CUSTOM_INCLUDE <stdlib.h>
#define LV_MEM_CUSTOM_ALLOC   malloc
#define LV_MEM_CUSTOM_FREE    free
#define LV_MEM_CUSTOM_REALLOC realloc




#define LV_USE_LOG 0

#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1

#endif
