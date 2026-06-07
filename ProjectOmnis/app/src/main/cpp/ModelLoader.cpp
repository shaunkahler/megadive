#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "ModelLoader.h"
#include <android/log.h>

#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "OmnisEngine_ModelLoader", __VA_ARGS__))
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "OmnisEngine_ModelLoader", __VA_ARGS__))

MeshData ModelLoader::LoadGLB(AAssetManager* assetManager, const char* assetPath) {
    MeshData result;
    
    AAsset* asset = AAssetManager_open(assetManager, assetPath, AASSET_MODE_BUFFER);
    if (!asset) {
        LOGE("Failed to open %s", assetPath);
        return result;
    }
    
    off_t fileLength = AAsset_getLength(asset);
    const void* fileData = AAsset_getBuffer(asset);
    void* allocatedData = nullptr;
    if (!fileData) {
        allocatedData = malloc(fileLength);
        AAsset_read(asset, allocatedData, fileLength);
        fileData = allocatedData;
    }

    cgltf_options options = {};
    cgltf_data* data = nullptr;
    cgltf_result parse_result = cgltf_parse(&options, fileData, fileLength, &data);
    
    if (parse_result != cgltf_result_success) {
        LOGE("Failed to parse GLB %s", assetPath);
        if (allocatedData) free(allocatedData);
        AAsset_close(asset);
        return result;
    }
    
    cgltf_result load_result = cgltf_load_buffers(&options, data, assetPath);
    if (load_result != cgltf_result_success) {
        LOGE("Failed to load buffers for GLB %s", assetPath);
        cgltf_free(data);
        if (allocatedData) free(allocatedData);
        AAsset_close(asset);
        return result;
    }

    // Load images
    std::vector<unsigned char*> image_pixels(data->images_count, nullptr);
    std::vector<int> image_widths(data->images_count, 0);
    std::vector<int> image_heights(data->images_count, 0);
    for (cgltf_size i = 0; i < data->images_count; ++i) {
        cgltf_image* img = &data->images[i];
        if (img->buffer_view && img->buffer_view->buffer->data) {
            int comp;
            unsigned char* pixels = stbi_load_from_memory(
                (const stbi_uc*)((const uint8_t*)img->buffer_view->buffer->data + img->buffer_view->offset),
                img->buffer_view->size,
                &image_widths[i], &image_heights[i], &comp, 4);
            image_pixels[i] = pixels;
        }
    }

    // Extract meshes from nodes to apply transforms
    for (cgltf_size n = 0; n < data->nodes_count; ++n) {
        cgltf_node* node = &data->nodes[n];
        if (!node->mesh) continue;
        
        cgltf_float matrix[16];
        cgltf_node_transform_world(node, matrix);
        
        cgltf_mesh* mesh = node->mesh;
        for (cgltf_size p = 0; p < mesh->primitives_count; ++p) {
            cgltf_primitive* primitive = &mesh->primitives[p];
            
            int texture_image_index = -1;
            float base_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
            if (primitive->material && primitive->material->has_pbr_metallic_roughness) {
                cgltf_pbr_metallic_roughness* pbr = &primitive->material->pbr_metallic_roughness;
                base_color[0] = pbr->base_color_factor[0];
                base_color[1] = pbr->base_color_factor[1];
                base_color[2] = pbr->base_color_factor[2];
                base_color[3] = pbr->base_color_factor[3];
                
                if (pbr->base_color_texture.texture && pbr->base_color_texture.texture->image) {
                    texture_image_index = pbr->base_color_texture.texture->image - data->images;
                }
            }

            uint32_t vertexOffset = (uint32_t)result.vertices.size();
            cgltf_accessor* texcoord_accessor = nullptr;
            
            // Extract attributes (positions)
            for (cgltf_size a = 0; a < primitive->attributes_count; ++a) {
                cgltf_attribute* attribute = &primitive->attributes[a];
                if (attribute->type == cgltf_attribute_type_texcoord) {
                    texcoord_accessor = attribute->data;
                }
                if (attribute->type == cgltf_attribute_type_position) {
                    cgltf_accessor* accessor = attribute->data;
                    result.vertices.resize(vertexOffset + accessor->count);
                    
                    for (cgltf_size v = 0; v < accessor->count; ++v) {
                        float pos[3];
                        cgltf_accessor_read_float(accessor, v, pos, 3);
                        
                        // Apply node transform matrix (column-major)
                        result.vertices[vertexOffset + v].pos[0] = matrix[0] * pos[0] + matrix[4] * pos[1] + matrix[8] * pos[2] + matrix[12];
                        result.vertices[vertexOffset + v].pos[1] = matrix[1] * pos[0] + matrix[5] * pos[1] + matrix[9] * pos[2] + matrix[13];
                        result.vertices[vertexOffset + v].pos[2] = matrix[2] * pos[0] + matrix[6] * pos[1] + matrix[10] * pos[2] + matrix[14];
                        
                        // Default color
                        result.vertices[vertexOffset + v].color[0] = base_color[0];
                        result.vertices[vertexOffset + v].color[1] = base_color[1];
                        result.vertices[vertexOffset + v].color[2] = base_color[2];
                    }
                }
            }

            // Apply texture colors if available
            if (texcoord_accessor && texture_image_index >= 0 && image_pixels[texture_image_index]) {
                unsigned char* pixels = image_pixels[texture_image_index];
                int w = image_widths[texture_image_index];
                int h = image_heights[texture_image_index];
                
                for (cgltf_size v = 0; v < texcoord_accessor->count; ++v) {
                    float uv[2];
                    cgltf_accessor_read_float(texcoord_accessor, v, uv, 2);
                    
                    float u = uv[0] - floorf(uv[0]);
                    float v_coord = uv[1] - floorf(uv[1]);
                    
                    int px = (int)(u * w) % w;
                    int py = (int)(v_coord * h) % h;
                    if (px < 0) px += w;
                    if (py < 0) py += h;
                    
                    int pidx = (py * w + px) * 4;
                    result.vertices[vertexOffset + v].color[0] *= (pixels[pidx] / 255.0f);
                    result.vertices[vertexOffset + v].color[1] *= (pixels[pidx+1] / 255.0f);
                    result.vertices[vertexOffset + v].color[2] *= (pixels[pidx+2] / 255.0f);
                }
            }

            // Extract indices
            if (primitive->indices) {
                cgltf_accessor* accessor = primitive->indices;
                size_t indexOffset = result.indices.size();
                result.indices.resize(indexOffset + accessor->count);
                for (cgltf_size i = 0; i < accessor->count; ++i) {
                    result.indices[indexOffset + i] = vertexOffset + (uint32_t)cgltf_accessor_read_index(accessor, i);
                }
            } else {
                // Non-indexed primitive, auto-generate indices
                size_t vertCount = result.vertices.size() - vertexOffset;
                size_t indexOffset = result.indices.size();
                result.indices.resize(indexOffset + vertCount);
                for (uint32_t i = 0; i < vertCount; ++i) {
                    result.indices[indexOffset + i] = vertexOffset + i;
                }
            }
        }
    }

    // Fallback if no nodes had meshes but meshes exist
    if (result.vertices.empty() && data->meshes_count > 0) {
        cgltf_mesh* mesh = &data->meshes[0];
        for (cgltf_size p = 0; p < mesh->primitives_count; ++p) {
            cgltf_primitive* primitive = &mesh->primitives[p];
            uint32_t vertexOffset = (uint32_t)result.vertices.size();
            
            for (cgltf_size a = 0; a < primitive->attributes_count; ++a) {
                cgltf_attribute* attribute = &primitive->attributes[a];
                if (attribute->type == cgltf_attribute_type_position) {
                    cgltf_accessor* accessor = attribute->data;
                    result.vertices.resize(vertexOffset + accessor->count);
                    for (cgltf_size v = 0; v < accessor->count; ++v) {
                        cgltf_accessor_read_float(accessor, v, result.vertices[vertexOffset + v].pos, 3);
                        result.vertices[vertexOffset + v].color[0] = 1.0f;
                        result.vertices[vertexOffset + v].color[1] = 1.0f;
                        result.vertices[vertexOffset + v].color[2] = 1.0f;
                    }
                }
            }

            if (primitive->indices) {
                cgltf_accessor* accessor = primitive->indices;
                size_t indexOffset = result.indices.size();
                result.indices.resize(indexOffset + accessor->count);
                for (cgltf_size i = 0; i < accessor->count; ++i) {
                    result.indices[indexOffset + i] = vertexOffset + (uint32_t)cgltf_accessor_read_index(accessor, i);
                }
            } else {
                size_t vertCount = result.vertices.size() - vertexOffset;
                size_t indexOffset = result.indices.size();
                result.indices.resize(indexOffset + vertCount);
                for (uint32_t i = 0; i < vertCount; ++i) {
                    result.indices[indexOffset + i] = vertexOffset + i;
                }
            }
        }
    }

    // Free images
    for (size_t i = 0; i < image_pixels.size(); ++i) {
        if (image_pixels[i]) {
            stbi_image_free(image_pixels[i]);
        }
    }

    cgltf_free(data);
    if (allocatedData) free(allocatedData);
    AAsset_close(asset);
    
    LOGI("Successfully loaded %s: %zu vertices, %zu indices", assetPath, result.vertices.size(), result.indices.size());
    return result;
}
