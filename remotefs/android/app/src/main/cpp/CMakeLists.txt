cmake_minimum_required(VERSION 3.10.2)
project("remotefs")

# Path to the remotefs source directory containing android.c
# This CMakeLists.txt is at: remotefs/android/app/src/main/cpp/
# REMOTEFS_DIR is 5 levels up = remotefs/
# Widget sources are at ../widget/
set(REMOTEFS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../.." CACHE PATH "Path to cooledit/remotefs")

set(WIDGET_DIR "${REMOTEFS_DIR}/../widget")

set(SOURCES
    ${WIDGET_DIR}/remotefs.c
    ${WIDGET_DIR}/aes.c
    ${WIDGET_DIR}/sha256.c
    ${WIDGET_DIR}/symauth.c
    ${WIDGET_DIR}/hashtable.c
    ${WIDGET_DIR}/regtools.c
    ${WIDGET_DIR}/pathdup.c
    ${WIDGET_DIR}/ipv6scan.c
    ${WIDGET_DIR}/fnmatch.c
    ${REMOTEFS_DIR}/android.c
)

add_library(remotefs SHARED ${SOURCES})

target_include_directories(remotefs PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${WIDGET_DIR}
    ${REMOTEFS_DIR}/..
)

target_compile_definitions(remotefs PRIVATE ANDROID)

target_link_libraries(remotefs
    android
    log
)
