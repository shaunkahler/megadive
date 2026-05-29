#pragma once

#include <cstdint>
#include <atomic>
#include <thread>
#include "ECS.h"
#include "NetworkClient.h"

enum class WorldState {
    HOME_WORLD,
    WARMING_PORTAL,
    IN_ENVIRONMENT
};

class WorldManager {
public:
    WorldManager();
    ~WorldManager();

    // Map the pre-allocated memory arenas
    void Initialize(void* homeArena, void* currentArena, void* stagingArena);

    // [MQ-5] Portal Warm-up & Streaming
    void TriggerPortalWarmup(uint32_t portalId);
    
    // [MQ-6] Instant Teleport
    void InstantGoHome();

    void Update(float deltaTime);

    NetworkClient* GetNetworkClient() { return &m_network; }

private:
    WorldState m_currentState;
    
    // Memory Arenas for the 3-World Cache Engine
    void* m_homeArena;
    void* m_currentArena;
    void* m_stagingArena;

    NetworkClient m_network;

    std::atomic<bool> m_isStreamingPortal{false};
    std::thread m_backgroundStreamThread;

    void BackgroundStreamTask(uint32_t portalId);
    void FlushArena(void* arena, size_t size);
};
