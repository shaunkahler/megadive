cmake_minimum_required(VERSION 3.22.1)
project(OmnisEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Android Native App Glue
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
add_library(native_app_glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
target_include_directories(native_app_glue PUBLIC ${ANDROID_NDK}/sources/android/native_app_glue)

# Target Library
add_library(omnis_engine SHARED main.cpp)

# Libraries to link (OpenXR and Vulkan need to be provided by Meta XR SDK)
target_link_libraries(omnis_engine
    native_app_glue
    android
    log
    EGL
    GLESv3
    # openxr_loader # Requires Meta OpenXR Mobile SDK
    # vulkan        # Requires Android NDK Vulkan headers
)
