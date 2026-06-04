#include "OpenXRContext.h"
#include "Math.h"
#include <cmath>
#include <cstring>
#include <vector>

OpenXRContext::OpenXRContext(struct android_app* app) : m_app(app) {
    m_startTime = std::chrono::high_resolution_clock::now();
}

OpenXRContext::~OpenXRContext() {}

void OpenXRContext::Initialize() {
    LOGI("Initializing OpenXR Context...");
    CreateInstance();
    InitializeSystem();
    
    // Initialize Vulkan BEFORE OpenXR Session, as OpenXR needs the Vulkan instance
    m_vulkan.Initialize(m_instance, m_systemId, m_app->activity->assetManager);
    
    CreateSession();
    SetupSpaces();
    InitializeHandTracking();
    SetupSwapchains();
    
    // Initialize UI Manager with the reference spaces
    m_uiManager.Initialize(m_headSpace, m_worldSpace);
    
      // Spawn a test floating screen to act as the integrated Linux Desktop
      XrPosef desktopPose = { {0,0,0,1}, {0.0f, 0.0f, -2.0f} }; // Eye level, 2m away
      m_uiManager.SpawnFloatingScreen(desktopPose, 1.2f, 0.8f);
}

void OpenXRContext::CreateInstance() {
    LOGI("Creating OpenXR Instance...");
    
    // Initialize OpenXR Loader for Android
    PFN_xrInitializeLoaderKHR xrInitializeLoaderKHR;
    xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR", (PFN_xrVoidFunction*)&xrInitializeLoaderKHR);
    if (xrInitializeLoaderKHR != nullptr) {
        XrLoaderInitInfoAndroidKHR loaderInitInfo = {XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};
        loaderInitInfo.applicationVM = m_app->activity->vm;
        loaderInitInfo.applicationContext = m_app->activity->clazz;
        xrInitializeLoaderKHR((XrLoaderInitInfoBaseHeaderKHR*)&loaderInitInfo);
        LOGI("xrInitializeLoaderKHR called successfully.");
    } else {
        LOGE("Failed to get xrInitializeLoaderKHR function pointer!");
    }

    // Setup Android-specific instance create info
    XrInstanceCreateInfoAndroidKHR androidCreateInfo = {XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR};
    androidCreateInfo.applicationVM = m_app->activity->vm;
    androidCreateInfo.applicationActivity = m_app->activity->clazz;

    XrInstanceCreateInfo createInfo = {XR_TYPE_INSTANCE_CREATE_INFO};
    createInfo.next = &androidCreateInfo;
    
    // Application info
    strncpy(createInfo.applicationInfo.applicationName, "MegaDive OS", XR_MAX_APPLICATION_NAME_SIZE);
    createInfo.applicationInfo.applicationVersion = 1;
    strncpy(createInfo.applicationInfo.engineName, "Omnis Engine", XR_MAX_ENGINE_NAME_SIZE);
    createInfo.applicationInfo.engineVersion = 1;
    createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;

    // We must request the Android platform and Vulkan extensions
    const char* extensions[] = {
        "XR_KHR_android_create_instance",
        "XR_KHR_vulkan_enable2",
        XR_EXT_HAND_TRACKING_EXTENSION_NAME
    };
    createInfo.enabledExtensionCount = 3;
    createInfo.enabledExtensionNames = extensions;
    
    LOGI("Requesting OpenXR Extensions:");
    for(int i = 0; i < 2; i++) LOGI("  - %s", extensions[i]);

    XR_CHECK(xrCreateInstance(&createInfo, &m_instance));
    LOGI("OpenXR Instance created successfully!");
}

void OpenXRContext::InitializeSystem() {
    LOGI("Initializing OpenXR System...");
    XrSystemGetInfo systemInfo = {XR_TYPE_SYSTEM_GET_INFO};
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XR_CHECK(xrGetSystem(m_instance, &systemInfo, &m_systemId));
    LOGI("OpenXR System initialized! SystemID: %llu", (unsigned long long)m_systemId);
}

