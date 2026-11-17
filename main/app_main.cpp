#include "dashboard_display.h"
#include "dashboard_ui.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void app_main() {
  ESP_ERROR_CHECK(dashboard_display_init());
  ESP_ERROR_CHECK(dashboard_ui_init());

  // O esp_lvgl_port mantém a tarefa LVGL. Esta tarefa apenas mantém o app vivo.
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
