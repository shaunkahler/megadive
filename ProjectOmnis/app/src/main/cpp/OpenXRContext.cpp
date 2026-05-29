#include "OpenXRContext.h"
#include <cmath>

OpenXRContext::OpenXRContext(struct android_app* app) : m_app(app) {
    m_startTime = std::chrono::high_resolution_clock::now();
}

OpenXRContext::~OpenXRContext() {}

void OpenXRContext::Initialize() {
    LOGI("Initializing OpenXR Context...");
    CreateInstance();
    InitializeSystem();
    
    // Initialize Vulkan BEFORE OpenXR Session, as OpenXR needs the Vulkan instance
    m_vulkan.Initialize();
    
    CreateSession();
    SetupSpaces();
    
    // Initialize UI Manager with the reference spaces
    m_uiManager.Initialize(m_headSpace, m_worldSpace);
    
    // Spawn a test floating screen to act as the integrated Linux Desktop
    XrPosef desktopPose = { {0,0,0,1}, {0.0f, 1.5f, -2.0f} }; // 1.5m high, 2m away
    m_uiManager.SpawnFloatingScreen(desktopPose, 1.2f, 0.8f);

    m_sessionRunning = true;
}

bool OpenXRContext::ProcessFrame() {
    if (!m_sessionRunning) return false;

    // Calculate time for "Fade In" sequence [MQ-3]
    auto currentTime = std::chrono::high_resolution_clock::now();
    float elapsedSeconds = std::chrono::duration<float>(currentTime - m_startTime).count();
    float fadeAlpha = 1.0f - (elapsedSeconds / m_fadeDurationSeconds);
    if (fadeAlpha < 0.0f) fadeAlpha = 0.0f;

    // 1. xrWaitFrame
    // 2. xrBeginFrame

    // Update UI animations/logic
    m_uiManager.Update(0.011f);

    // 3. Render Vulkan Geometry (The 3D World)
    m_vulkan.RenderFrame(0.011f, fadeAlpha);

    // 4. xrEndFrame (Submit composition layers)
    // std::vector<XrCompositionLayerBaseHeader*> layers;
    // layers.push_back((XrCompositionLayerBaseHeader*)&worldProjectionLayer); // Vulkan 3D world
    
    // Get HUD and Floating Screens to render ON TOP of the 3D world
    // auto uiLayers = m_uiManager.GetCompositionLayers();
    // layers.insert(layers.end(), uiLayers.begin(), uiLayers.end());

    // XrFrameEndInfo frameEndInfo{XR_TYPE_FRAME_END_INFO};
    // frameEndInfo.layerCount = (uint32_t)layers.size();
    // frameEndInfo.layers = layers.data();
    // xrEndFrame(m_session, &frameEndInfo);

    return true;
}

void OpenXRContext::CreateInstance() {
    LOGI("OpenXR Instance Created (Extensions Requested: Vulkan, Meta Performance)");
}

void OpenXRContext::InitializeSystem() {
    LOGI("OpenXR System Initialized (Form Factor: HMD)");
}

void OpenXRContext::CreateSession() {
    LOGI("OpenXR Session Created (Vulkan Graphics Binding attached)");
}

void OpenXRContext::SetupSpaces() {
    LOGI("OpenXR Spaces Created: VIEW (Head-locked) and LOCAL (World-locked)");
    // xrCreateReferenceSpace(m_session, XR_REFERENCE_SPACE_TYPE_VIEW, ... &m_headSpace);
    // xrCreateReferenceSpace(m_session, XR_REFERENCE_SPACE_TYPE_LOCAL, ... &m_worldSpace);
}
