set(LV_UI_DIR ui)
file(GLOB_RECURSE LV_UI_SOURCES ${LV_UI_DIR}/*.c)

set(APP_DIR app)
file(GLOB_RECURSE APP_SOURCES ${APP_DIR}/*.c)

idf_component_register(SRCS "app_main.c" ${LV_UI_SOURCES} ${APP_SOURCES}
                       INCLUDE_DIRS "." ${LV_UI_DIR}  ${APP_DIR})
