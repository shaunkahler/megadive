#include "VulkanRenderer.h"
#include <stdexcept>
#include <string>
#include <cstring>

// OpenXR Extension function pointers for XR_KHR_vulkan_enable2
PFN_xrGetVulkanGraphicsRequirements2KHR pfnGetVulkanGraphicsRequirements2KHR = nullptr;
PFN_xrCreateVulkanInstanceKHR pfnCreateVulkanInstanceKHR = nullptr;
PFN_xrGetVulkanGraphicsDevice2KHR pfnGetVulkanGraphicsDevice2KHR = nullptr;
PFN_xrCreateVulkanDeviceKHR pfnCreateVulkanDeviceKHR = nullptr;

VulkanRenderer::VulkanRenderer() {}

VulkanRenderer::~VulkanRenderer() {
    LOGI("VulkanRenderer Destroyed");
    if (m_vkDevice != VK_NULL_HANDLE) {
        vkDestroyDevice(m_vkDevice, nullptr);
    }
    if (m_vkInstance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_vkInstance, nullptr);
    }
}

void VulkanRenderer::Initialize(XrInstance xrInstance, XrSystemId systemId) {
    LOGI("Initializing Vulkan TBDR Renderer...");
    m_xrInstance = xrInstance;
    m_systemId = systemId;

    // Load OpenXR Vulkan Extension Functions (enable2)
    xrGetInstanceProcAddr(m_xrInstance, "xrGetVulkanGraphicsRequirements2KHR", (PFN_xrVoidFunction*)&pfnGetVulkanGraphicsRequirements2KHR);
    xrGetInstanceProcAddr(m_xrInstance, "xrCreateVulkanInstanceKHR", (PFN_xrVoidFunction*)&pfnCreateVulkanInstanceKHR);
    xrGetInstanceProcAddr(m_xrInstance, "xrGetVulkanGraphicsDevice2KHR", (PFN_xrVoidFunction*)&pfnGetVulkanGraphicsDevice2KHR);
    xrGetInstanceProcAddr(m_xrInstance, "xrCreateVulkanDeviceKHR", (PFN_xrVoidFunction*)&pfnCreateVulkanDeviceKHR);

    // Call GetVulkanGraphicsRequirements2KHR first (required by OpenXR spec)
    if (pfnGetVulkanGraphicsRequirements2KHR) {
        XrGraphicsRequirementsVulkanKHR requirements = {XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR};
        XR_CHECK(pfnGetVulkanGraphicsRequirements2KHR(m_xrInstance, m_systemId, &requirements));
        LOGI("Vulkan API Version Requirements: Min %d.%d.%d, Max %d.%d.%d",
             XR_VERSION_MAJOR(requirements.minApiVersionSupported), XR_VERSION_MINOR(requirements.minApiVersionSupported), XR_VERSION_PATCH(requirements.minApiVersionSupported),
             XR_VERSION_MAJOR(requirements.maxApiVersionSupported), XR_VERSION_MINOR(requirements.maxApiVersionSupported), XR_VERSION_PATCH(requirements.maxApiVersionSupported));
    }

    CreateInstance();
    CreateDevice();
    SetupCommandBuffers();
    SetupRenderPass();
    BuildFadeInPipeline();
}

void VulkanRenderer::CreateInstance() {
    LOGI("Creating Vulkan Instance...");
    
    if (!pfnCreateVulkanInstanceKHR) {
        LOGE("pfnCreateVulkanInstanceKHR is null! Did you enable XR_KHR_vulkan_enable2?");
        return;
    }

    VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName = "MegaDive OS";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Omnis Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo createInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    createInfo.pApplicationInfo = &appInfo;

    XrVulkanInstanceCreateInfoKHR xrVulkanCreateInfo = {XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR};
    xrVulkanCreateInfo.systemId = m_systemId;
    xrVulkanCreateInfo.pfnGetInstanceProcAddr = &vkGetInstanceProcAddr;
    xrVulkanCreateInfo.vulkanCreateInfo = &createInfo;
    xrVulkanCreateInfo.vulkanAllocator = nullptr;

    VkResult vkResult;
    XR_CHECK(pfnCreateVulkanInstanceKHR(m_xrInstance, &xrVulkanCreateInfo, &m_vkInstance, &vkResult));
    VK_CHECK(vkResult);

    LOGI("Vulkan Instance created successfully via OpenXR!");
}

