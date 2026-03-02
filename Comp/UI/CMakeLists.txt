SET(SOURCES components/ui_comp_panel1.c
    components/ui_comp.c
    screens/ui_SCLolos.c
    screens/ui_SCHome.c
    screens/ui_SCSolder.c
    screens/ui_SCSettings.c
    screens/ui_SCSetTemp.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    ui_events.c
    images/ui_img_bk_dark_png.c
    images/ui_img_home3_png.c
    fonts/ui_font_LolosFactoryFont24.c)

add_library(ui ${SOURCES})
