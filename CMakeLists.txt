idf_build_get_property(idf_target IDF_TARGET)

idf_component_register(INCLUDE_DIRS "include")

target_link_libraries(${COMPONENT_LIB} INTERFACE "${CMAKE_CURRENT_LIST_DIR}/lib/${idf_target}/libesp_rader.a")
