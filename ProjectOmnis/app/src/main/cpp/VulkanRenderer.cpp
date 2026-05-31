#include "VulkanRenderer.h"
#include <stdexcept>
#include <string>
#include <cstring>

struct PushConstantData {
    Matrix4x4 mvp;
    float color[4];
};

const uint32_t hand_vert[] = 
#include "hand_vert.spv.h"
;

const uint32_t hand_frag[] = 
#include "hand_frag.spv.h"
;

struct Vertex {
    float pos[3];
    float color[3];
};

const Vertex cubeVertices[] = {
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},
    {{ 0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},
    {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},
    {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}},

    {{-0.5f, -0.5f,  0.5f}, {0.9f, 0.9f, 0.9f}},
    {{ 0.5f, -0.5f,  0.5f}, {0.9f, 0.9f, 0.9f}},
    {{ 0.5f,  0.5f,  0.5f}, {0.9f, 0.9f, 0.9f}},
    {{ 0.5f,  0.5f,  0.5f}, {0.9f, 0.9f, 0.9f}},
    {{-0.5f,  0.5f,  0.5f}, {0.9f, 0.9f, 0.9f}},
    {{-0.5f, -0.5f,  0.5f}, {0.9f, 0.9f, 0.9f}},

    {{-0.5f,  0.5f,  0.5f}, {0.8f, 0.8f, 0.8f}},
    {{-0.5f,  0.5f, -0.5f}, {0.8f, 0.8f, 0.8f}},
    {{-0.5f, -0.5f, -0.5f}, {0.8f, 0.8f, 0.8f}},
    {{-0.5f, -0.5f, -0.5f}, {0.8f, 0.8f, 0.8f}},
    {{-0.5f, -0.5f,  0.5f}, {0.8f, 0.8f, 0.8f}},
    {{-0.5f,  0.5f,  0.5f}, {0.8f, 0.8f, 0.8f}},

    {{ 0.5f,  0.5f,  0.5f}, {0.8f, 0.8f, 0.8f}},
    {{ 0.5f,  0.5f, -0.5f}, {0.8f, 0.8f, 0.8f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.8f, 0.8f, 0.8f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.8f, 0.8f, 0.8f}},
    {{ 0.5f, -0.5f,  0.5f}, {0.8f, 0.8f, 0.8f}},
    {{ 0.5f,  0.5f,  0.5f}, {0.8f, 0.8f, 0.8f}},

    {{-0.5f, -0.5f, -0.5f}, {0.7f, 0.7f, 0.7f}},
    {{ 0.5f, -0.5f, -0.5f}, {0.7f, 0.7f, 0.7f}},
    {{ 0.5f, -0.5f,  0.5f}, {0.7f, 0.7f, 0.7f}},
    {{ 0.5f, -0.5f,  0.5f}, {0.7f, 0.7f, 0.7f}},
    {{-0.5f, -0.5f,  0.5f}, {0.7f, 0.7f, 0.7f}},
    {{-0.5f, -0.5f, -0.5f}, {0.7f, 0.7f, 0.7f}},

    {{-0.5f,  0.5f, -0.5f}, {0.7f, 0.7f, 0.7f}},
    {{ 0.5f,  0.5f, -0.5f}, {0.7f, 0.7f, 0.7f}},
    {{ 0.5f,  0.5f,  0.5f}, {0.7f, 0.7f, 0.7f}},
    {{ 0.5f,  0.5f,  0.5f}, {0.7f, 0.7f, 0.7f}},
    {{-0.5f,  0.5f,  0.5f}, {0.7f, 0.7f, 0.7f}},
    {{-0.5f,  0.5f, -0.5f}, {0.7f, 0.7f, 0.7f}}
};

// OpenXR Extension function pointers for XR_KHR_vulkan_enable2
PFN_xrGetVulkanGraphicsRequirements2KHR pfnGetVulkanGraphicsRequirements2KHR = nullptr;
PFN_xrCreateVulkanInstanceKHR pfnCreateVulkanInstanceKHR = nullptr;
PFN_xrGetVulkanGraphicsDevice2KHR pfnGetVulkanGraphicsDevice2KHR = nullptr;
PFN_xrCreateVulkanDeviceKHR pfnCreateVulkanDeviceKHR = nullptr;

