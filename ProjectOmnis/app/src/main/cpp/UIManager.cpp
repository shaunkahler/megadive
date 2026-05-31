#include "UIManager.h"
#include <android/log.h>
#include <android_native_app_glue.h>
#include <jni.h>
#include <cmath>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "OmnisEngine", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "OmnisEngine", __VA_ARGS__))

UIManager::UIManager() : m_headSpace(0), m_worldSpace(0), m_hudSwapchain(0) {}

UIManager::~UIManager() {}

void UIManager::Initialize(XrSpace headSpace, XrSpace worldSpace) {
    LOGI("Initializing Spatial UI / HUD Manager...");
    m_headSpace = headSpace;
    m_worldSpace = worldSpace;
    
    CreateHUD();
    InitializeLauncherMenu();
}

void UIManager::InitializeLauncherMenu() {
    LOGI("Populating Spatial Home Launcher App Shortcuts...");
    
    // We will place 4 floating 3D menus side-by-side in front of the user (e.g. 1.2 meters away, 1.0 meter high)
    const char* appNames[] = { "Meta Browser", "Settings", "System Shell", "MegaDive MMO" };
    const char* packages[] = { "com.oculus.browser", "com.android.settings", "com.oculus.vrshell", "com.megadive.mmo" };
    float colors[][3] = {
        {0.0f, 0.4f, 1.0f}, // Blue
        {0.0f, 0.8f, 0.2f}, // Green
        {1.0f, 0.6f, 0.0f}, // Orange
        {0.9f, 0.1f, 0.1f}  // Red
    };

    for (int i = 0; i < 4; ++i) {
        MenuButton btn;
        btn.label = appNames[i];
        btn.packageName = packages[i];
        
        // Centered straight ahead, closer (1.2 meters away) and tighter spacing for comfortable selection
        btn.pose.position = { -0.375f + i * 0.25f, 1.0f, -1.2f };
        btn.pose.orientation = { 0.0f, 0.0f, 0.0f, 1.0f }; // Identity (facing straight forward)
        
        btn.size[0] = 0.22f; // Width (22cm)
        btn.size[1] = 0.13f; // Height (13cm)
        btn.size[2] = 0.04f; // Depth (4cm)
        
        btn.color[0] = colors[i][0];
        btn.color[1] = colors[i][1];
        btn.color[2] = colors[i][2];
        
        btn.hovered = false;
        btn.triggered = false;
        
        m_menuButtons.push_back(btn);
    }
}

static void LaunchAndroidPackage(struct android_app* app, const char* packageName) {
    LOGI("Attempting bare-metal launch of package: %s", packageName);
    JNIEnv* env = nullptr;
    app->activity->vm->AttachCurrentThread(&env, nullptr);
    if (!env) {
        LOGE("Failed to attach thread to Java VM for launching %s", packageName);
        return;
    }

    jclass activityClass = env->GetObjectClass(app->activity->clazz);
    jmethodID getPackageManagerMethod = env->GetMethodID(activityClass, "getPackageManager", "()Landroid/content/pm/PackageManager;");
    jobject packageManagerObj = env->CallObjectMethod(app->activity->clazz, getPackageManagerMethod);

    jclass pmClass = env->GetObjectClass(packageManagerObj);
    jmethodID getLaunchIntentMethod = env->GetMethodID(pmClass, "getLaunchIntentForPackage", "(Ljava/lang/String;)Landroid/content/Intent;");

    jstring pkgString = env->NewStringUTF(packageName);
    jobject intentObj = env->CallObjectMethod(packageManagerObj, getLaunchIntentMethod, pkgString);

    if (intentObj != nullptr) {
        jmethodID startActivityMethod = env->GetMethodID(activityClass, "startActivity", "(Landroid/content/Intent;)V");
        env->CallVoidMethod(app->activity->clazz, startActivityMethod, intentObj);
        LOGI("JNI Launch Intent triggered successfully for %s", packageName);
    } else {
        LOGE("Could not locate launch Intent for package: %s (Is it installed?)", packageName);
    }

    app->activity->vm->DetachCurrentThread();
}

