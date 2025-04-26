idf_component_register(
    SRCS "SPI_Config.c" "Temp_Ctrl.c" "PWM_Config.c" "lv_port_disp.c"
    INCLUDE_DIRS "."
    REQUIRES lvgl driver esp_lcd
)

