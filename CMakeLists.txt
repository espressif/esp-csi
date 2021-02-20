set(EXPECT_IDF_VERSION "6e776946d01ec0d081d09000c36d23ec1d318c06")

execute_process(COMMAND sh "check_idf_version.sh" ${EXPECT_IDF_VERSION} WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/tools")
idf_build_get_property(idf_target IDF_TARGET)

idf_component_register(INCLUDE_DIRS "include")

target_link_libraries(${COMPONENT_LIB} INTERFACE "${CMAKE_CURRENT_LIST_DIR}/lib/${idf_target}/libesp_rader.a")