void UIManager::UpdateMenu(float deltaTime, 
                          const XrHandJointLocationEXT* leftJoints, bool leftActive, 
                          const XrHandJointLocationEXT* rightJoints, bool rightActive, 
                          void* androidApp) {
    struct android_app* app = (struct android_app*)androidApp;

    // 1. Detect if Right index finger tip touches Left wrist watch area to toggle menu
    if (leftActive && rightActive && leftJoints && rightJoints) {
        float dx = rightJoints[10].pose.position.x - leftJoints[0].pose.position.x;
        float dy = rightJoints[10].pose.position.y - leftJoints[0].pose.position.y;
        float dz = rightJoints[10].pose.position.z - leftJoints[0].pose.position.z;
        float dist = sqrtf(dx*dx + dy*dy + dz*dz);
        
        // Check if both joints are tracked away from the coordinate origin {0,0,0}
        // This prevents uninitialized hands from triggering a fake touch on frame 1
        float lenLeft = sqrtf(leftJoints[0].pose.position.x*leftJoints[0].pose.position.x + 
                              leftJoints[0].pose.position.y*leftJoints[0].pose.position.y + 
                              leftJoints[0].pose.position.z*leftJoints[0].pose.position.z);
        
        bool jointsTracked = (lenLeft > 0.01f);

        // Log distance when they get reasonably close to help visualize and calibrate
        if (jointsTracked && dist < 0.35f) {
            static int logThrottle = 0;
            if (logThrottle++ % 60 == 0) { // log once per second at 60fps
                LOGI("[Menu Calibration] Right index to Left wrist distance: %.3fm (Trigger threshold: 0.15m)", dist);
            }
        }

        bool wristTouched = jointsTracked && (dist < 0.15f); // Expanded touch radius to 15cm to comfortably cover forearm skin surface
        if (wristTouched && !m_wristTouchedLastFrame) {
            m_menuVisible = !m_menuVisible;
            LOGI("Wrist watch touch detected! Toggled far menu visibility to: %s", m_menuVisible ? "VISIBLE" : "HIDDEN");
        }
        m_wristTouchedLastFrame = wristTouched;
    }

    // If invisible, hover/click actions are dormant
    if (!m_menuVisible) {
        for (auto& btn : m_menuButtons) {
            btn.hovered = false;
            btn.triggered = false;
        }
        return;
    }

    // 2. Hand Pointer Raycasting (Right wrist joint 0 -> right index proximal joint 7)
    bool isPointing = false;
    float rayOrigin[3] = {0,0,0};
    float rayDir[3] = {0,0,-1};
    
    if (rightActive && rightJoints) {
        if ((rightJoints[0].locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
            (rightJoints[7].locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0) {
            
            rayOrigin[0] = rightJoints[0].pose.position.x;
            rayOrigin[1] = rightJoints[0].pose.position.y;
            rayOrigin[2] = rightJoints[0].pose.position.z;
            
            float rx = rightJoints[7].pose.position.x - rayOrigin[0];
            float ry = rightJoints[7].pose.position.y - rayOrigin[1];
            float rz = rightJoints[7].pose.position.z - rayOrigin[2];
            float rlen = sqrtf(rx*rx + ry*ry + rz*rz);
            if (rlen > 0.001f) {
                rayDir[0] = rx / rlen;
                rayDir[1] = ry / rlen;
                rayDir[2] = rz / rlen;
                isPointing = true;
            }
        }
    }

    // 3. Pinch-to-Select Detection (Right Thumb joint 5 <-> Right Index joint 10)
    bool isPinching = false;
    if (rightActive && rightJoints) {
        if ((rightJoints[5].locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
            (rightJoints[10].locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0) {
            
            float pdx = rightJoints[10].pose.position.x - rightJoints[5].pose.position.x;
            float pdy = rightJoints[10].pose.position.y - rightJoints[5].pose.position.y;
            float pdz = rightJoints[10].pose.position.z - rightJoints[5].pose.position.z;
            float pdist = sqrtf(pdx*pdx + pdy*pdy + pdz*pdz);
            isPinching = (pdist < 0.025f); // 2.5cm thumb-to-index pinch distance
        }
    }

    bool pinchTriggered = isPinching && !m_rightHandPinchingLastFrame;
    m_rightHandPinchingLastFrame = isPinching;

    // 4. Ray-Sphere Collision Testing for Bounding Target Volumes (25cm radii)
    for (auto& btn : m_menuButtons) {
        bool isHovered = false;
        
        if (isPointing) {
            float vx = btn.pose.position.x - rayOrigin[0];
            float vy = btn.pose.position.y - rayOrigin[1];
            float vz = btn.pose.position.z - rayOrigin[2];
            
            float t = vx*rayDir[0] + vy*rayDir[1] + vz*rayDir[2];
            if (t > 0.0f) {
                float px = rayOrigin[0] + t * rayDir[0];
                float py = rayOrigin[1] + t * rayDir[1];
                float pz = rayOrigin[2] + t * rayDir[2];
                
                float cdx = px - btn.pose.position.x;
                float cdy = py - btn.pose.position.y;
                float cdz = pz - btn.pose.position.z;
                float distSq = cdx*cdx + cdy*cdy + cdz*cdz;
                
                if (distSq < (0.28f * 0.28f)) { // Generous 28cm sphere target for seamless far selection
                    isHovered = true;
                }
            }
        }

        if (isHovered) {
            if (!btn.hovered) {
                btn.hovered = true;
                LOGI("Hand Point Hover Enter: '%s'", btn.label.c_str());
            }
            
            if (pinchTriggered) {
                LOGI("Pinch Gesture Trigger: Launching package: %s", btn.packageName.c_str());
                LaunchAndroidPackage(app, btn.packageName.c_str());
            }
        } else {
            btn.hovered = false;
        }
    }
}

void UIManager::CreateHUD() {
    LOGI("Constructing Always-On Visor HUD Composition Layer...");
    // The HUD is locked 0.5 meters directly in front of the user's face.
    // By using m_headSpace, it perfectly tracks with the head hardware, bypassing Vulkan rendering lag.
    m_hudPose.position = {0.0f, 0.0f, -0.5f}; 
    m_hudPose.orientation = {0.0f, 0.0f, 0.0f, 1.0f}; // Identity rotation
    m_hudSize = {0.8f, 0.4f}; // 80cm wide, 40cm tall

    // TODO: xrCreateSwapchain for m_hudSwapchain
}

uint32_t UIManager::SpawnFloatingScreen(XrPosef initialPose, float width, float height) {
    LOGI("Spawning new Infinite Multi-Screen (ID: %d)", m_nextScreenId);
    
    VirtualScreen screen;
    screen.id = m_nextScreenId++;
    screen.pose = initialPose;
    screen.size = {width, height};
    screen.isVisible = true;
    
    // TODO: xrCreateSwapchain to map an Android SurfaceTexture or Chromium Webview here
    screen.swapchain = 0; 

    m_virtualScreens.push_back(screen);
    return screen.id;
}

void UIManager::MoveScreen(uint32_t screenId, XrPosef newPose) {
    for (auto& screen : m_virtualScreens) {
        if (screen.id == screenId) {
            screen.pose = newPose;
            break;
        }
    }
}

void UIManager::Update(float deltaTime) {
    // Logic to animate UI elements, handle drag-and-drop physics for floating screens, etc.
}

void* UIManager::GetCompositionLayersStub() {
    // In a full implementation, this constructs:
    // 1. XrCompositionLayerQuad for the Visor HUD (using m_headSpace)
    // 2. XrCompositionLayerQuad for EVERY floating screen (using m_worldSpace)
    // 
    // Example construction per layer:
    // XrCompositionLayerQuad quad{XR_TYPE_COMPOSITION_LAYER_QUAD};
    // quad.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
    // quad.space = (isHud) ? m_headSpace : m_worldSpace;
    // quad.pose = screen.pose;
    // quad.size = screen.size;
    // quad.subImage.swapchain = screen.swapchain;

    return nullptr; // Return vector of XrCompositionLayerBaseHeader*
}

#include "Math.h"

void UIManager::PositionMenuInFrontOf(XrPosef headPose) {
    LOGI("Head view located. Positioned menu buttons dynamically in front of the user.");

    Matrix4x4 headMat;
    CreatePoseMatrix(&headMat, headPose);

    // Column 0 is Right, Column 2 is Backward/Forward
    float right[3] = { headMat.m[0], headMat.m[1], headMat.m[2] };
    float forward[3] = { -headMat.m[8], -headMat.m[9], -headMat.m[10] };

    // Project forward onto horizontal (XZ) plane so buttons don't fly up/down
    float horizontalLen = sqrtf(forward[0]*forward[0] + forward[2]*forward[2]);
    if (horizontalLen > 0.001f) {
        forward[0] /= horizontalLen;
        forward[1] = 0.0f; // Lock height level to head level
        forward[2] /= horizontalLen;
    } else {
        forward[0] = 0.0f;
        forward[1] = 0.0f;
        forward[2] = -1.0f;
    }

    // Recalculate horizontal right vector
    right[0] = -forward[2];
    right[1] = 0.0f;
    right[2] = forward[0];

    // Place menu 2.5 meters directly in front of the user's head, slightly lower at height (chest level)
    float centerX = headPose.position.x + forward[0] * 2.5f;
    float centerY = headPose.position.y - 0.20f; // 20cm below eye level
    float centerZ = headPose.position.z + forward[2] * 2.5f;

    for (size_t i = 0; i < m_menuButtons.size(); ++i) {
        float offset = -0.75f + i * 0.50f; // wider horizontal spacing spread for far distance
        m_menuButtons[i].pose.position.x = centerX + right[0] * offset;
        m_menuButtons[i].pose.position.y = centerY;
        m_menuButtons[i].pose.position.z = centerZ + right[2] * offset;
        
        // Face the user
        m_menuButtons[i].pose.orientation = headPose.orientation;
    }
}
