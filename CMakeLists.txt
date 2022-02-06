# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.12)

# Include build functions from Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)



# Set name of project (as PROJECT_NAME) and C/C++ standards
project(RPPicoMandelbrot C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()




# Tell CMake where to find the executable source file
add_executable(${PROJECT_NAME} 
    main.c
)


# RAM program
pico_set_binary_type(${PROJECT_NAME}  no_flash)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})

################################################################################
add_library(st7735 st7735/st7735.c st7735/st7735.h)


target_link_libraries(st7735
    pico_stdlib
    hardware_spi
    hardware_dma
)
################################################################################



# Link to pico_stdlib (gpio, time, etc. functions)
target_link_libraries(${PROJECT_NAME} 
    pico_stdlib
    pico_multicore
    pico_base_headers
    hardware_clocks
    hardware_exception
    hardware_pwm
    st7735
)



target_compile_options(${PROJECT_NAME} PRIVATE
    -Os
)

# Enable usb output, disable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

add_custom_command(TARGET ${PROJECT_NAME} 
        POST_BUILD
        COMMAND arm-none-eabi-size ${PROJECT_NAME}.elf
        )