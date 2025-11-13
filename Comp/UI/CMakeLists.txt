SET(SOURCES screens/ui_Screen1.c
    screens/ui_Screen2.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    images/ui_img_bakground_blue_png.c
    images/ui_img_icon_gold_png.c
    images/ui_img_left_arrow_png.c
    fonts/ui_font_Roboto2.c
    fonts/ui_font_Roboto1.c)

add_library(ui ${SOURCES})
