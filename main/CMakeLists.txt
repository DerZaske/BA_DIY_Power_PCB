idf_component_register(SRCS "functions.c" "app_main.c"
                       INCLUDE_DIRS "include""${CMAKE_SOURCE_DIR}""."
                       REQUIRES ssd1306 driver esp_adc)
