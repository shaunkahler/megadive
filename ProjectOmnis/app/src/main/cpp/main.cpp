#include <android_native_app_glue.h>
#include <android/log.h>
#include <unistd.h>
#include <sched.h>
#include <sys/mman.h>

// Note: Requires OpenXR headers from Meta XR SDK
// #include <openxr/openxr.h>
// #include <openxr/openxr_platform.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "OmnisEngine", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "OmnisEngine", __VA_ARGS__))

// [MQ-5] Core Affinity Configuration
void SetThreadAffinity(int coreId) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(coreId, &cpuset);
    sched_setaffinity(gettid(), sizeof(cpu_set_t), &cpuset);
}

// [MQ-2] Memory Arena Allocation
void* AllocateMemoryArena(size_t sizeGB) {
    size_t sizeBytes = sizeGB * 1024 * 1024 * 1024;
    void* mem = mmap(NULL, sizeBytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) {
        LOGE("Failed to allocate %zu GB memory arena", sizeGB);
        return nullptr;
    }
    LOGI("Successfully allocated %zu GB memory arena", sizeGB);
    return mem;
}

// [MQ-1] Main Engine Loop
void RunEngineLoop(struct android_app* app) {
    LOGI("Entering main engine loop...");
    
    // Set Main Render Thread to Prime Core (Core 6)
    SetThreadAffinity(6);

    // Initialize 3-World Cache Engine Arenas
    void* homeArena = AllocateMemoryArena(1); // 1.5GB represented as 1GB for safety in testing
    void* currentEnvArena = AllocateMemoryArena(2);
    
    bool running = true;
    while (running) {
        int events;
        struct android_poll_source* source;
        
        while (ALooper_pollAll(0, nullptr, &events, (void**)&source) >= 0) {
            if (source != nullptr) {
                source->process(app, source);
            }
            if (app->destroyRequested != 0) {
                running = false;
            }
        }
        
        // TODO: OpenXR xrWaitFrame, xrBeginFrame
        // TODO: Vulkan TBDR Draw Calls
        // TODO: xrEndFrame (Submitting composition layers for HUD)
    }

    if (homeArena) madvise(homeArena, 1024 * 1024 * 1024, MADV_DONTNEED);
    if (currentEnvArena) madvise(currentEnvArena, 2LL * 1024 * 1024 * 1024, MADV_DONTNEED);
}

void HandleCmd(struct android_app* app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            LOGI("Window initialized. Booting Omnis OpenXR Compositor.");
            RunEngineLoop(app);
            break;
        case APP_CMD_TERM_WINDOW:
            LOGI("Window terminated.");
            break;
    }
}

// Android NDK Entry Point
void android_main(struct android_app* app) {
    app->onAppCmd = HandleCmd;
    LOGI("Project Omnis Shell booting...");
    
    int events;
    struct android_poll_source* source;
    while (app->destroyRequested == 0) {
        if (ALooper_pollAll(-1, nullptr, &events, (void**)&source) >= 0) {
            if (source != nullptr) {
                source->process(app, source);
            }
        }
    }
}
