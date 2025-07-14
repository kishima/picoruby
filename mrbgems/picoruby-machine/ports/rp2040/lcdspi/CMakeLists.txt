# lcdspi library for LCD SPI display

add_library(lcdspi STATIC
    lcdspi.c
)

target_include_directories(lcdspi PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/fonts
)

target_link_libraries(lcdspi PRIVATE
    pico_stdlib
    hardware_spi
    hardware_gpio
    hardware_timer
    hardware_dma
    pico_multicore
)