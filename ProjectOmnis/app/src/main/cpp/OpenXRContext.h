#pragma once

#include "VulkanRenderer.h"
#include "UIManager.h"
#include <android_native_app_glue.h>
#include <chrono>

class OpenXRContext {
public:
    OpenXRContext(struct android_app* app);
    ~OpenXRContext();

    void Initialize();
    bool ProcessFrame();

private:
    struct android_app* m_app;
    VulkanRenderer m_vulkan;
    UIManager m_uiManager;

    // OpenXR Core Objects (Stubbed for architecture)
    // XrInstance m_instance;
    // XrSession m_session;
    // XrSystemId m_systemId;
    XrSpace m_headSpace = 1;  // Stubbed ID
    XrSpace m_worldSpace = 2; // Stubbed ID

    bool m_sessionRunning = false;
    
    // Fade-In tracking
    std::chrono::high_resolution_clock::time_point m_startTime;
    float m_fadeDurationSeconds = 3.0f;

    void CreateInstance();
    void InitializeSystem();
    void CreateSession();
    void SetupSpaces();
};
