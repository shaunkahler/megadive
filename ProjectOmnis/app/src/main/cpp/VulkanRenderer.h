#pragma once

#include <vector>
#include <jni.h>
#include <android/log.h>
#include "Math.h"
#include "UIManager.h"

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
    
    // Hands
    void RenderHands(const XrHandJointLocationEXT* leftJoints, bool leftActive, 
                     const XrHandJointLocationEXT* rightJoints, bool rightActive,
                     const struct Matrix4x4& viewProj);

    // Menus
    void RenderMenuButtons(const std::vector<MenuButton>& buttons, const struct Matrix4x4& viewProj);

    XrGraphicsBindingVulkanKHR GetVulkanBinding() const;
    void SetupCommandBuffers();
    void ClearImage(VkImage image, VkClearColorValue color);

    void BeginRender(VkImage image, uint32_t width, uint32_t height);
    void EndRender();

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

    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;

    VkImageView m_currentImageView = VK_NULL_HANDLE;
    VkFramebuffer m_currentFramebuffer = VK_NULL_HANDLE;

    void CreateInstance();
    void CreateDevice();
    void SetupRenderPass();
    void BuildPipeline();
    void CreateVertexBuffer();
    void BuildFadeInPipeline();
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};
