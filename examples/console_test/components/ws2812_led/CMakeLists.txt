if(CONFIG_WS2812_LED_ENABLE)
    set(srcs "ws2812_led.c" "led_strip_rmt_ws2812.c")
else()
    set(srcs "ws2812_led.c")
endif()

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES "driver")
