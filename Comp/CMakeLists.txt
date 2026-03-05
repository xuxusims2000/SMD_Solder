# Collect all UI source files automatically
file(GLOB_RECURSE UI_SOURCES "UI/*.c")

idf_component_register(
    SRCS
        "scr/ILI9341_screen.c"
        "scr/Temp_Sensing.c"
        "scr/Temp_Ctrl.c"
        "scr/Screen_design.c"
        "scr/Display_Manager.c"
        "scr/SMD_Manager.c"
        "Test/Test.c"
        "Test/Temp_Sensing/TestTemp_Sensing.c"
        "Test/Display_Manager/TestDisplay_Manager.c"
        "Test/SMD_Manager/TestSMD_Manager.c"
        "Test/Temp_Ctrl/TestTemp_Ctrl.c"
        ${UI_SOURCES}
    INCLUDE_DIRS
        "includes"
        "scr"
        "Test"
        "Test/Temp_Sensing"
        "Test/Display_Manager"
        "Test/SMD_Manager"
        "Test/Temp_Ctrl"
        "UI"
        "."
    REQUIRES
        lvgl
        driver
        esp_lcd
        esp_lcd_ili9341
        esp_lcd_touch_xpt2046
)
