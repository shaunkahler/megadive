# MegaDive OS 🌌

**A Standalone Spatial OS Architecture for Meta Quest 3 (Snapdragon XR2 Gen 2)**

MegaDive is an experimental, bare-metal native Android/Linux application designed to operate as a primary Shell interface, completely bypassing the standard Meta Quest Horizon OS. Built strictly in C++ using the Android NDK and designed for OpenXR, MegaDive focuses on maximizing thermal efficiency, aggressive memory optimization, and zero-latency asynchronous rendering on resource-constrained mobile hardware.

## 🚀 Vision

To provide a seamless, massive-world MMO and productivity environment natively on standalone VR/MR headsets. By dropping Java/Kotlin overhead and interfacing directly with the Linux kernel and OpenXR runtime, MegaDive extracts maximum performance from the Snapdragon XR2 Gen 2 architecture.

## 🧠 Core Architecture & Features (Current Scaffold)

*   **OS Override (Home Launcher):** Configured via `AndroidManifest.xml` to act as an Android Home Intent, dropping the user directly into the MegaDive shell upon boot.
*   **Bare-Metal Execution:** Utilizes `android_native_app_glue` to run entirely in C++, bypassing the Dalvik/ART virtual machine to eliminate garbage collection stutters.
*   **Aggressive Power Management:** Requests Meta-specific permissions (`com.oculus.permission.SET_CPU_GPU_LEVEL`) to command Level 5 (Maximum Sustained) clock speeds during heavy portal loads.
*   **3-World Cache Engine (Memory Arenas):** Uses low-level Linux `mmap` to pre-allocate gigabyte-scale memory arenas (Home, Active, Staging). Utilizes `madvise(MADV_DONTNEED)` to gracefully return pages to the OS, evading the Android Low Memory Killer (LMK) during massive world transitions.
*   **Strict CPU Core Affinity:** Physically locks the main engine loop to the Snapdragon Prime Core (Cortex-X4) via `sched_setaffinity` to prevent the OS governor from throttling the render thread.

## 🛠️ Prerequisites & Build Setup

Currently, this repository contains the foundational C++ kernel overrides and build scaffolding. To compile the engine into a functional APK, you will need:

1.  **Android Studio** (Flamingo or newer recommended)
2.  **Android NDK** (Side-by-side installation via SDK Manager)
3.  **CMake** (Version 3.22.1+)
4.  **Meta OpenXR Mobile SDK:** You must download the proprietary Meta OpenXR SDK and link `libopenxr_loader.so` in `CMakeLists.txt`.

### Build Instructions

1. Clone the repository.
2. Open the `ProjectMegaDive` directory in Android Studio.
3. Ensure your NDK path is configured correctly in `local.properties`.
4. Drop the Meta OpenXR Mobile SDK headers into `app/src/main/cpp/include` and the `.so` binaries into your `jniLibs` folder.
5. Uncomment the OpenXR includes and linker flags in `main.cpp` and `CMakeLists.txt`.
6. Build and deploy to your Quest device via ADB.

*(Note: To test the Home Override functionality, you must sideload the APK and use ADB to set it as the default launcher, or launch it directly via `adb shell am start`.)*

## 🗺️ Roadmap: The Path Forward

*   [ ] **Phase 1: OpenXR Boilerplate** - Initialize `xrCreateInstance` and `xrCreateSession`, and establish the render loop.
*   [ ] **Phase 2: Vulkan TBDR Pipeline** - Implement a Tile-Based Deferred Renderer using Vulkan Subpasses to keep G-Buffers in ultra-fast on-chip SRAM, mitigating thermal throttling.
*   [ ] **Phase 3: Always-On HUD** - Implement OpenXR Composition Layers (`XrCompositionLayerQuad`) to render the system UI completely independent of the world geometry pass.
*   [ ] **Phase 4: DSP Voice Interface** - Offload Whisper STT to the Hexagon NPU/DSP to free up primary CPU cores.

## 📜 License

MIT License. See `LICENSE` for details.
