cmake_minimum_required(VERSION 4.1)
project(src C)

set(CMAKE_C_STANDARD 11)

add_definitions(-DSTM32F446xx -DUSE_HAL_DRIVER)

include_directories(
        Core/Inc
        Drivers/STM32F4xx_HAL_Driver/Inc
        Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
        Drivers/CMSIS/Include
        Drivers/CMSIS/Device/ST/STM32F4xx/Include
)

# Tous les .c du Core (ton code)
file(GLOB_RECURSE SRC_SOURCES
        Core/Src/*.c
)

# Tous les .c du HAL
file(GLOB HAL_SOURCES
        Drivers/STM32F4xx_HAL_Driver/Src/*.c
)

add_executable(src
        ${SRC_SOURCES}
        ${HAL_SOURCES}
)