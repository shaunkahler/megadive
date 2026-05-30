#pragma once

#include "VulkanRenderer.h"
#include "UIManager.h"
#include <android_native_app_glue.h>
#include <chrono>

// Add OpenXR includes
#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_VULKAN
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#ifndef LOGI
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "OmnisEngine", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "OmnisEngine", __VA_ARGS__))
#endif

// Debug Macro for OpenXR
#define XR_CHECK(res) \
    if (XR_FAILED(res)) { \
        LOGE("OpenXR Error: %d at %s:%d", res, __FILE__, __LINE__); \
    }

class OpenXRContext {
public:
    OpenXRContext(struct android_app* app);
    ~OpenXRContext();

    void Initialize();
    void ProcessFrame();

private:
    struct android_app* m_app;
    VulkanRenderer m_vulkan;
    UIManager m_uiManager;

    // OpenXR Core Objects
    XrInstance m_instance = XR_NULL_HANDLE;
    XrSession m_session = XR_NULL_HANDLE;
    XrSystemId m_systemId = XR_NULL_SYSTEM_ID;
    XrSpace m_headSpace = XR_NULL_HANDLE;
    XrSpace m_worldSpace = XR_NULL_HANDLE;
    XrSessionState m_sessionState = XR_SESSION_STATE_UNKNOWN;

    bool m_sessionRunning = false;
    uint32_t m_frameCount = 0; // For debugging output
    
    // Fade-In tracking
    std::chrono::high_resolution_clock::time_point m_startTime;
    float m_fadeDurationSeconds = 3.0f;

    void CreateInstance();
    void InitializeSystem();
    void CreateSession();
    void SetupSpaces();
    void SetupSwapchains();
    void PollEvents();

    struct SwapchainInfo {
        XrSwapchain swapchain;
        std::vector<XrSwapchainImageVulkanKHR> images;
        int32_t width;
        int32_t height;
    };
    std::vector<SwapchainInfo> m_swapchains;
    std::vector<XrView> m_views;
    std::vector<XrViewConfigurationView> m_viewConfigs;
};
