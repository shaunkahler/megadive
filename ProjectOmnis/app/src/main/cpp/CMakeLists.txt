cmake_minimum_required(VERSION 3.22.1)
project(OmnisEngine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Android Native App Glue
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")
add_library(native_app_glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
target_include_directories(native_app_glue PUBLIC ${ANDROID_NDK}/sources/android/native_app_glue)

# Target Library
add_library(omnis_engine SHARED 
    main.cpp
    OpenXRContext.cpp
    VulkanRenderer.cpp
    UIManager.cpp
    WorldManager.cpp
    NetworkClient.cpp
    VoiceInterface.cpp
    ModelLoader.cpp
)

# Target Include Directories
target_include_directories(omnis_engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Find Vulkan in the NDK
find_package(Vulkan REQUIRED)

# Find OpenXR and Horizon OS SDK via Android Prefab
find_package(OpenXR REQUIRED CONFIG)
find_package(horizon-os-nsdk REQUIRED CONFIG)

target_link_libraries(omnis_engine
    native_app_glue
    android
    log
    EGL
    GLESv3
    Vulkan::Vulkan
    OpenXR::openxr_loader
    horizon-os-nsdk::hzos
)
