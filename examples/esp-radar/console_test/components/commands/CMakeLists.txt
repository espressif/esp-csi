idf_component_register(SRC_DIRS "src"
                       INCLUDE_DIRS "include"
                       REQUIRES  "console" "mbedtls" "nvs_flash" "fatfs" "esp_wifi" "spi_flash")
                       
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
