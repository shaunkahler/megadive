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
    
    // We will place 1 floating 3D menu directly in front of the user
    MenuButton btn;
    btn.label = "MegaDive MMO";
    btn.description = "Play MegaDive";
    btn.packageName = "com.megadive.mmo";
    
    // Centered straight ahead, right in front of the user (0.6 meters away). Y=0.0f is eye-level since reference space is LOCAL.
    btn.pose.position = { 0.0f, -0.1f, -0.6f };
    btn.pose.orientation = { 0.0f, 0.0f, 0.0f, 1.0f }; // Identity (facing straight forward)
    
    // Use UNIFORM scaling so the 3D model doesn't get squished!
    // We'll scale it evenly across X, Y, and Z. 
    btn.size[0] = 0.15f; // Width
    btn.size[1] = 0.15f; // Height
    btn.size[2] = 0.15f; // Depth
    
    btn.color[0] = 0.0f; // R
    btn.color[1] = 0.4f; // G
    btn.color[2] = 1.0f; // B (Blueish)
    
    btn.hovered = false;
    btn.triggered = false;
    
    m_menuButtons.push_back(btn);
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
                          void* androidApp, XrPosef headPose) {
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
            if (m_menuVisible) {
                PositionMenuInFrontOf(headPose);
            }
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

    // 2. Hand Pointer Raycasting (Right middle proximal joint 12 -> right middle tip 15)
    m_isPointing = false;
    bool isIndexFlexed = false;
    bool clickTriggered = false;
    
    if (rightActive && rightJoints) {
        if ((rightJoints[12].locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
            (rightJoints[15].locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0) {
            
            m_pointerOrigin[0] = rightJoints[12].pose.position.x;
            m_pointerOrigin[1] = rightJoints[12].pose.position.y;
            m_pointerOrigin[2] = rightJoints[12].pose.position.z;
            
            float rx = rightJoints[15].pose.position.x - m_pointerOrigin[0];
            float ry = rightJoints[15].pose.position.y - m_pointerOrigin[1];
            float rz = rightJoints[15].pose.position.z - m_pointerOrigin[2];
            float rlen = sqrtf(rx*rx + ry*ry + rz*rz);
            if (rlen > 0.001f) {
                m_pointerDir[0] = rx / rlen;
                m_pointerDir[1] = ry / rlen;
                m_pointerDir[2] = rz / rlen;
                m_isPointing = true;
            }
        }

        // --- Index Proximal (Base) Flex Click Gesture ---
        if ((rightJoints[6].locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
            (rightJoints[7].locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
            (rightJoints[8].locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0) {
            
            // Palm segment: Metacarpal (6) to Proximal (7)
            float dx_palm = rightJoints[7].pose.position.x - rightJoints[6].pose.position.x;
            float dy_palm = rightJoints[7].pose.position.y - rightJoints[6].pose.position.y;
            float dz_palm = rightJoints[7].pose.position.z - rightJoints[6].pose.position.z;
            float len_palm = sqrtf(dx_palm*dx_palm + dy_palm*dy_palm + dz_palm*dz_palm);

            // Base finger segment: Proximal (7) to Intermediate (8)
            float dx_base = rightJoints[8].pose.position.x - rightJoints[7].pose.position.x;
            float dy_base = rightJoints[8].pose.position.y - rightJoints[7].pose.position.y;
            float dz_base = rightJoints[8].pose.position.z - rightJoints[7].pose.position.z;
            float len_base = sqrtf(dx_base*dx_base + dy_base*dy_base + dz_base*dz_base);

            if (len_palm > 0.001f && len_base > 0.001f) {
                // Dot product / (len1 * len2) gives cos(theta). 
                float palm_base_dot = (dx_palm*dx_base + dy_palm*dy_base + dz_palm*dz_base) / (len_palm * len_base);

                // Require the bottom joint to be flexed (angle > ~20 deg). Cos(20) is roughly 0.940
                isIndexFlexed = (palm_base_dot < 0.94f);
            }
        }
    }

    // Process the Click State Machine
    if (m_isPointing) {
        if (!m_indexFlexed && isIndexFlexed) {
            m_indexFlexed = true;
            m_indexFlexTimer = 0.0f; // Start flex timer
        } else if (m_indexFlexed) {
            m_indexFlexTimer += deltaTime;
            if (!isIndexFlexed) {
                // Unflexed! Check if it was quick (< 400ms)
                if (m_indexFlexTimer < 0.400f) { 
                    clickTriggered = true;
                    m_clickVisualTimer = 0.2f; // Trigger visual laser flash
                }
                m_indexFlexed = false;
            } else if (m_indexFlexTimer > 0.500f) {
                // Held the flex too long, cancel the click
                m_indexFlexed = false;
            }
        }
    }

    // 4. Ray-Bounding Box Collision Testing for precise targeting
    for (auto& btn : m_menuButtons) {
        bool isHovered = false;
        
        if (m_isPointing) {
            float vx = btn.pose.position.x - m_pointerOrigin[0];
            float vy = btn.pose.position.y - m_pointerOrigin[1];
            float vz = btn.pose.position.z - m_pointerOrigin[2];
            
            float t = vx*m_pointerDir[0] + vy*m_pointerDir[1] + vz*m_pointerDir[2];
            if (t > 0.0f) {
                float px = m_pointerOrigin[0] + t * m_pointerDir[0];
                float py = m_pointerOrigin[1] + t * m_pointerDir[1];
                float pz = m_pointerOrigin[2] + t * m_pointerDir[2];
                
                float cdx = px - btn.pose.position.x;
                float cdy = py - btn.pose.position.y;
                float cdz = pz - btn.pose.position.z;
                
                // Since the menu panels always face the user horizontally (upright),
                // we can perform a local-space bounding box check.
                // X/Z forms the width plane, Y is height.
                float verticalDist = std::abs(cdy);
                float horizontalDist = std::sqrt(cdx*cdx + cdz*cdz);
                
                if (verticalDist < (btn.size[1] * 0.5f) && horizontalDist < (btn.size[0] * 0.5f)) {
                    isHovered = true;
                }
            }
        }

        if (isHovered) {
            if (!btn.hovered) {
                btn.hovered = true;
                LOGI("Hand Point Hover Enter: '%s'", btn.label.c_str());
            }
            
            if (clickTriggered) {
                LOGI("Index Flex Click Trigger: Launching package: %s", btn.packageName.c_str());
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
    if (m_clickVisualTimer > 0.0f) {
        m_clickVisualTimer -= deltaTime;
        // Make it bright green for feedback
        m_laserColor[0] = 0.0f; m_laserColor[1] = 1.0f; m_laserColor[2] = 0.0f; 
    } else {
        // Default Red
        m_laserColor[0] = 1.0f; m_laserColor[1] = 0.0f; m_laserColor[2] = 0.0f; 
    }
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

    // Place menu 0.6 meters directly in front of the user's head, slightly lower at height (chest level)
    float centerX = headPose.position.x + forward[0] * 0.6f;
    float centerY = headPose.position.y - 0.10f; // 10cm below eye level
    float centerZ = headPose.position.z + forward[2] * 0.6f;

    // Calculate a purely horizontal yaw quaternion so the menu is always upright
    // We use -forward[0] because a positive Y rotation rotates -Z to -X.
    float yaw = std::atan2(-forward[0], -forward[2]);
    XrQuaternionf uprightOrientation;
    uprightOrientation.x = 0.0f;
    uprightOrientation.y = std::sin(yaw * 0.5f);
    uprightOrientation.z = 0.0f;
    uprightOrientation.w = std::cos(yaw * 0.5f);

    for (size_t i = 0; i < m_menuButtons.size(); ++i) {
        // Calculate offset to center the items dynamically based on how many exist
        float totalWidth = (m_menuButtons.size() - 1) * 0.15f;
        float offset = -(totalWidth / 2.0f) + (i * 0.15f);
        
        m_menuButtons[i].pose.position.x = centerX + right[0] * offset;
        m_menuButtons[i].pose.position.y = centerY;
        m_menuButtons[i].pose.position.z = centerZ + right[2] * offset;
        
        // Face the user upright
        m_menuButtons[i].pose.orientation = uprightOrientation;
    }
}
