#include "UIManager.h"
#include <android/log.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "OmnisEngine", __VA_ARGS__))

UIManager::UIManager() : m_headSpace(0), m_worldSpace(0), m_hudSwapchain(0) {}

UIManager::~UIManager() {}

void UIManager::Initialize(XrSpace headSpace, XrSpace worldSpace) {
    LOGI("Initializing Spatial UI / HUD Manager...");
    m_headSpace = headSpace;
    m_worldSpace = worldSpace;
    
    CreateHUD();
}

void UIManager::CreateHUD() {
    LOGI("Constructing Always-On Visor HUD Composition Layer...");
    // The HUD is locked 0.5 meters directly in front of the user's face.
    // By using m_headSpace, it perfectly tracks with the head hardware, bypassing Vulkan rendering lag.
    m_hudPose.position = {0.0f, 0.0f, -0.5f}; 
    m_hudPose.orientation = {0.0f, 0.0f, 0.0f, 1.0f}; // Identity rotation
    m_hudSize = {0.8f, 0.4f}; // 80cm wide, 40cm tall

    // TODO: xrCreateSwapchain for m_hudSwapchain
}

uint32_t UIManager::SpawnFloatingScreen(XrPosef initialPose, float width, float height) {
    LOGI("Spawning new Infinite Multi-Screen (ID: %d)", m_nextScreenId);
    
    VirtualScreen screen;
    screen.id = m_nextScreenId++;
    screen.pose = initialPose;
    screen.size = {width, height};
    screen.isVisible = true;
    
    // TODO: xrCreateSwapchain to map an Android SurfaceTexture or Chromium Webview here
    screen.swapchain = 0; 

    m_virtualScreens.push_back(screen);
    return screen.id;
}

void UIManager::MoveScreen(uint32_t screenId, XrPosef newPose) {
    for (auto& screen : m_virtualScreens) {
        if (screen.id == screenId) {
            screen.pose = newPose;
            break;
        }
    }
}

void UIManager::Update(float deltaTime) {
    // Logic to animate UI elements, handle drag-and-drop physics for floating screens, etc.
}

void* UIManager::GetCompositionLayersStub() {
    // In a full implementation, this constructs:
    // 1. XrCompositionLayerQuad for the Visor HUD (using m_headSpace)
    // 2. XrCompositionLayerQuad for EVERY floating screen (using m_worldSpace)
    // 
    // Example construction per layer:
    // XrCompositionLayerQuad quad{XR_TYPE_COMPOSITION_LAYER_QUAD};
    // quad.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
    // quad.space = (isHud) ? m_headSpace : m_worldSpace;
    // quad.pose = screen.pose;
    // quad.size = screen.size;
    // quad.subImage.swapchain = screen.swapchain;

    return nullptr; // Return vector of XrCompositionLayerBaseHeader*
}
