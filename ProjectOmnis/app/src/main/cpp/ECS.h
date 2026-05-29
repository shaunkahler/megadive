#pragma once
#include <cstdint>
#include <vector>

// Forward declarations matching OpenXR Math
struct XrVector3f;
struct XrQuaternionf;

// --- Basic Entity Component System (ECS) ---

typedef uint64_t EntityID;

struct TransformComponent {
    float position[3];
    float rotation[4]; // Quaternion
    float scale[3];
};

struct MeshComponent {
    uint32_t assetId;
    uint32_t materialOverrideId;
};

struct NetworkStateComponent {
    EntityID serverId;
    uint64_t lastUpdateTimestamp;
};

// Represents a serializable delta-modification for [MQ-6]
// When a user modifies the world, we don't save the mesh, just this 64-byte payload.
struct EntityModificationPacket {
    EntityID entity;
    TransformComponent transform;
    uint32_t newMaterialId;
};
