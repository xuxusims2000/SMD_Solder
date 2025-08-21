idf_component_register(
    SRCS "ILI9341_screen.c" "SPI_Config.c" "Temp_Ctrl.c" "PWM_Config.c" "Screen_design.c" 
    INCLUDE_DIRS "."
    REQUIRES lvgl driver esp_lcd esp_lcd_ili9341 esp_lcd_touch_xpt2046
)


