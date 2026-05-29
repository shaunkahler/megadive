#pragma once

#include <vector>
#include <android/log.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "OmnisEngine", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "OmnisEngine", __VA_ARGS__))

class VulkanRenderer {
public:
    VulkanRenderer();
    ~VulkanRenderer();

    void Initialize();
    void RenderFrame(float deltaTime, float fadeAlpha);

private:
    // Vulkan Core Objects (Stubbed for architecture)
    // VkInstance instance;
    // VkDevice device;
    // VkRenderPass renderPass;
    // VkPipeline graphicsPipeline;

    void CreateInstance();
    void CreateDevice();
    void SetupRenderPass();
    void BuildFadeInPipeline();
};
