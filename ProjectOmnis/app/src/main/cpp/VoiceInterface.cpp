#include "VoiceInterface.h"
#include <android/log.h>
#include <sched.h>
#include <unistd.h>
#include <chrono>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "OmnisEngine_Voice", __VA_ARGS__))

VoiceInterface::VoiceInterface() {}

VoiceInterface::~VoiceInterface() {
    m_isRunning = false;
    if (m_npuListenerThread.joinable()) {
        m_npuListenerThread.join();
    }
}

void VoiceInterface::Initialize() {
    LOGI("Initializing Local AI Voice Interface...");
    
    SetupAudioCapture();
    LoadLocalAIModel();

    m_isRunning = true;
    m_npuListenerThread = std::thread(&VoiceInterface::NpuProcessLoop, this);
}

void VoiceInterface::SetCommandCallback(std::function<void(VoiceCommand, const std::string&)> callback) {
    m_commandCallback = callback;
}

void VoiceInterface::SetupAudioCapture() {
    LOGI("Configuring AAudio Microphone ring-buffer (Low Latency mode)");
}

void VoiceInterface::LoadLocalAIModel() {
    LOGI("Loading Whisper TFLite model. Applying NNAPI/Hexagon DSP Delegate for Snapdragon NPU.");
}

void VoiceInterface::NpuProcessLoop() {
    // [MQ-4] Lock this background listener thread to Core 5 (Performance Core)
    // We keep Core 6 free for rendering, and Core 2 free for UFS disk streaming
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(5, &cpuset);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

    LOGI("[Core 5] Voice NPU Listener Thread Active. Waiting for DSP interrupts...");

    while (m_isRunning) {
        // 1. Read chunk from AAudio ring-buffer
        // 2. Feed into TFLite NNAPI Delegate (Hardware Accelerated via DSP)
        // 3. Wait for NPU interrupt (Simulated here with sleep)
        
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Simulated trigger of the priority "Go Home" command
        static int simCounter = 0;
        simCounter++;
        if (simCounter == 10) { // Simulate voice command after 5 seconds
            LOGI("[Core 5] NPU Recognized Audio: 'System, take me back home'");
            if (m_commandCallback) {
                m_commandCallback(VoiceCommand::GO_HOME, "System, take me back home");
            }
        }
    }
}

void VoiceInterface::Update() {
    // Check for conversational AI text queues to render on the HUD, etc.
}
