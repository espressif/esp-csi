idf_component_register(
    SRCS "bsp_C5_dual_antenna.c"  ${SRC_VER}
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "priv_include"
    REQUIRES driver spiffs esp_wifi
    PRIV_REQUIRES fatfs esp_lcd
)
