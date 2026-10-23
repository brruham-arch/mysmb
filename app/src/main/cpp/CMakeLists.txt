cmake_minimum_required(VERSION 3.22.1)
project(smbcore C)
get_filename_component(ROOT "${CMAKE_SOURCE_DIR}/../../../.." ABSOLUTE)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -std=c99 -ffunction-sections -fdata-sections")
set(CMAKE_C_FLAGS_DEBUG "-O3")
set(CMAKE_C_FLAGS_RELEASE "-O3")
include_directories(${ROOT}/codegen ${ROOT}/codegen/game ${ROOT}/codegen/lib)
add_library(smbcore SHARED
    jni_glue.c
    ${ROOT}/codegen/game/code.c
    ${ROOT}/codegen/game/data.c
    ${ROOT}/codegen/lib/cpu.c
    ${ROOT}/codegen/lib/video.c
    ${ROOT}/codegen/lib/render.c
    ${ROOT}/codegen/native/display.c
    ${ROOT}/codegen/lib/apu.c
    ${ROOT}/codegen/lib/sound.c
    ${ROOT}/codegen/lib/state.c
    ${ROOT}/codegen/lib/debug_chr.c
    ${ROOT}/codegen/lib/common.c)
target_link_libraries(smbcore log m)
