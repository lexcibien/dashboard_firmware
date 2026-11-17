idf_component_register(
    SRCS "display.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_lcd esp_lcd_gc9a01 esp_lvgl_port lvgl
)
