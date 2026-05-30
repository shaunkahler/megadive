#pragma once

#include <vector>
#include <jni.h>
#include <android/log.h>

#define XR_USE_PLATFORM_ANDROID
#define XR_USE_GRAPHICS_API_VULKAN
#include <vulkan/vulkan.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#ifndef LOGI
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "OmnisEngine", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "OmnisEngine", __VA_ARGS__))
#endif

// Debug Macro for Vulkan
#define VK_CHECK(res) \
    if ((res) != VK_SUCCESS) { \
        LOGE("Vulkan Error: %d at %s:%d", res, __FILE__, __LINE__); \
    }

// Debug Macro for OpenXR
#ifndef XR_CHECK
#define XR_CHECK(res) \
    if ((res) != XR_SUCCESS) { \
        LOGE("OpenXR Error: %d", res); \
    }
#endif

class VulkanRenderer {
public:
    VulkanRenderer();
    ~VulkanRenderer();

    void Initialize(XrInstance xrInstance, XrSystemId systemId);
    void RenderFrame(float deltaTime, float fadeAlpha);
    XrGraphicsBindingVulkanKHR GetVulkanBinding() const;
    void SetupCommandBuffers();
    void ClearImage(VkImage image, VkClearColorValue color);

private:
    XrInstance m_xrInstance = XR_NULL_HANDLE;
    XrSystemId m_systemId = XR_NULL_SYSTEM_ID;

    // Vulkan Core Objects
    VkInstance m_vkInstance = VK_NULL_HANDLE;
    VkPhysicalDevice m_vkPhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_vkDevice = VK_NULL_HANDLE;
    VkQueue m_vkQueue = VK_NULL_HANDLE;
    uint32_t m_queueFamilyIndex = 0;
    
    VkCommandPool m_vkCommandPool = VK_NULL_HANDLE;
    VkCommandBuffer m_vkCommandBuffer = VK_NULL_HANDLE;

    void CreateInstance();
    void CreateDevice();
    void SetupRenderPass();
    void BuildFadeInPipeline();
};