VulkanRenderer::VulkanRenderer() {}

VulkanRenderer::~VulkanRenderer() {
    LOGI("VulkanRenderer Destroyed");

    if (m_vkDevice != VK_NULL_HANDLE) {
        if (m_pipeline) vkDestroyPipeline(m_vkDevice, m_pipeline, nullptr);
        if (m_pipelineLayout) vkDestroyPipelineLayout(m_vkDevice, m_pipelineLayout, nullptr);
        if (m_renderPass) vkDestroyRenderPass(m_vkDevice, m_renderPass, nullptr);
        if (m_vertexBuffer) vkDestroyBuffer(m_vkDevice, m_vertexBuffer, nullptr);
        if (m_vertexBufferMemory) vkFreeMemory(m_vkDevice, m_vertexBufferMemory, nullptr);
        if (m_redVertexBuffer) vkDestroyBuffer(m_vkDevice, m_redVertexBuffer, nullptr);
        if (m_redVertexBufferMemory) vkFreeMemory(m_vkDevice, m_redVertexBufferMemory, nullptr);

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
    CreateVertexBuffer();
    BuildPipeline();
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


uint32_t VulkanRenderer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return 0;
}

void VulkanRenderer::CreateVertexBuffer() {
    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = sizeof(cubeVertices);
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK(vkCreateBuffer(m_vkDevice, &bufferInfo, nullptr, &m_vertexBuffer));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_vkDevice, m_vertexBuffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VK_CHECK(vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &m_vertexBufferMemory));
    vkBindBufferMemory(m_vkDevice, m_vertexBuffer, m_vertexBufferMemory, 0);

    void* data;
    vkMapMemory(m_vkDevice, m_vertexBufferMemory, 0, bufferInfo.size, 0, &data);
    memcpy(data, cubeVertices, (size_t)bufferInfo.size);
    vkUnmapMemory(m_vkDevice, m_vertexBufferMemory);

    // Create Red Vertex Buffer for Laser
    VK_CHECK(vkCreateBuffer(m_vkDevice, &bufferInfo, nullptr, &m_redVertexBuffer));
    vkGetBufferMemoryRequirements(m_vkDevice, m_redVertexBuffer, &memRequirements);
    allocInfo.allocationSize = memRequirements.size;
    VK_CHECK(vkAllocateMemory(m_vkDevice, &allocInfo, nullptr, &m_redVertexBufferMemory));
    vkBindBufferMemory(m_vkDevice, m_redVertexBuffer, m_redVertexBufferMemory, 0);

    vkMapMemory(m_vkDevice, m_redVertexBufferMemory, 0, bufferInfo.size, 0, &data);
    Vertex redVertices[36];
    memcpy(redVertices, cubeVertices, sizeof(cubeVertices));
    for (int i = 0; i < 36; ++i) {
        redVertices[i].color[0] = 1.0f; // R
        redVertices[i].color[1] = 0.0f; // G
        redVertices[i].color[2] = 0.0f; // B
    }
    memcpy(data, redVertices, sizeof(redVertices));
    vkUnmapMemory(m_vkDevice, m_redVertexBufferMemory);
}

void VulkanRenderer::SetupRenderPass() {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = VK_FORMAT_R8G8B8A8_SRGB;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // because we cleared it previously
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VK_CHECK(vkCreateRenderPass(m_vkDevice, &renderPassInfo, nullptr, &m_renderPass));
}

