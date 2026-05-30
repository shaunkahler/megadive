plugins {
    id("com.android.application")
}

android {
    namespace = "com.projectomnis.shell"
    compileSdk = 34
    // NDK 26 is required for API 34 support
    ndkVersion = "26.1.10909125"

    defaultConfig {
        applicationId = "com.projectomnis.shell"
        minSdk = 34
        targetSdk = 34
        versionCode = 1
        versionName = "1.0"

        externalNativeBuild {
            cmake {
                cppFlags += "-std=c++20"
                // Ensure the native platform matches the library requirements
                arguments("-DANDROID_PLATFORM=android-34")
                abiFilters("arm64-v8a")
            }
        }
        
        ndk {
            abiFilters.add("arm64-v8a")
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
        }
    }
    
    externalNativeBuild {
        cmake {
            path("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    buildFeatures {
        prefab = true
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_1_8
        targetCompatibility = JavaVersion.VERSION_1_8
    }
}

dependencies {
    implementation("org.khronos.openxr:openxr_loader_for_android:1.0.34")
    implementation("com.meta.horizonos:horizon-os-nsdk:204")
}
