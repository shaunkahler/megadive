#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include <vector>
#include <string>
#include <android/asset_manager.h>

struct MeshVertex {
    float pos[3];
    float color[3];
};

struct MeshData {
    std::vector<MeshVertex> vertices;
    std::vector<uint32_t> indices;
};

class ModelLoader {
public:
    // Loads a .glb or .gltf file from Android assets and returns the combined mesh data
    static MeshData LoadGLB(AAssetManager* assetManager, const char* assetPath);
};

#endif // MODEL_LOADER_H
