cmake_minimum_required(VERSION 3.25)

project(eval-eth.elf LANGUAGES C)
set(PROJECT_ROOT_NAME eval-eth)

# Set toolchain variables
include(arm_none_eabi.cmake)

# Project specific build options
file(GLOB LINKER_FILE src/app.ld)
set(MCPU_FLAGS "-mthumb -mcpu=cortex-m7")
set(VFP_FLAGS "-mfloat-abi=hard -mfpu=fpv5-d16")
set(SPEC_FLAGS "--specs=nosys.specs")
set(LD_FLAGS "") # maybe "-nostartfiles"
set(DBG_FLAGS "-Og -ggdb3 -g -gdwarf-2") # -ggdb3 -g -gdwarf-2 to reveal source files in Ozone, or -Os for release
set(STM32_DEFINES "-DSTM32F7") # STM32 SoC specific variables

set(CMAKE_C_FLAGS   "${MCPU_FLAGS} ${VFP_FLAGS} ${SPEC_FLAGS} ${DBG_FLAGS} -fdata-sections -ffunction-sections -mlong-calls" CACHE INTERNAL "c compiler flags")
set(CMAKE_ASM_FLAGS "${MCPU_FLAGS} ${VFP_FLAGS} -x assembler-with-cpp" CACHE INTERNAL "asm compiler flags")
set(CMAKE_EXE_LINKER_FLAGS "${MCPU_FLAGS} ${LD_FLAGS} -T ${LINKER_FILE} -Wl,--gc-sections -Wl,-Map=output.map -lm -lc" CACHE INTERNAL "exe link flags")

# Only build for arm-none-eabi
if (NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "arm-none-eabi")
    message(FATAL_ERROR "Invalid CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

# libopencm3
set(LIBOPENCM3_LIB opencm3_stm32f7) # must be defined before including libopencm3.cmake)
include(lib/libopencm3.cmake)

# enable asm for stm startup.s file
enable_language(ASM)

file(GLOB C_SOURCE
    src/main.c 
    src/sysinit.c 
    src/arch/sys_arch.c
    src/ip/ip_thread.c
    src/periph/eth.c
    src/periph/clock.c
    src/periph/heater.c
    src/rttp/rtctlsrv.c
    src/rttp/rtmon.c
    src/rttp/rtutil.c
)

set(TRG_INC 
    src/periph
    src/rttp
)

file(GLOB ASM_SOURCE src/arch/tx_initialize_low_level.S)
set_property(SOURCE ${ASM_SOURCE} PROPERTY LANGUAGE ASM)

set_source_files_properties(${C_SOURCE}
    PROPERTIES COMPILE_FLAGS ${STM32_DEFINES}
)

add_executable(${PROJECT_NAME}
    ${C_SOURCE}
    ${ASM_SOURCE}
    ${EXTERNAL_EXECUTABLES}
)

include_directories(src/arch src/ip) # need before lwip included

# ThreadX internal CMake, must be added after add_executable()
# ${PROJECT_NAME} must be the same with the target executable added by add_executable()
add_subdirectory(lib/threadx/common)
add_subdirectory(lib/threadx/ports/cortex_m7/gnu)


# lwip
set(LWIP_DIR lib/lwip)
set (LWIP_INCLUDE_DIRS
    ${LWIP_DIR}/src/include
    src
    lib/threadx/common/inc
    lib/threadx/ports/cortex_m7/gnu/inc
)
include(${LWIP_DIR}/src/Filelists.cmake)
set(EXTERNAL_EXECUTABLES 
    ${EXTERNAL_EXECUTABLES} 
    ${lwipcore_SRCS} 
    ${lwipcore4_SRCS}
    ${lwipcore6_SRCS}
)
include_directories(${LWIP_INCLUDE_DIRS})
set(EXTERNAL_LIBS ${EXTERNAL_LIBS} lwipcore)

target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${TRG_INC})

if (EXTERNAL_DEPENDENCIES)
    add_dependencies(${PROJECT_ROOT_NAME}.elf
        ${EXTERNAL_DEPENDENCIES}
    )
endif()

if (EXTERNAL_LIBS)
    target_link_libraries(${PROJECT_ROOT_NAME}.elf
        ${EXTERNAL_LIBS}
    )
endif()

add_custom_target ( ${PROJECT_ROOT_NAME}.hex ALL
    DEPENDS ${PROJECT_ROOT_NAME}.elf 
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${PROJECT_ROOT_NAME}.elf ${PROJECT_ROOT_NAME}.hex
    COMMENT "Generating ${PROJECT_ROOT_NAME}.hex"
)
add_custom_target ( ${PROJECT_ROOT_NAME}.bin ALL
    DEPENDS ${PROJECT_ROOT_NAME}.elf 
    COMMAND ${CMAKE_OBJCOPY} -O binary ${PROJECT_ROOT_NAME}.elf ${PROJECT_ROOT_NAME}.bin
    COMMENT "Generating ${PROJECT_ROOT_NAME}.bin"
)

add_custom_command(TARGET ${PROJECT_ROOT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_SIZE} --format=berkeley "${PROJECT_ROOT_NAME}.elf")

set(CMAKE_SKIP_INSTALL_RULES True)

