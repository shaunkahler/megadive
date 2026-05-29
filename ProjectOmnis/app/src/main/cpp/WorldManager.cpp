#include "WorldManager.h"
#include <android/log.h>
#include <sys/mman.h>
#include <unistd.h>
#include <chrono>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "OmnisEngine_World", __VA_ARGS__))

WorldManager::WorldManager() : m_currentState(WorldState::HOME_WORLD), 
    m_homeArena(nullptr), m_currentArena(nullptr), m_stagingArena(nullptr) {}

WorldManager::~WorldManager() {
    if (m_backgroundStreamThread.joinable()) {
        m_backgroundStreamThread.join();
    }
}

void WorldManager::Initialize(void* homeArena, void* currentArena, void* stagingArena) {
    LOGI("Initializing World Manager with 3-World Cache Arenas...");
    m_homeArena = homeArena;
    m_currentArena = currentArena;
    m_stagingArena = stagingArena;

    // Connect to MMO server
    m_network.Initialize("192.168.1.100", 7777);
}

void WorldManager::TriggerPortalWarmup(uint32_t portalId) {
    if (m_isStreamingPortal) {
        LOGI("Portal already warming up!");
        return;
    }

    LOGI("Initiating Portal Warmup for World %d. User remains in Home World.", portalId);
    m_currentState = WorldState::WARMING_PORTAL;
    m_isStreamingPortal = true;

    // Launch background thread explicitly mapped to Core 2 (Efficiency Core)
    m_backgroundStreamThread = std::thread(&WorldManager::BackgroundStreamTask, this, portalId);
}

void WorldManager::BackgroundStreamTask(uint32_t portalId) {
    // Map thread to Efficiency Core 2 to avoid throttling main render thread
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(2, &cpuset);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

    LOGI("[Core 2] Streaming World %d from UFS Disk into Staging Arena (1.5GB)", portalId);
    
    // Simulate disk I/O and asset decompression
    std::this_thread::sleep_for(std::chrono::seconds(2)); 
    
    LOGI("[Core 2] Portal Warmup Complete. Ready for transition.");
    m_isStreamingPortal = false;
    
    // In a full implementation, we'd swap the Staging and Current arena pointers here
    // m_currentState = WorldState::IN_ENVIRONMENT;
}

void WorldManager::InstantGoHome() {
    LOGI("EMERGENCY OVERRIDE: Instant 'Go Home' Command Received.");
    
    if (m_currentState != WorldState::HOME_WORLD) {
        // Drop the current environment from RAM immediately to prevent LMK crash
        FlushArena(m_currentArena, 2LL * 1024 * 1024 * 1024);
        
        // Immediately snap state back to the pinned Home Arena
        m_currentState = WorldState::HOME_WORLD;
        LOGI("Teleported to Home World. Active arena flushed.");
    }
}

void WorldManager::FlushArena(void* arena, size_t size) {
    if (arena) {
        // [MQ-2] Inform Linux Kernel to reclaim physical pages, but keep virtual mapping
        madvise(arena, size, MADV_DONTNEED);
    }
}

void WorldManager::Update(float deltaTime) {
    // Background thread handles actual network I/O, main thread just polls for synced ECS states
    // In a real app, m_network.Update() would be on the Core 2 thread loop.
    m_network.Update();
}
