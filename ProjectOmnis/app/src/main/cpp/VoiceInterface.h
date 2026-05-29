#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <functional>

// Represents recognized commands from the local STT model
enum class VoiceCommand {
    NONE,
    GO_HOME,
    OPEN_INVENTORY,
    SPAWN_SCREEN
};

class VoiceInterface {
public:
    VoiceInterface();
    ~VoiceInterface();

    void Initialize();
    void SetCommandCallback(std::function<void(VoiceCommand, const std::string&)> callback);

    // Polls the AI bot's latest state (called from main thread)
    void Update();

private:
    std::atomic<bool> m_isRunning{false};
    std::thread m_npuListenerThread;
    
    std::function<void(VoiceCommand, const std::string&)> m_commandCallback;

    // AAudio/Oboe Audio Stream (Stubbed)
    // AAudioStream* m_audioStream;

    // TFLite / NNAPI Interpreter (Stubbed)
    // TfLiteInterpreter* m_interpreter;

    void NpuProcessLoop();
    void SetupAudioCapture();
    void LoadLocalAIModel();
};
