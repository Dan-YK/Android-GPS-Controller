project(cpp-test)

cmake_minimum_required(VERSION 3.16.0)

set(CMAKE_CXX_STANDARD 17)

add_compile_options(-fno-rtti)

set(CMAKE_FIND_DEBUG_MODE FALSE)

#find_library(ABCDEFG
#    NAMES binder
#    PATHS "/home/dev/Projects/TurningOnAndroidLocation/lib64"
#    NO_DEFAULT_PATH
#)

include_directories(
    "/home/dev/Projects/AOSP/frameworks/av/camera/ndk/include"
    "/home/dev/Projects/AOSP/frameworks/native/libs/binder/include"
    "/home/dev/Projects/AOSP/system/libbase/include"
    "/home/dev/Projects/AOSP/system/core/libutils/include"
    "/home/dev/Projects/AOSP/system/logging/liblog/include"
    "/home/dev/Projects/AOSP/external/selinux/libselinux/include"
    "/home/dev/Projects/AOSP/system/core/libcutils/include_outside_system"
    "/home/dev/Android/Sdk/ndk/25.1.8937393/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include"
)

add_executable(main main.cpp)
target_link_libraries(main
    "${CMAKE_CURRENT_SOURCE_DIR}/lib64/libbinder.so"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib64/libutils.so"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib64/libselinux.so"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib64/liblog.so"
)
