cmake_minimum_required(VERSION 3.20)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/arm-none-eabi.cmake)
project(stm32g431_driver C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()
set(CMAKE_C_FLAGS_DEBUG "-Og -g3")

set(TP ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# ---------------- CMSIS (core + STM32G4 device) ----------------
add_library(cmsis INTERFACE)
target_include_directories(cmsis SYSTEM INTERFACE
    ${TP}/cmsis/include
    ${TP}/cmsis/device/include)
target_compile_definitions(cmsis INTERFACE STM32G431xx)

# ---------------- FreeRTOS ----------------
add_library(freertos_config INTERFACE)
target_include_directories(freertos_config SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/app/inc)
target_link_libraries(freertos_config INTERFACE cmsis)

set(FREERTOS_PORT GCC_ARM_CM4F CACHE STRING "")
set(FREERTOS_HEAP 4 CACHE STRING "")
add_subdirectory(${TP}/FreeRTOS-Kernel)

# ---------------- Drivers ----------------
add_library(drivers STATIC
    drivers/src/err.c
    drivers/src/rcc.c
    drivers/src/gpio.c
    drivers/src/uart.c
    drivers/src/rng.c)
target_include_directories(drivers PUBLIC drivers/inc)
target_link_libraries(drivers PUBLIC cmsis freertos_kernel)

# ---------------- Application ----------------
add_executable(${PROJECT_NAME}
    app/src/main.c
    app/src/syscalls.c
    app/src/debug.c
    ${TP}/cmsis/device/src/system_stm32g4xx.c
    ${TP}/cmsis/device/src/startup_stm32g431xx.s)
target_include_directories(${PROJECT_NAME} PRIVATE app/inc)
target_link_libraries(${PROJECT_NAME} PRIVATE drivers freertos_kernel)
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")

set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/STM32G431KBTX_FLASH.ld)
target_link_options(${PROJECT_NAME} PRIVATE
    -T${LINKER_SCRIPT}
    -Wl,-Map=${PROJECT_NAME}.map,--print-memory-usage)
set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY LINK_DEPENDS ${LINKER_SCRIPT})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}>)

# ---------------- Flash (OpenOCD, connect under reset) ----------------
add_custom_target(flash
    COMMAND openocd -f ${CMAKE_CURRENT_SOURCE_DIR}/openocd.cfg
            -c "program $<TARGET_FILE:${PROJECT_NAME}> verify reset exit"
    DEPENDS ${PROJECT_NAME}
    USES_TERMINAL)
