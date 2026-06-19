cmake_minimum_required(VERSION 3.20)

project(lunar3d LANGUAGES C CXX ASM)

if(NOT DEVKITPRO OR NOT DEVKITARM)
    message(FATAL_ERROR "Configure with the devkitARM toolchain file, for example: cmake --preset devkitarm")
endif()

# CMake build description
set(TARGET lunar3d)
set(BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(SOURCE_DIRS
    source
    source/activities
    source/moonlight
    source/task
    source/ui
    source/ui/components
)
set(DATA_DIR data)
set(INCLUDE_DIRS include)
set(GRAPHICS_DIR gfx)
set(RESOURCES_DIR resources)
set(ROMFS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/romfs)
set(GFXBUILD_DIR ${ROMFS_DIR}/gfx)

set(BANNER_AUDIO ${CMAKE_CURRENT_SOURCE_DIR}/${RESOURCES_DIR}/audio)
set(BANNER_IMAGE ${CMAKE_CURRENT_SOURCE_DIR}/${RESOURCES_DIR}/banner)
set(APP_ICON ${CMAKE_CURRENT_SOURCE_DIR}/${RESOURCES_DIR}/icon.png)
set(RSF ${CMAKE_CURRENT_SOURCE_DIR}/${RESOURCES_DIR}/template.rsf)

set(APP_TITLE Lunar3D)
set(APP_DESCRIPTION "Moonlight Client for N3DS")
set(APP_AUTHOR UselessMnemonic)
set(APP_PRODUCT_CODE CTR-P-LUNAR3D)
set(APP_UNIQUE_ID 0x36001)
set(APP_VERSION_MAJOR 0)
set(APP_VERSION_MINOR 1)
set(APP_VERSION_MICRO 0)

set(PORTLIBS ${DEVKITPRO}/portlibs/3ds)
set(CTRULIB ${DEVKITPRO}/libctru)

add_library(lunar3d_3ds_settings INTERFACE)
target_include_directories(lunar3d_3ds_settings SYSTEM INTERFACE
    ${PORTLIBS}/include
    ${CTRULIB}/include
)
target_compile_options(lunar3d_3ds_settings INTERFACE
    -mword-relocations -ffunction-sections
    -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
)
target_compile_definitions(lunar3d_3ds_settings INTERFACE __3DS__)

set(MOONLIGHT_COMMON_C_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/moonlight-common-c)
if(NOT EXISTS ${MOONLIGHT_COMMON_C_DIR}/CMakeLists.txt)
    message(FATAL_ERROR "moonlight-common-c is missing. Run: git submodule update --init --recursive")
endif()

set(BUILD_SHARED_LIBS OFF)
set(USE_MBEDTLS ON)
add_subdirectory(${MOONLIGHT_COMMON_C_DIR} EXCLUDE_FROM_ALL)
target_link_libraries(moonlight-common-c PRIVATE lunar3d_3ds_settings)
if(TARGET enet)
    target_link_libraries(enet PRIVATE lunar3d_3ds_settings)
endif()

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/source/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/source/*.s
)

file(GLOB DATA_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${DATA_DIR}/*.*)
file(GLOB SHADER_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/source/*.v.pica
    ${CMAKE_CURRENT_SOURCE_DIR}/source/*.shlist
)
file(GLOB GRAPHICS_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${GRAPHICS_DIR}/*.t3s)

set(GENERATED_HEADERS)
set(GENERATED_OBJECTS)

foreach(DATA_FILE IN LISTS DATA_FILES)
    get_filename_component(DATA_NAME ${DATA_FILE} NAME)
    string(REPLACE "." "_" DATA_SYMBOL ${DATA_NAME})
    set(DATA_HEADER ${BUILD_DIR}/${DATA_SYMBOL}.h)
    set(DATA_OBJECT ${BUILD_DIR}/${DATA_NAME}.o)
    add_custom_command(
        OUTPUT ${DATA_OBJECT} ${DATA_HEADER}
        COMMAND ${CMAKE_COMMAND} -E echo ${DATA_NAME}
        COMMAND /bin/sh -c "'${BIN2S}' '${DATA_FILE}' | '${CMAKE_ASM_COMPILER}' -o '${DATA_OBJECT}'"
        COMMAND ${CMAKE_COMMAND} -DINPUT=${DATA_NAME} -DOUTPUT=${DATA_HEADER} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateBinaryHeader.cmake
        DEPENDS ${DATA_FILE}
        VERBATIM
    )
    list(APPEND GENERATED_OBJECTS ${DATA_OBJECT})
    list(APPEND GENERATED_HEADERS ${DATA_HEADER})
endforeach()

foreach(SHADER_FILE IN LISTS SHADER_FILES)
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
    string(REGEX REPLACE "\\.(v\\.pica|shlist)$" "" SHADER_BASE ${SHADER_NAME})
    set(SHBIN ${BUILD_DIR}/${SHADER_BASE}.shbin)
    set(SHADER_OBJECT ${BUILD_DIR}/${SHADER_BASE}.shbin.o)
    set(SHADER_HEADER ${BUILD_DIR}/${SHADER_BASE}_shbin.h)

    if(SHADER_FILE MATCHES "\\.shlist$")
        get_filename_component(SHADER_DIR ${SHADER_FILE} DIRECTORY)
        file(STRINGS ${SHADER_FILE} SHADER_LIST_INPUTS)
        list(TRANSFORM SHADER_LIST_INPUTS PREPEND ${SHADER_DIR}/)
        add_custom_command(
            OUTPUT ${SHBIN}
            COMMAND ${PICASSO} -o ${SHBIN} ${SHADER_LIST_INPUTS}
            DEPENDS ${SHADER_FILE} ${SHADER_LIST_INPUTS}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            VERBATIM
        )
    else()
        get_filename_component(SHADER_DIR ${SHADER_FILE} DIRECTORY)
        set(GEOMETRY_SHADER ${SHADER_DIR}/${SHADER_BASE}.g.pica)
        if(EXISTS ${GEOMETRY_SHADER})
            set(SHADER_INPUTS ${SHADER_FILE} ${GEOMETRY_SHADER})
        else()
            set(SHADER_INPUTS ${SHADER_FILE})
        endif()
        add_custom_command(
            OUTPUT ${SHBIN}
            COMMAND ${PICASSO} -o ${SHBIN} ${SHADER_INPUTS}
            DEPENDS ${SHADER_INPUTS}
            VERBATIM
        )
    endif()

    add_custom_command(
        OUTPUT ${SHADER_OBJECT} ${SHADER_HEADER}
        COMMAND /bin/sh -c "'${BIN2S}' '${SHBIN}' | '${CMAKE_ASM_COMPILER}' -o '${SHADER_OBJECT}'"
        COMMAND ${CMAKE_COMMAND} -DINPUT=${SHADER_BASE}.shbin -DOUTPUT=${SHADER_HEADER} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateBinaryHeader.cmake
        DEPENDS ${SHBIN}
        VERBATIM
    )
    list(APPEND GENERATED_OBJECTS ${SHADER_OBJECT})
    list(APPEND GENERATED_HEADERS ${SHADER_HEADER})
endforeach()

foreach(GRAPHICS_FILE IN LISTS GRAPHICS_FILES)
    get_filename_component(GFX_NAME ${GRAPHICS_FILE} NAME_WE)
    set(T3X_FILE ${GFXBUILD_DIR}/${GFX_NAME}.t3x)
    set(T3X_HEADER ${BUILD_DIR}/${GFX_NAME}.h)
    add_custom_command(
        OUTPUT ${T3X_FILE} ${T3X_HEADER}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${GFXBUILD_DIR}
        COMMAND ${TEX3DS} -i ${GRAPHICS_FILE} -H ${T3X_HEADER} -d ${BUILD_DIR}/${GFX_NAME}.d -o ${T3X_FILE}
        DEPENDS ${GRAPHICS_FILE}
        VERBATIM
    )
    list(APPEND GENERATED_HEADERS ${T3X_HEADER})
endforeach()

add_executable(${TARGET}.elf ${PROJECT_SOURCES} ${GENERATED_OBJECTS} ${GENERATED_HEADERS})
set_target_properties(${TARGET}.elf PROPERTIES
    OUTPUT_NAME ${TARGET}
    SUFFIX .elf
)

target_include_directories(${TARGET}.elf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/${INCLUDE_DIRS}
    ${BUILD_DIR}
    ${PORTLIBS}/include
    ${CTRULIB}/include
)

target_compile_options(${TARGET}.elf PRIVATE
    -Wall -O2 -mword-relocations -fomit-frame-pointer -ffunction-sections
    -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
)
target_compile_definitions(${TARGET}.elf PRIVATE __3DS__)
target_compile_options(${TARGET}.elf PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-std=gnu99>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti -fno-exceptions -std=gnu++20>
)
target_link_options(${TARGET}.elf PRIVATE
    -specs=3dsx.specs
    -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
    -Wl,-Map,${TARGET}.map
)
target_link_directories(${TARGET}.elf PRIVATE ${PORTLIBS}/lib ${CTRULIB}/lib)
target_link_libraries(${TARGET}.elf PRIVATE
    lunar3d_3ds_settings moonlight-common-c citro2d citro3d tinyxml2 mbedtls mbedx509 mbedcrypto ctru m
)

set(OUTPUT_FILE ${BUILD_DIR}/${TARGET})
set(SMDH_FILE ${OUTPUT_FILE}.smdh)
set(THREEDSX_FILE ${OUTPUT_FILE}.3dsx)
set(CIA_FILE ${OUTPUT_FILE}.cia)
set(THREEDS_FILE ${OUTPUT_FILE}.3ds)
set(BANNER_FILE ${BUILD_DIR}/banner.bnr)
set(ICON_FILE ${BUILD_DIR}/icon.icn)
set(RELEASE_ROOT ${BUILD_DIR}/release)
set(RELEASE_ZIP ${OUTPUT_FILE}.zip)

add_custom_command(
    OUTPUT ${SMDH_FILE}
    COMMAND ${SMDHTOOL} --create ${APP_TITLE} ${APP_DESCRIPTION} ${APP_AUTHOR} ${APP_ICON} ${SMDH_FILE}
    DEPENDS ${APP_ICON}
    VERBATIM
)

add_custom_command(
    OUTPUT ${THREEDSX_FILE}
    COMMAND ${THREEDSXTOOL} $<TARGET_FILE:${TARGET}.elf> ${THREEDSX_FILE} --smdh=${SMDH_FILE} --romfs=${ROMFS_DIR}
    DEPENDS ${TARGET}.elf ${SMDH_FILE}
    VERBATIM
)
add_custom_target(3dsx ALL DEPENDS ${THREEDSX_FILE})

if(EXISTS ${BANNER_IMAGE}.cgfx)
    set(BANNER_IMAGE_FILE ${BANNER_IMAGE}.cgfx)
    set(BANNER_IMAGE_ARG -ci ${BANNER_IMAGE_FILE})
else()
    set(BANNER_IMAGE_FILE ${BANNER_IMAGE}.png)
    set(BANNER_IMAGE_ARG -i ${BANNER_IMAGE_FILE})
endif()

if(EXISTS ${BANNER_AUDIO}.cwav)
    set(BANNER_AUDIO_FILE ${BANNER_AUDIO}.cwav)
    set(BANNER_AUDIO_ARG -ca ${BANNER_AUDIO_FILE})
else()
    set(BANNER_AUDIO_FILE ${BANNER_AUDIO}.wav)
    set(BANNER_AUDIO_ARG -a ${BANNER_AUDIO_FILE})
endif()

add_custom_command(
    OUTPUT ${BANNER_FILE}
    COMMAND ${BANNERTOOL} makebanner ${BANNER_IMAGE_ARG} ${BANNER_AUDIO_ARG} -o ${BANNER_FILE}
    DEPENDS ${BANNER_IMAGE_FILE} ${BANNER_AUDIO_FILE}
    VERBATIM
)

add_custom_command(
    OUTPUT ${ICON_FILE}
    COMMAND ${BANNERTOOL} makesmdh -s ${APP_TITLE} -l ${APP_TITLE} -p ${APP_AUTHOR} -i ${APP_ICON} -o ${ICON_FILE}
    DEPENDS ${APP_ICON}
    VERBATIM
)

set(COMMON_MAKEROM_PARAMS
    -rsf ${RSF}
    -target t
    -exefslogo
    -elf $<TARGET_FILE:${TARGET}.elf>
    -icon ${ICON_FILE}
    -banner ${BANNER_FILE}
    -DAPP_TITLE=${APP_TITLE}
    -DAPP_PRODUCT_CODE=${APP_PRODUCT_CODE}
    -DAPP_UNIQUE_ID=${APP_UNIQUE_ID}
    -DAPP_ROMFS=${ROMFS_DIR}
    -DAPP_SYSTEM_MODE=64MB
    -DAPP_SYSTEM_MODE_EXT=Legacy
    -major ${APP_VERSION_MAJOR}
    -minor ${APP_VERSION_MINOR}
    -micro ${APP_VERSION_MICRO}
)

add_custom_command(
    OUTPUT ${CIA_FILE}
    COMMAND ${MAKEROM} -f cia -o ${CIA_FILE} -DAPP_ENCRYPTED=false ${COMMON_MAKEROM_PARAMS}
    DEPENDS ${TARGET}.elf ${BANNER_FILE} ${ICON_FILE}
    VERBATIM
)
add_custom_target(cia DEPENDS ${CIA_FILE})

add_custom_command(
    OUTPUT ${THREEDS_FILE}
    COMMAND ${MAKEROM} -f cci -o ${THREEDS_FILE} -DAPP_ENCRYPTED=true ${COMMON_MAKEROM_PARAMS}
    DEPENDS ${TARGET}.elf ${BANNER_FILE} ${ICON_FILE}
    VERBATIM
)
add_custom_target(3ds DEPENDS ${THREEDS_FILE})

add_custom_target(elf DEPENDS ${TARGET}.elf)

add_custom_command(
    OUTPUT ${RELEASE_ZIP}
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${RELEASE_ROOT}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RELEASE_ROOT}/3ds/${TARGET}
    COMMAND ${CMAKE_COMMAND} -E copy ${THREEDSX_FILE} ${RELEASE_ROOT}/3ds/${TARGET}/${TARGET}.3dsx
    COMMAND ${CMAKE_COMMAND} -E copy ${SMDH_FILE} ${RELEASE_ROOT}/3ds/${TARGET}/${TARGET}.smdh
    COMMAND ${CMAKE_COMMAND} -E tar cf ${RELEASE_ZIP} --format=zip 3ds
    WORKING_DIRECTORY ${RELEASE_ROOT}
    DEPENDS ${THREEDSX_FILE} ${SMDH_FILE}
    VERBATIM
)
add_custom_target(release DEPENDS ${RELEASE_ZIP} ${CIA_FILE} ${THREEDS_FILE})
