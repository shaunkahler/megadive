#define CGLTF_IMPLEMENTATION
#include "cgltf.h"
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

    cgltf_options options = {};
    cgltf_data* data = nullptr;
    cgltf_result parse_result = cgltf_parse(&options, fileData, fileLength, &data);
    
    if (parse_result != cgltf_result_success) {
        LOGE("Failed to parse GLB %s", assetPath);
        AAsset_close(asset);
        return result;
    }
    
    cgltf_result load_result = cgltf_load_buffers(&options, data, assetPath);
    if (load_result != cgltf_result_success) {
        LOGE("Failed to load buffers for GLB %s", assetPath);
        cgltf_free(data);
        AAsset_close(asset);
        return result;
    }

    // Extract first mesh
    if (data->meshes_count > 0) {
        cgltf_mesh* mesh = &data->meshes[0];
        for (cgltf_size p = 0; p < mesh->primitives_count; ++p) {
            cgltf_primitive* primitive = &mesh->primitives[p];
            
            uint32_t vertexOffset = (uint32_t)result.vertices.size();
            
            // Extract attributes (positions)
            for (cgltf_size a = 0; a < primitive->attributes_count; ++a) {
                cgltf_attribute* attribute = &primitive->attributes[a];
                if (attribute->type == cgltf_attribute_type_position) {
                    cgltf_accessor* accessor = attribute->data;
                    result.vertices.resize(vertexOffset + accessor->count);
                    
                    for (cgltf_size v = 0; v < accessor->count; ++v) {
                        cgltf_accessor_read_float(accessor, v, result.vertices[vertexOffset + v].pos, 3);
                        // Default color
                        result.vertices[vertexOffset + v].color[0] = 1.0f;
                        result.vertices[vertexOffset + v].color[1] = 1.0f;
                        result.vertices[vertexOffset + v].color[2] = 1.0f;
                    }
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

    cgltf_free(data);
    AAsset_close(asset);
    
    LOGI("Successfully loaded %s: %zu vertices, %zu indices", assetPath, result.vertices.size(), result.indices.size());
    return result;
}
