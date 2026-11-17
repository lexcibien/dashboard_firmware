#include "dashboard_display.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_lcd_gc9a01.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "sdkconfig.h"

namespace {

constexpr char kTag[] = "dashboard_display";
constexpr int kDisplayWidth = 240;
constexpr int kDisplayHeight = 240;
constexpr int kBufferLines = 20;

esp_lcd_panel_io_handle_t s_panel_io = nullptr;
esp_lcd_panel_handle_t s_panel = nullptr;
lv_display_t* s_display = nullptr;

void initialize_backlight() {
#if CONFIG_DASHBOARD_LCD_BACKLIGHT_GPIO >= 0
  const gpio_num_t backlight_gpio = static_cast<gpio_num_t>(CONFIG_DASHBOARD_LCD_BACKLIGHT_GPIO);
  ESP_ERROR_CHECK(gpio_reset_pin(backlight_gpio));
  ESP_ERROR_CHECK(gpio_set_direction(backlight_gpio, GPIO_MODE_OUTPUT));
  ESP_ERROR_CHECK(gpio_set_level(backlight_gpio, 1));
#endif
}

} // namespace

extern "C" esp_err_t dashboard_display_init() {
  if (s_display != nullptr) {
    return ESP_ERR_INVALID_STATE;
  }

  const gpio_num_t sclk_gpio = static_cast<gpio_num_t>(CONFIG_DASHBOARD_LCD_SCLK_GPIO);
  const gpio_num_t mosi_gpio = static_cast<gpio_num_t>(CONFIG_DASHBOARD_LCD_MOSI_GPIO);
  const gpio_num_t cs_gpio = static_cast<gpio_num_t>(CONFIG_DASHBOARD_LCD_CS_GPIO);
  const gpio_num_t dc_gpio = static_cast<gpio_num_t>(CONFIG_DASHBOARD_LCD_DC_GPIO);
  const gpio_num_t reset_gpio = static_cast<gpio_num_t>(CONFIG_DASHBOARD_LCD_RST_GPIO);

  ESP_LOGI(kTag, "Initializing GC9A01 at %d Hz (SCLK=%d MOSI=%d CS=%d DC=%d RST=%d)", CONFIG_DASHBOARD_LCD_SPI_CLOCK_HZ, sclk_gpio, mosi_gpio,
           cs_gpio, dc_gpio, reset_gpio);

  spi_bus_config_t bus_config = {};
  bus_config.sclk_io_num = sclk_gpio;
  bus_config.mosi_io_num = mosi_gpio;
  bus_config.miso_io_num = GPIO_NUM_NC;
  bus_config.quadwp_io_num = GPIO_NUM_NC;
  bus_config.quadhd_io_num = GPIO_NUM_NC;
  bus_config.max_transfer_sz = kDisplayWidth * kBufferLines * static_cast<int>(sizeof(uint16_t));
  ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_config, SPI_DMA_CH_AUTO));

  esp_lcd_panel_io_spi_config_t io_config = {};
  io_config.cs_gpio_num = cs_gpio;
  io_config.dc_gpio_num = dc_gpio;
  io_config.spi_mode = 0;
  io_config.pclk_hz = CONFIG_DASHBOARD_LCD_SPI_CLOCK_HZ;
  io_config.trans_queue_depth = 10;
  io_config.lcd_cmd_bits = 8;
  io_config.lcd_param_bits = 8;
  ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(static_cast<esp_lcd_spi_bus_handle_t>(SPI2_HOST), &io_config, &s_panel_io));

  esp_lcd_panel_dev_config_t panel_config = {};
  panel_config.reset_gpio_num = reset_gpio;
  panel_config.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
  panel_config.bits_per_pixel = 16;
  ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(s_panel_io, &panel_config, &s_panel));
  ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
  ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
#if CONFIG_DASHBOARD_LCD_INVERT_COLOR
  ESP_ERROR_CHECK(esp_lcd_panel_invert_color(s_panel, true));
#endif
  ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(s_panel, true));
  initialize_backlight();

  lvgl_port_cfg_t lvgl_config = ESP_LVGL_PORT_INIT_CONFIG();
  lvgl_config.timer_period_ms = 5;
  ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_config), kTag, "LVGL port initialization failed");

  lvgl_port_display_cfg_t display_config = {};
  display_config.io_handle = s_panel_io;
  display_config.panel_handle = s_panel;
  display_config.buffer_size = kDisplayWidth * kBufferLines;
  display_config.double_buffer = true;
  display_config.hres = kDisplayWidth;
  display_config.vres = kDisplayHeight;
  display_config.monochrome = false;
  display_config.color_format = LV_COLOR_FORMAT_RGB565;
  display_config.rotation.swap_xy = false;
  display_config.rotation.mirror_x = false;
  display_config.rotation.mirror_y = false;
  display_config.flags.buff_dma = true;
  display_config.flags.swap_bytes = CONFIG_DASHBOARD_LCD_SWAP_BYTES;
  display_config.flags.sw_rotate = false;

  s_display = lvgl_port_add_disp(&display_config);
  if (s_display == nullptr) {
    ESP_LOGE(kTag, "Failed to add GC9A01 display to LVGL");
    return ESP_ERR_NO_MEM;
  }

  return ESP_OK;
}

extern "C" lv_display_t* dashboard_display_get() { return s_display; }
