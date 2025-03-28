idf_component_register(
    SRCS "Temp_Ctrl.c" "I2C_Config.c" "PWM_Config.c" "lv_port_disp.c"
    INCLUDE_DIRS "."
    REQUIRES lvgl driver esp_lcd
)