void VulkanRenderer::CreateDevice() {
    LOGI("Creating Vulkan Device...");

    if (!pfnGetVulkanGraphicsDevice2KHR || !pfnCreateVulkanDeviceKHR) {
        LOGE("Vulkan Device functions are null! Did you enable XR_KHR_vulkan_enable2?");
        return;
    }

    // 1. Ask OpenXR which Physical Device it wants us to use
    XrVulkanGraphicsDeviceGetInfoKHR deviceGetInfo = {XR_TYPE_VULKAN_GRAPHICS_DEVICE_GET_INFO_KHR};
    deviceGetInfo.systemId = m_systemId;
    deviceGetInfo.vulkanInstance = m_vkInstance;
    XR_CHECK(pfnGetVulkanGraphicsDevice2KHR(m_xrInstance, &deviceGetInfo, &m_vkPhysicalDevice));

    // Log the physical device we got
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(m_vkPhysicalDevice, &deviceProperties);
    LOGI("Selected Vulkan Physical Device: %s (API: %d.%d)", 
         deviceProperties.deviceName,
         VK_VERSION_MAJOR(deviceProperties.apiVersion),
         VK_VERSION_MINOR(deviceProperties.apiVersion));

    // 2. Find a Graphics Queue Family
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            m_queueFamilyIndex = i;
            break;
        }
    }
    LOGI("Selected Graphics Queue Family Index: %u", m_queueFamilyIndex);

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueCreateInfo.queueFamilyIndex = m_queueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo deviceCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.enabledExtensionCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

    XrVulkanDeviceCreateInfoKHR xrVulkanDeviceCreateInfo = {XR_TYPE_VULKAN_DEVICE_CREATE_INFO_KHR};
    xrVulkanDeviceCreateInfo.systemId = m_systemId;
    xrVulkanDeviceCreateInfo.pfnGetInstanceProcAddr = &vkGetInstanceProcAddr;
    xrVulkanDeviceCreateInfo.vulkanPhysicalDevice = m_vkPhysicalDevice;
    xrVulkanDeviceCreateInfo.vulkanCreateInfo = &deviceCreateInfo;
    xrVulkanDeviceCreateInfo.vulkanAllocator = nullptr;

    VkResult vkResult;
    XR_CHECK(pfnCreateVulkanDeviceKHR(m_xrInstance, &xrVulkanDeviceCreateInfo, &m_vkDevice, &vkResult));
    VK_CHECK(vkResult);
    
    LOGI("Vulkan Logical Device created successfully via OpenXR!");

    vkGetDeviceQueue(m_vkDevice, m_queueFamilyIndex, 0, &m_vkQueue);
}

void VulkanRenderer::SetupCommandBuffers() {
    VkCommandPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_queueFamilyIndex;
    VK_CHECK(vkCreateCommandPool(m_vkDevice, &poolInfo, nullptr, &m_vkCommandPool));

    VkCommandBufferAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = m_vkCommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    VK_CHECK(vkAllocateCommandBuffers(m_vkDevice, &allocInfo, &m_vkCommandBuffer));
}

void VulkanRenderer::ClearImage(VkImage image, VkClearColorValue color) {
    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(m_vkCommandBuffer, &beginInfo);

    VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(m_vkCommandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkImageSubresourceRange range = {};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;
    vkCmdClearColorImage(m_vkCommandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &range);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // OpenXR expects this
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    vkCmdPipelineBarrier(m_vkCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    vkEndCommandBuffer(m_vkCommandBuffer);

    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_vkCommandBuffer;

    vkQueueSubmit(m_vkQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_vkQueue); // Wait for clear to finish
}

XrGraphicsBindingVulkanKHR VulkanRenderer::GetVulkanBinding() const {
    XrGraphicsBindingVulkanKHR binding = {XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR};
    binding.instance = m_vkInstance;
    binding.physicalDevice = m_vkPhysicalDevice;
    binding.device = m_vkDevice;
    binding.queueFamilyIndex = m_queueFamilyIndex;
    binding.queueIndex = 0;
    return binding;
}

void VulkanRenderer::SetupRenderPass() {
    LOGI("Vulkan Subpasses Configured for TBDR");
}

void VulkanRenderer::BuildFadeInPipeline() {
    LOGI("Fade-In Alpha Pipeline Built");
}

void VulkanRenderer::RenderFrame(float deltaTime, float fadeAlpha) {
    // Phase 2 rendering logic
    if (fadeAlpha > 0.01f) {
        // Implementation for fade quad goes here
    }
}
