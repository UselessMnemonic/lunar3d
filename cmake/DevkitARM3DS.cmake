set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR armv6k)

if(NOT DEFINED ENV{DEVKITARM})
    message(FATAL_ERROR "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif()

set(DEVKITARM $ENV{DEVKITARM} CACHE PATH "Path to devkitARM")
set(DEVKITPRO $ENV{DEVKITPRO} CACHE PATH "Path to devkitPro")
if(NOT DEVKITPRO)
    get_filename_component(DEVKITPRO ${DEVKITARM}/.. ABSOLUTE)
    set(DEVKITPRO ${DEVKITPRO} CACHE PATH "Path to devkitPro" FORCE)
endif()

set(CMAKE_C_COMPILER ${DEVKITARM}/bin/arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER ${DEVKITARM}/bin/arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER ${DEVKITARM}/bin/arm-none-eabi-gcc)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

find_program(BIN2S bin2s HINTS ${DEVKITARM}/bin REQUIRED)
find_program(PICASSO picasso HINTS ${DEVKITPRO}/tools/bin REQUIRED)
find_program(THREEDSXTOOL 3dsxtool HINTS ${DEVKITPRO}/tools/bin REQUIRED)
find_program(SMDHTOOL smdhtool HINTS ${DEVKITPRO}/tools/bin REQUIRED)
find_program(BANNERTOOL bannertool HINTS ${DEVKITPRO}/tools/bin REQUIRED)
find_program(MAKEROM makerom HINTS ${DEVKITPRO}/tools/bin REQUIRED)
find_program(TEX3DS tex3ds HINTS ${DEVKITPRO}/tools/bin REQUIRED)