void VulkanRenderer::BuildPipeline() {
    VkShaderModuleCreateInfo vertInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    vertInfo.codeSize = sizeof(hand_vert);
    vertInfo.pCode = hand_vert;
    VkShaderModule vertShader;
    vkCreateShaderModule(m_vkDevice, &vertInfo, nullptr, &vertShader);

    VkShaderModuleCreateInfo fragInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    fragInfo.codeSize = sizeof(hand_frag);
    fragInfo.pCode = hand_frag;
    VkShaderModule fragShader;
    vkCreateShaderModule(m_vkDevice, &fragInfo, nullptr, &fragShader);

    VkPipelineShaderStageCreateInfo vertStage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertShader;
    vertStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragStage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragShader;
    fragStage.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertStage, fragStage};

    VkVertexInputBindingDescription bindingDesc = {};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(Vertex);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrDesc[2] = {};
    attrDesc[0].binding = 0;
    attrDesc[0].location = 0;
    attrDesc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrDesc[0].offset = offsetof(Vertex, pos);
    
    attrDesc[1].binding = 0;
    attrDesc[1].location = 1;
    attrDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrDesc[1].offset = offsetof(Vertex, color);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = attrDesc;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPushConstantRange pushConstant = {};
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstant.offset = 0;
    pushConstant.size = sizeof(PushConstantData);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

    VK_CHECK(vkCreatePipelineLayout(m_vkDevice, &pipelineLayoutInfo, nullptr, &m_pipelineLayout));

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicStateInfo = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamicStateInfo.dynamicStateCount = 2;
    dynamicStateInfo.pDynamicStates = dynamicStates;

    VkGraphicsPipelineCreateInfo pipelineInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicStateInfo;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;

    VK_CHECK(vkCreateGraphicsPipelines(m_vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline));

    vkDestroyShaderModule(m_vkDevice, vertShader, nullptr);
    vkDestroyShaderModule(m_vkDevice, fragShader, nullptr);
}