void OpenXRContext::CreateSession() {
    LOGI("Creating OpenXR Session...");
    
    // Get the fully initialized Vulkan binding from the renderer
    XrGraphicsBindingVulkanKHR graphicsBinding = m_vulkan.GetVulkanBinding();
    
    XrSessionCreateInfo createInfo = {XR_TYPE_SESSION_CREATE_INFO};
    createInfo.next = &graphicsBinding;
    createInfo.systemId = m_systemId;

    XR_CHECK(xrCreateSession(m_instance, &createInfo, &m_session));
    LOGI("OpenXR Session created successfully!");
}

void OpenXRContext::SetupSpaces() {
    LOGI("Setting up Reference Spaces...");
    XrReferenceSpaceCreateInfo playSpaceInfo = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    playSpaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    playSpaceInfo.poseInReferenceSpace.orientation.w = 1.0f;
    XR_CHECK(xrCreateReferenceSpace(m_session, &playSpaceInfo, &m_worldSpace));

    XrReferenceSpaceCreateInfo headSpaceInfo = {XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
    headSpaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    headSpaceInfo.poseInReferenceSpace.orientation.w = 1.0f;
    XR_CHECK(xrCreateReferenceSpace(m_session, &headSpaceInfo, &m_headSpace));
    LOGI("Reference Spaces (Local and View) created.");
}

void OpenXRContext::SetupSwapchains() {
    LOGI("Setting up Swapchains...");
    uint32_t viewConfigCount;
    XR_CHECK(xrEnumerateViewConfigurationViews(m_instance, m_systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &viewConfigCount, nullptr));
    m_viewConfigs.resize(viewConfigCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
    XR_CHECK(xrEnumerateViewConfigurationViews(m_instance, m_systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, viewConfigCount, &viewConfigCount, m_viewConfigs.data()));

    m_views.resize(viewConfigCount, {XR_TYPE_VIEW});

    for (uint32_t i = 0; i < viewConfigCount; i++) {
        XrSwapchainCreateInfo createInfo = {XR_TYPE_SWAPCHAIN_CREATE_INFO};
        createInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.format = VK_FORMAT_R8G8B8A8_SRGB; // Standard for Meta Quest
        createInfo.sampleCount = 1;
        createInfo.width = m_viewConfigs[i].recommendedImageRectWidth;
        createInfo.height = m_viewConfigs[i].recommendedImageRectHeight;
        createInfo.faceCount = 1;
        createInfo.arraySize = 1;
        createInfo.mipCount = 1;
        
        SwapchainInfo info;
        info.width = createInfo.width;
        info.height = createInfo.height;
        XR_CHECK(xrCreateSwapchain(m_session, &createInfo, &info.swapchain));

        uint32_t imageCount;
        XR_CHECK(xrEnumerateSwapchainImages(info.swapchain, 0, &imageCount, nullptr));
        info.images.resize(imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR});
        XR_CHECK(xrEnumerateSwapchainImages(info.swapchain, imageCount, &imageCount, (XrSwapchainImageBaseHeader*)info.images.data()));

        m_swapchains.push_back(info);
    }
    LOGI("Swapchains created for %d views.", viewConfigCount);
}

void OpenXRContext::PollEvents() {
    XrEventDataBuffer event = {XR_TYPE_EVENT_DATA_BUFFER};
    while (xrPollEvent(m_instance, &event) == XR_SUCCESS) {
        if (event.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
            XrEventDataSessionStateChanged* sessionStateChanged = (XrEventDataSessionStateChanged*)&event;
            m_sessionState = sessionStateChanged->state;
            
            LOGI("OpenXR Session State Changed to: %d", m_sessionState);

            if (m_sessionState == XR_SESSION_STATE_READY) {
                LOGI("OpenXR state is READY. Calling xrBeginSession...");
                XrSessionBeginInfo sessionBeginInfo = {XR_TYPE_SESSION_BEGIN_INFO};
                sessionBeginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                XR_CHECK(xrBeginSession(m_session, &sessionBeginInfo));
                m_sessionRunning = true;
                LOGI("OpenXR Session Begun.");
            } else if (m_sessionState == XR_SESSION_STATE_STOPPING) {
                LOGI("OpenXR state is STOPPING. Calling xrEndSession...");
                m_sessionRunning = false;
                XR_CHECK(xrEndSession(m_session));
                LOGI("OpenXR Session Ended.");
            }
        }
        event.type = XR_TYPE_EVENT_DATA_BUFFER; // Reset for next poll
    }
}

void OpenXRContext::ProcessFrame() {
    PollEvents();

    if (!m_sessionRunning) {
        return; // Still waiting for the headset to put us into READY state
    }

    // 1. xrWaitFrame (Ask the OS to predict head tracking for the next display refresh)
    XrFrameWaitInfo frameWaitInfo = {XR_TYPE_FRAME_WAIT_INFO};
    XrFrameState frameState = {XR_TYPE_FRAME_STATE};
    XR_CHECK(xrWaitFrame(m_session, &frameWaitInfo, &frameState));

    // 2. xrBeginFrame
    XrFrameBeginInfo frameBeginInfo = {XR_TYPE_FRAME_BEGIN_INFO};
    XR_CHECK(xrBeginFrame(m_session, &frameBeginInfo));

    std::vector<XrCompositionLayerBaseHeader*> layers;
    XrCompositionLayerProjection projectionLayer = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
    std::vector<XrCompositionLayerProjectionView> projectionViews(m_views.size());

    if (frameState.shouldRender == XR_TRUE) {
        if (m_frameCount == 0) {
            LOGI("Rendering FIRST OpenXR Frame!");
        }

        // Calculate time for "Fade In" sequence
        auto currentTime = std::chrono::high_resolution_clock::now();
        float elapsedSeconds = std::chrono::duration<float>(currentTime - m_startTime).count();
        float fadeAlpha = 1.0f - (elapsedSeconds / m_fadeDurationSeconds);
        if (fadeAlpha < 0.0f) fadeAlpha = 0.0f;

        LocateHands(frameState.predictedDisplayTime);

        // Locate views
        XrViewLocateInfo viewLocateInfo = {XR_TYPE_VIEW_LOCATE_INFO};
        viewLocateInfo.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        viewLocateInfo.displayTime = frameState.predictedDisplayTime;
        viewLocateInfo.space = m_worldSpace;
        
        XrViewState viewState = {XR_TYPE_VIEW_STATE};
        uint32_t viewCount;
        XR_CHECK(xrLocateViews(m_session, &viewLocateInfo, &viewState, m_views.size(), &viewCount, m_views.data()));

        for (uint32_t i = 0; i < viewCount; i++) {
            XrSwapchainImageAcquireInfo acquireInfo = {XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
            uint32_t imageIndex;
            XR_CHECK(xrAcquireSwapchainImage(m_swapchains[i].swapchain, &acquireInfo, &imageIndex));

            XrSwapchainImageWaitInfo waitInfo = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
            waitInfo.timeout = XR_INFINITE_DURATION;
            XR_CHECK(xrWaitSwapchainImage(m_swapchains[i].swapchain, &waitInfo));

            // Clear to rapid random colors (SEIZURE MODE)
            float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
            float g = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
            float b = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
            VkClearColorValue color = VkClearColorValue{{r, g, b, 1.0f}};
            m_vulkan.ClearImage(m_swapchains[i].images[imageIndex].image, color);

            m_vulkan.BeginRender(m_swapchains[i].images[imageIndex].image, m_swapchains[i].width, m_swapchains[i].height);
            
            Matrix4x4 view, proj, viewProj;
            CreateViewMatrix(&view, m_views[i].pose);
            CreateProjectionMatrix(&proj, m_views[i].fov, 0.05f, 100.0f);
            Matrix4x4_Multiply(&viewProj, &proj, &view);
            
            m_vulkan.RenderHands(m_leftHandJoints, m_leftHandActive, m_rightHandJoints, m_rightHandActive, viewProj);
            if (m_uiManager.IsMenuVisible()) {
                m_vulkan.RenderMenuButtons(m_uiManager.GetMenuButtons(), viewProj);
            }
            if (m_uiManager.IsPointing()) {
                m_vulkan.RenderLaser(m_uiManager.GetPointerOrigin(), m_uiManager.GetPointerDir(), m_uiManager.GetLaserColor(), viewProj);
            }
            
            m_vulkan.EndRender();

            XrSwapchainImageReleaseInfo releaseInfo = {XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
            XR_CHECK(xrReleaseSwapchainImage(m_swapchains[i].swapchain, &releaseInfo));

            projectionViews[i].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
            projectionViews[i].next = nullptr;
            projectionViews[i].pose = m_views[i].pose;
            projectionViews[i].fov = m_views[i].fov;
            projectionViews[i].subImage.swapchain = m_swapchains[i].swapchain;
            projectionViews[i].subImage.imageRect.offset = {0, 0};
            projectionViews[i].subImage.imageRect.extent = {m_swapchains[i].width, m_swapchains[i].height};
            projectionViews[i].subImage.imageArrayIndex = 0;
        }

        projectionLayer.space = m_worldSpace;
        projectionLayer.viewCount = viewCount;
        projectionLayer.views = projectionViews.data();
        layers.push_back((XrCompositionLayerBaseHeader*)&projectionLayer);

        // Update UI animations/logic
        m_uiManager.Update(0.011f);
        
        m_uiManager.UpdateMenu(0.011f, m_leftHandJoints, m_leftHandActive, m_rightHandJoints, m_rightHandActive, m_app, m_views[0].pose);

        m_vulkan.RenderFrame(0.011f, fadeAlpha);
// RenderHands moved inside view loop
        m_frameCount++;
    }

    // 4. xrEndFrame (Submit composition layers to the OS Compositor)
    XrFrameEndInfo frameEndInfo = {XR_TYPE_FRAME_END_INFO};
    frameEndInfo.displayTime = frameState.predictedDisplayTime;
    frameEndInfo.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    frameEndInfo.layerCount = (uint32_t)layers.size();
    frameEndInfo.layers = layers.data();
    
    XR_CHECK(xrEndFrame(m_session, &frameEndInfo));
}

void OpenXRContext::InitializeHandTracking() {
    LOGI("Initializing OpenXR Hand Tracking...");
    
    // Get function pointers
    xrGetInstanceProcAddr(m_instance, "xrCreateHandTrackerEXT", (PFN_xrVoidFunction*)&pfnCreateHandTrackerEXT);
    xrGetInstanceProcAddr(m_instance, "xrDestroyHandTrackerEXT", (PFN_xrVoidFunction*)&pfnDestroyHandTrackerEXT);
    xrGetInstanceProcAddr(m_instance, "xrLocateHandJointsEXT", (PFN_xrVoidFunction*)&pfnLocateHandJointsEXT);

    if (!pfnCreateHandTrackerEXT || !pfnLocateHandJointsEXT) {
        LOGE("Hand tracking extensions not available!");
        return;
    }

    XrHandTrackerCreateInfoEXT leftInfo = {XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT};
    leftInfo.hand = XR_HAND_LEFT_EXT;
    leftInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
    XR_CHECK(pfnCreateHandTrackerEXT(m_session, &leftInfo, &m_handTrackerLeft));

    XrHandTrackerCreateInfoEXT rightInfo = {XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT};
    rightInfo.hand = XR_HAND_RIGHT_EXT;
    rightInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
    XR_CHECK(pfnCreateHandTrackerEXT(m_session, &rightInfo, &m_handTrackerRight));

    LOGI("Hand trackers created successfully.");
}

void OpenXRContext::LocateHands(XrTime predictedDisplayTime) {
    if (!pfnLocateHandJointsEXT) return;

    XrHandJointsLocateInfoEXT locateInfo = {XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT};
    locateInfo.baseSpace = m_worldSpace;
    locateInfo.time = predictedDisplayTime;

    // Left Hand
    XrHandJointLocationsEXT leftLocations = {XR_TYPE_HAND_JOINT_LOCATIONS_EXT};
    leftLocations.jointCount = XR_HAND_JOINT_COUNT_EXT;
    leftLocations.jointLocations = m_leftHandJoints;
    if (m_handTrackerLeft) {
        pfnLocateHandJointsEXT(m_handTrackerLeft, &locateInfo, &leftLocations);
        m_leftHandActive = leftLocations.isActive == XR_TRUE;
    }

    // Right Hand
    XrHandJointLocationsEXT rightLocations = {XR_TYPE_HAND_JOINT_LOCATIONS_EXT};
    rightLocations.jointCount = XR_HAND_JOINT_COUNT_EXT;
    rightLocations.jointLocations = m_rightHandJoints;
    if (m_handTrackerRight) {
        pfnLocateHandJointsEXT(m_handTrackerRight, &locateInfo, &rightLocations);
        m_rightHandActive = rightLocations.isActive == XR_TRUE;
    }
}

