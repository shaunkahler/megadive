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

struct MenuButton {
    std::string label;
    std::string description;
    std::string packageName;
    XrPosef pose;
    float size[3]; // width, height, depth
    float color[3];
    bool hovered;
    bool triggered;
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
    void UpdateMenu(float deltaTime, 
                    const XrHandJointLocationEXT* leftJoints, bool leftActive, 
                    const XrHandJointLocationEXT* rightJoints, bool rightActive, 
                    void* androidApp, XrPosef headPose);
    void PositionMenuInFrontOf(XrPosef headPose);

    // Get the prepared composition layers to submit to xrEndFrame
    // std::vector<XrCompositionLayerBaseHeader*> GetCompositionLayers();
    void* GetCompositionLayersStub(); // Stubbed return for compilation without OpenXR

    const std::vector<MenuButton>& GetMenuButtons() const { return m_menuButtons; }
    bool IsMenuVisible() const { return m_menuVisible; }
    bool IsPointing() const { return m_isPointing; }
    const float* GetPointerOrigin() const { return m_pointerOrigin; }
    const float* GetPointerDir() const { return m_pointerDir; }
    const float* GetLaserColor() const { return m_laserColor; }

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

    // Menu Launcher items
    std::vector<MenuButton> m_menuButtons;
    bool m_menuVisible = true;
    bool m_wristTouchedLastFrame = false;
    bool m_rightHandPinchingLastFrame = false;
    bool m_leftHandPinchingLastFrame = false;
    bool m_menuPositionInitialized = false;

    bool m_isPointing = false;
    float m_pointerOrigin[3] = {0.0f, 0.0f, 0.0f};
    float m_pointerDir[3] = {0.0f, 0.0f, -1.0f};

    // Index Flex Click Gesture State
    bool m_indexFlexed = false;
    float m_indexFlexTimer = 0.0f;
    
    // Laser Visual Feedback
    float m_clickVisualTimer = 0.0f;
    float m_laserColor[4] = {1.0f, 0.0f, 0.0f, 1.0f};

    void CreateHUD();
    void InitializeLauncherMenu();
};