void VulkanRenderer::BeginRender(VkImage image, uint32_t width, uint32_t height) {
    VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(m_vkCommandBuffer, &beginInfo);
    
    // Create ImageView and Framebuffer
    VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    vkCreateImageView(m_vkDevice, &viewInfo, nullptr, &m_currentImageView);

    VkFramebufferCreateInfo fbInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    fbInfo.renderPass = m_renderPass;
    fbInfo.attachmentCount = 1;
    fbInfo.pAttachments = &m_currentImageView;
    fbInfo.width = width;
    fbInfo.height = height;
    fbInfo.layers = 1;
    vkCreateFramebuffer(m_vkDevice, &fbInfo, nullptr, &m_currentFramebuffer);

    VkRenderPassBeginInfo renderPassInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_currentFramebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = {width, height};

    vkCmdBeginRenderPass(m_vkCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    vkCmdBindPipeline(m_vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    
    VkViewport viewport = {0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f};
    vkCmdSetViewport(m_vkCommandBuffer, 0, 1, &viewport);
    
    VkRect2D scissor = {{0, 0}, {width, height}};
    vkCmdSetScissor(m_vkCommandBuffer, 0, 1, &scissor);
}

void VulkanRenderer::EndRender() {
    vkCmdEndRenderPass(m_vkCommandBuffer);
    vkEndCommandBuffer(m_vkCommandBuffer);

    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_vkCommandBuffer;
    vkQueueSubmit(m_vkQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_vkQueue);
    
    vkDestroyFramebuffer(m_vkDevice, m_currentFramebuffer, nullptr);
    vkDestroyImageView(m_vkDevice, m_currentImageView, nullptr);
    m_currentFramebuffer = VK_NULL_HANDLE;
    m_currentImageView = VK_NULL_HANDLE;
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

struct HandBone {
    int parent;
    int child;
};

const HandBone HAND_BONES[] = {
    {0, 1}, // Wrist to Palm
    
    // Thumb
    {1, 2}, {2, 3}, {3, 4}, {4, 5},
    
    // Index
    {1, 6}, {6, 7}, {7, 8}, {8, 9}, {9, 10},
    
    // Middle
    {1, 11}, {11, 12}, {12, 13}, {13, 14}, {14, 15},
    
    // Ring
    {1, 16}, {16, 17}, {17, 18}, {18, 19}, {19, 20},
    
    // Pinky
    {1, 21}, {21, 22}, {22, 23}, {23, 24}, {24, 25},
    
    // Knuckle lines
    {7, 12}, {12, 17}, {17, 22}
};
const int HAND_BONE_COUNT = sizeof(HAND_BONES) / sizeof(HAND_BONES[0]);

void VulkanRenderer::RenderHands(const XrHandJointLocationEXT* leftJoints, bool leftActive, 
                                 const XrHandJointLocationEXT* rightJoints, bool rightActive,
                                 const Matrix4x4& viewProj) {
    if (m_pipeline == VK_NULL_HANDLE) return;

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(m_vkCommandBuffer, 0, 1, &m_vertexBuffer, offsets);

    // Left Hand
    if (leftActive && leftJoints) {
        // Draw bones
        for (int i = 0; i < HAND_BONE_COUNT; ++i) {
            int pIdx = HAND_BONES[i].parent;
            int cIdx = HAND_BONES[i].child;
            if ((leftJoints[pIdx].locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
                (leftJoints[cIdx].locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0) {
                
                Matrix4x4 model, mvp;
                CreateBoneMatrix(&model, leftJoints[pIdx].pose.position, leftJoints[cIdx].pose.position, 0.008f);
                Matrix4x4_Multiply(&mvp, &viewProj, &model);
                PushConstantData pcd;
                pcd.mvp = mvp;
                pcd.color[0] = 0.8f; pcd.color[1] = 0.8f; pcd.color[2] = 0.8f; pcd.color[3] = 1.0f;
                vkCmdPushConstants(m_vkCommandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData), &pcd);
                vkCmdDraw(m_vkCommandBuffer, 36, 1, 0, 0);
            }
        }
        
        // Draw joints
        for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; ++i) {
            if ((leftJoints[i].locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0) {
                Matrix4x4 model, mvp;
                float cubeSize = leftJoints[i].radius * 2.0f; // Joint sphere fallback size (scaled by OpenXR reported radius)
                if (cubeSize < 0.001f) cubeSize = 0.015f; 
                float scale[3] = {cubeSize, cubeSize, cubeSize};
                CreateModelMatrix(&model, leftJoints[i].pose, scale);
                Matrix4x4_Multiply(&mvp, &viewProj, &model);
                PushConstantData pcd;
                pcd.mvp = mvp;
                pcd.color[0] = 0.9f; pcd.color[1] = 0.9f; pcd.color[2] = 0.9f; pcd.color[3] = 1.0f;
                vkCmdPushConstants(m_vkCommandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData), &pcd);
                vkCmdDraw(m_vkCommandBuffer, 36, 1, 0, 0);
            }
        }
    }
    
    // Right Hand
    if (rightActive && rightJoints) {
        // Draw bones
        for (int i = 0; i < HAND_BONE_COUNT; ++i) {
            int pIdx = HAND_BONES[i].parent;
            int cIdx = HAND_BONES[i].child;
            if ((rightJoints[pIdx].locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
                (rightJoints[cIdx].locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0) {
                
                Matrix4x4 model, mvp;
                CreateBoneMatrix(&model, rightJoints[pIdx].pose.position, rightJoints[cIdx].pose.position, 0.008f);
                Matrix4x4_Multiply(&mvp, &viewProj, &model);
                PushConstantData pcd;
                pcd.mvp = mvp;
                pcd.color[0] = 0.8f; pcd.color[1] = 0.8f; pcd.color[2] = 0.8f; pcd.color[3] = 1.0f;
                vkCmdPushConstants(m_vkCommandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData), &pcd);
                vkCmdDraw(m_vkCommandBuffer, 36, 1, 0, 0);
            }
        }
        
        // Draw joints
        for (int i = 0; i < XR_HAND_JOINT_COUNT_EXT; ++i) {
            if ((rightJoints[i].locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0) {
                Matrix4x4 model, mvp;
                float cubeSize = rightJoints[i].radius * 2.0f;
                if (cubeSize < 0.001f) cubeSize = 0.015f;
                float scale[3] = {cubeSize, cubeSize, cubeSize};
                CreateModelMatrix(&model, rightJoints[i].pose, scale);
                Matrix4x4_Multiply(&mvp, &viewProj, &model);
                PushConstantData pcd;
                pcd.mvp = mvp;
                pcd.color[0] = 0.9f; pcd.color[1] = 0.9f; pcd.color[2] = 0.9f; pcd.color[3] = 1.0f;
                vkCmdPushConstants(m_vkCommandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData), &pcd);
                vkCmdDraw(m_vkCommandBuffer, 36, 1, 0, 0);
            }
        }
    }
}

void VulkanRenderer::RenderLaser(const float origin[3], const float dir[3], const float color[4], const Matrix4x4& viewProj) {
    if (m_pipeline == VK_NULL_HANDLE) return;

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(m_vkCommandBuffer, 0, 1, &m_redVertexBuffer, offsets);

    Matrix4x4 model, mvp;
    
    // Stretch cube to make a long laser. 10 meters long, 2mm thick.
    float length = 10.0f;
    float scale[3] = {0.002f, 0.002f, length};
    
    XrPosef pose;
    pose.position.x = origin[0] + dir[0] * (length * 0.5f);
    pose.position.y = origin[1] + dir[1] * (length * 0.5f);
    pose.position.z = origin[2] + dir[2] * (length * 0.5f);
    
    // Create rotation quaternion from direction vector (z-axis pointing)
    float d[3] = {dir[0], dir[1], dir[2]};
    float up[3] = {0, 1, 0};
    if (fabs(d[1]) > 0.99f) { up[0] = 1; up[1] = 0; }
    
    // Right = cross(up, d)
    float r[3] = {
        up[1]*d[2] - up[2]*d[1],
        up[2]*d[0] - up[0]*d[2],
        up[0]*d[1] - up[1]*d[0]
    };
    float rLen = sqrtf(r[0]*r[0] + r[1]*r[1] + r[2]*r[2]);
    if (rLen > 0) { r[0]/=rLen; r[1]/=rLen; r[2]/=rLen; }
    
    // Real Up = cross(d, r)
    float u[3] = {
        d[1]*r[2] - d[2]*r[1],
        d[2]*r[0] - d[0]*r[2],
        d[0]*r[1] - d[1]*r[0]
    };
    
    // CreateModelMatrix takes XrPosef which takes quaternion. Since we are creating custom matrix, we can bypass CreateModelMatrix and build it directly.
    for(int i = 0; i < 16; ++i) model.m[i] = 0.0f;
    model.m[0] = model.m[5] = model.m[10] = model.m[15] = 1.0f;
    model.m[0] = r[0] * scale[0]; model.m[1] = r[1] * scale[0]; model.m[2] = r[2] * scale[0];
    model.m[4] = u[0] * scale[1]; model.m[5] = u[1] * scale[1]; model.m[6] = u[2] * scale[1];
    // Notice: our 'd' is forward.
    model.m[8] = d[0] * scale[2]; model.m[9] = d[1] * scale[2]; model.m[10] = d[2] * scale[2];
    model.m[12] = pose.position.x; model.m[13] = pose.position.y; model.m[14] = pose.position.z;

    Matrix4x4_Multiply(&mvp, &viewProj, &model);
    
    PushConstantData pcd;
    pcd.mvp = mvp;
    pcd.color[0] = color[0]; pcd.color[1] = color[1]; pcd.color[2] = color[2]; pcd.color[3] = color[3];
    vkCmdPushConstants(m_vkCommandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData), &pcd);
    vkCmdDraw(m_vkCommandBuffer, 36, 1, 0, 0);
}

void VulkanRenderer::RenderMenuButtons(const std::vector<MenuButton>& buttons, const Matrix4x4& viewProj) {
    if (m_pipeline == VK_NULL_HANDLE) return;

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(m_vkCommandBuffer, 0, 1, &m_vertexBuffer, offsets);

    for (const auto& btn : buttons) {
        Matrix4x4 model, mvp;
        
        float size[3] = { btn.size[0], btn.size[1], btn.size[2] };
        if (btn.hovered) {
            // Hovered buttons look larger (Z scales for popout depth)
            size[0] *= 1.15f;
            size[1] *= 1.15f;
            size[2] *= 2.0f;
        }

        CreateModelMatrix(&model, btn.pose, size);
        Matrix4x4_Multiply(&mvp, &viewProj, &model);
        
        PushConstantData pcd;
        pcd.mvp = mvp;
        pcd.color[0] = btn.color[0]; pcd.color[1] = btn.color[1]; pcd.color[2] = btn.color[2]; pcd.color[3] = 1.0f;
        vkCmdPushConstants(m_vkCommandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData), &pcd);
        vkCmdDraw(m_vkCommandBuffer, 36, 1, 0, 0);
    }
}
