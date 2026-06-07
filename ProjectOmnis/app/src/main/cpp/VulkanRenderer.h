#pragma once

#include <vector>
#include <jni.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include "Math.h"
#include "UIManager.h"
#include "stb_truetype.h"

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

    void Initialize(XrInstance xrInstance, XrSystemId systemId, class AAssetManager* assetManager);
    void RenderFrame(float deltaTime, float fadeAlpha);
    
    // Hands
    void RenderHands(const XrHandJointLocationEXT* leftJoints, bool leftActive, 
                     const XrHandJointLocationEXT* rightJoints, bool rightActive,
                     const struct Matrix4x4& viewProj);
    void RenderMenuButtons(const std::vector<MenuButton>& buttons, const struct Matrix4x4& viewProj);
    void RenderLaser(const float origin[3], const float dir[3], const float color[4], const struct Matrix4x4& viewProj);
    void RenderSonic(const struct Matrix4x4& viewProj);

    XrGraphicsBindingVulkanKHR GetVulkanBinding() const;
    void SetupCommandBuffers();
    void ClearImage(VkImage image, VkClearColorValue color);

    void BeginRender(VkImage image, uint32_t width, uint32_t height);
    void EndRender();

private:
    AAssetManager* m_assetManager = nullptr;
    XrInstance m_xrInstance = XR_NULL_HANDLE;
    XrSystemId m_systemId = XR_NULL_SYSTEM_ID;

    // Vulkan Core Objects
    VkInstance m_vkInstance = VK_NULL_HANDLE;
    VkPhysicalDevice m_vkPhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_vkDevice = VK_NULL_HANDLE;
    VkQueue m_vkQueue = VK_NULL_HANDLE;
    uint32_t m_queueFamilyIndex = 0;

    VkImage m_depthImage = VK_NULL_HANDLE;
    VkDeviceMemory m_depthImageMemory = VK_NULL_HANDLE;
    VkImageView m_depthImageView = VK_NULL_HANDLE;
    uint32_t m_depthWidth = 0;
    uint32_t m_depthHeight = 0;

    VkCommandPool m_vkCommandPool = VK_NULL_HANDLE;
    VkCommandBuffer m_vkCommandBuffer = VK_NULL_HANDLE;

    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;

    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_indexBufferMemory = VK_NULL_HANDLE;
    uint32_t m_indexCount = 0;

    VkBuffer m_sonicVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_sonicVertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer m_sonicIndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_sonicIndexBufferMemory = VK_NULL_HANDLE;
    uint32_t m_sonicIndexCount = 0;

    VkBuffer m_redVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_redVertexBufferMemory = VK_NULL_HANDLE;

    VkImageView m_currentImageView = VK_NULL_HANDLE;
    VkFramebuffer m_currentFramebuffer = VK_NULL_HANDLE;

    // UI Rendering
    VkPipelineLayout m_uiPipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_uiPipeline = VK_NULL_HANDLE;
    
    VkDescriptorSetLayout m_uiDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_uiDescriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_uiDescriptorSet = VK_NULL_HANDLE;
    
    VkImage m_fontImage = VK_NULL_HANDLE;
    VkDeviceMemory m_fontImageMemory = VK_NULL_HANDLE;
    VkImageView m_fontImageView = VK_NULL_HANDLE;
    VkSampler m_fontSampler = VK_NULL_HANDLE;
    
    stbtt_bakedchar m_cdata[96]; // ASCII 32..126 is 95 chars
    
    VkBuffer m_uiVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_uiVertexBufferMemory = VK_NULL_HANDLE;

    void CreateInstance();
    void CreateDevice();
    void SetupRenderPass();
    void BuildPipeline();
    void BuildUIPipeline();
    void LoadFont();
    void CreateVertexBuffer();
    void BuildFadeInPipeline();
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};
