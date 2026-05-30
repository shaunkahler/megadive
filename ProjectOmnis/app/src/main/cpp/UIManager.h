#pragma once

#include <vector>
#include <string>
#include <cstdint>

// Use official OpenXR headers to ensure type consistency across the project.
// This prevents "typedef redefinition" errors on 64-bit platforms.
#ifndef XR_USE_PLATFORM_ANDROID
#define XR_USE_PLATFORM_ANDROID
#endif
#include <openxr/openxr.h>

struct VirtualScreen {
    uint32_t id;
    XrPosef pose;           // Position and rotation in the 3D world
    XrExtent2Df size;       // Physical width/height in meters
    XrSwapchain swapchain;  // Texture holding the 2D Linux Desktop/Android Surface
    bool isVisible;
};

class UIManager {
public:
    UIManager();
    ~UIManager();

    void Initialize(XrSpace headSpace, XrSpace worldSpace);
    
    // Virtual Display Management [MQ-4]
    uint32_t SpawnFloatingScreen(XrPosef initialPose, float width, float height);
    void MoveScreen(uint32_t screenId, XrPosef newPose);
    
    // Updates the HUD and prepares the OpenXR layers for this frame
    void Update(float deltaTime);

    // Get the prepared composition layers to submit to xrEndFrame
    // std::vector<XrCompositionLayerBaseHeader*> GetCompositionLayers();
    void* GetCompositionLayersStub(); // Stubbed return for compilation without OpenXR

private:
    XrSpace m_headSpace = XR_NULL_HANDLE;
    XrSpace m_worldSpace = XR_NULL_HANDLE;
    
    // The Always-On Visor HUD [MQ-3]
    XrSwapchain m_hudSwapchain = XR_NULL_HANDLE;
    XrPosef m_hudPose;
    XrExtent2Df m_hudSize;

    // The Infinite Multi-Screen Array [MQ-4]
    std::vector<VirtualScreen> m_virtualScreens;
    uint32_t m_nextScreenId = 1;

    void CreateHUD();
};
