idf_build_get_property(idf_target IDF_TARGET)

idf_component_register(
                       INCLUDE_DIRS "include")

# set(WRAP_FUNCTIONS
#     esp_wifi_rader_set_config
#     esp_wifi_rader_get_config
#     esp_wifi_rader_start
#     esp_wifi_rader_stop
#     esp_wifi_rader_init
#     esp_wifi_rader_deinit)

target_link_libraries(${COMPONENT_LIB} INTERFACE "${CMAKE_CURRENT_LIST_DIR}/lib/${idf_target}/libesp_rader.a")

