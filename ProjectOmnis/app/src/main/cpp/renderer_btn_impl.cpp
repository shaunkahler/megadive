void VulkanRenderer::RenderMenuButtons(const std::vector<MenuButton>& buttons, const Matrix4x4& viewProj) {
    if (m_pipeline == VK_NULL_HANDLE) return;

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(m_vkCommandBuffer, 0, 1, &m_vertexBuffer, offsets);

    for (const auto& btn : buttons) {
        Matrix4x4 model, mvp;
        
        float size[3] = { btn.size[0], btn.size[1], btn.size[2] };
        if (btn.hovered) {
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

    if (m_uiPipeline == VK_NULL_HANDLE || m_fontImage == VK_NULL_HANDLE) return;

    // Draw UI Text
    vkCmdBindPipeline(m_vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_uiPipeline);
    vkCmdBindDescriptorSets(m_vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_uiPipelineLayout, 0, 1, &m_uiDescriptorSet, 0, nullptr);
    vkCmdBindVertexBuffers(m_vkCommandBuffer, 0, 1, &m_uiVertexBuffer, offsets);

    float* mappedData;
    vkMapMemory(m_vkDevice, m_uiVertexBufferMemory, 0, VK_WHOLE_SIZE, 0, (void**)&mappedData);

    uint32_t vertexCount = 0;
    std::vector<uint32_t> buttonVertexCounts(buttons.size(), 0);
    std::vector<uint32_t> buttonStartVertices(buttons.size(), 0);

    for (size_t i = 0; i < buttons.size(); ++i) {
        const auto& btn = buttons[i];
        if (btn.label.empty() && btn.description.empty()) continue;
        
        buttonStartVertices[i] = vertexCount;

        auto renderText = [&](const std::string& text, float yOffsetScale, float sizeScaleFactor) {
            if (text.empty()) return;
            float x_dummy = 0.0f, y_dummy = 0.0f;
            for (char c : text) {
                if (c >= 32 && c < 128) {
                    stbtt_aligned_quad q;
                    stbtt_GetBakedQuad(m_cdata, 2048, 2048, c - 32, &x_dummy, &y_dummy, &q, 1);
                }
            }
            float textWidth = x_dummy;
            float textHeight = 96.0f;
            
            float baseScale = btn.size[1] / textHeight * sizeScaleFactor; 
            if (btn.hovered) baseScale *= 1.15f;
            
            float maxTextWidth = btn.size[0] * 0.9f;
            if (btn.hovered) maxTextWidth *= 1.15f;
            float actualWidth = textWidth * baseScale;
            float scale = baseScale;
            if (actualWidth > maxTextWidth) {
                scale *= (maxTextWidth / actualWidth);
            }
            
            float startX = - (textWidth * scale) / 2.0f;
            float yOffset = (textHeight * yOffsetScale) * scale; 
            
            float temp_x = 0.0f;
            float temp_y = 0.0f;

            float zOffset = btn.size[2] + 0.01f; // Push it further out to ensure it's not clipped inside the box
            if (btn.hovered) zOffset = (btn.size[2] * 2.0f) + 0.01f;

            for (char c : text) {
                if (c >= 32 && c < 128) {
                    stbtt_aligned_quad q;
                    stbtt_GetBakedQuad(m_cdata, 2048, 2048, c - 32, &temp_x, &temp_y, &q, 1);
                    
                    float qx0 = startX + q.x0 * scale;
                    float qy0 = yOffset + q.y0 * scale; // Add instead of subtract to match Vulkan Y-down
                    float qx1 = startX + q.x1 * scale;
                    float qy1 = yOffset + q.y1 * scale;

                    // Counter-clockwise winding for Vulkan
                    mappedData[vertexCount*5 + 0] = qx0; mappedData[vertexCount*5 + 1] = qy0; mappedData[vertexCount*5 + 2] = zOffset;
                    mappedData[vertexCount*5 + 3] = q.s0; mappedData[vertexCount*5 + 4] = q.t0;
                    vertexCount++;
                    
                    mappedData[vertexCount*5 + 0] = qx0; mappedData[vertexCount*5 + 1] = qy1; mappedData[vertexCount*5 + 2] = zOffset;
                    mappedData[vertexCount*5 + 3] = q.s0; mappedData[vertexCount*5 + 4] = q.t1;
                    vertexCount++;
                    
                    mappedData[vertexCount*5 + 0] = qx1; mappedData[vertexCount*5 + 1] = qy1; mappedData[vertexCount*5 + 2] = zOffset;
                    mappedData[vertexCount*5 + 3] = q.s1; mappedData[vertexCount*5 + 4] = q.t1;
                    vertexCount++;
                    
                    mappedData[vertexCount*5 + 0] = qx1; mappedData[vertexCount*5 + 1] = qy1; mappedData[vertexCount*5 + 2] = zOffset;
                    mappedData[vertexCount*5 + 3] = q.s1; mappedData[vertexCount*5 + 4] = q.t1;
                    vertexCount++;
                    
                    mappedData[vertexCount*5 + 0] = qx1; mappedData[vertexCount*5 + 1] = qy0; mappedData[vertexCount*5 + 2] = zOffset;
                    mappedData[vertexCount*5 + 3] = q.s1; mappedData[vertexCount*5 + 4] = q.t0;
                    vertexCount++;
                    
                    mappedData[vertexCount*5 + 0] = qx0; mappedData[vertexCount*5 + 1] = qy0; mappedData[vertexCount*5 + 2] = zOffset;
                    mappedData[vertexCount*5 + 3] = q.s0; mappedData[vertexCount*5 + 4] = q.t0;
                    vertexCount++;
                }
            }
        };

        // Main label (centered slightly higher)
        renderText(btn.label, -0.1f, 0.3f);
        
        // Description text (smaller, centered lower)
        renderText(btn.description, 0.8f, 0.15f);

        buttonVertexCounts[i] = vertexCount - buttonStartVertices[i];
    }

    VkMappedMemoryRange range = {VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
    range.memory = m_uiVertexBufferMemory;
    range.offset = 0;
    range.size = VK_WHOLE_SIZE;
    vkFlushMappedMemoryRanges(m_vkDevice, 1, &range);

    vkUnmapMemory(m_vkDevice, m_uiVertexBufferMemory);

    for (size_t i = 0; i < buttons.size(); ++i) {
        if (buttonVertexCounts[i] == 0) continue;
        
        Matrix4x4 model, mvp;
        float textScale[3] = {1.0f, 1.0f, 1.0f};
        CreateModelMatrix(&model, buttons[i].pose, textScale);
        Matrix4x4_Multiply(&mvp, &viewProj, &model);
        
        PushConstantData pcd;
        pcd.mvp = mvp;
        pcd.color[0] = 0.0f; pcd.color[1] = 0.0f; pcd.color[2] = 0.0f; pcd.color[3] = 1.0f;
        
        vkCmdPushConstants(m_vkCommandBuffer, m_uiPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantData), &pcd);
        vkCmdDraw(m_vkCommandBuffer, buttonVertexCounts[i], 1, buttonStartVertices[i], 0);
    }
}
