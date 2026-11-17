#pragma once

#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t dashboard_display_init(void);
lv_display_t* dashboard_display_get(void);

#ifdef __cplusplus
}
#endif
