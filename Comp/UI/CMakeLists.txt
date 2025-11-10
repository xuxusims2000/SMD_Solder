SET(SOURCES screens/ui_Screen1.c
    screens/ui_Screen2.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    images/ui_img_up_buton_png.c
    images/ui_img_retun_button_png.c)

add_library(ui ${SOURCES})
