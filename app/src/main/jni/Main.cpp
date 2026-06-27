#include <list>
#include <vector>
#include <cstring>
#include <pthread.h>
#include <thread>
#include <cstring>
#include <string>
#include <jni.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <dlfcn.h>
#include <sys/stat.h>
#include "Includes/Logger.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.hpp"
#include "Menu/Menu.hpp"
#include "Menu/Jni.hpp"
#include "Includes/Macros.h"
#include "dobby.h"

// Do not change or translate the first text unless you know what you are doing
// Assigning feature numbers is optional. Without it, it will automatically count for you, starting from 0
// ButtonLink, Category, RichTextView and RichWebView is not counted. They can't have feature number assigned

jobjectArray GetFeatureList(JNIEnv *env, jobject context) {
    jobjectArray ret;

    const char *features[] = {
            OBFUSCATE("Button_Backup Inventory"),  // featNum 0
            OBFUSCATE("Button_Load Backup"),        // featNum 1
    };

    int Total_Feature = (sizeof features / sizeof features[0]);
    ret = (jobjectArray)
            env->NewObjectArray(Total_Feature, env->FindClass(OBFUSCATE("java/lang/String")),
                                env->NewStringUTF(""));

    for (int i = 0; i < Total_Feature; i++)
        env->SetObjectArrayElement(ret, i, env->NewStringUTF(features[i]));

    return (ret);
}

// ===== Hybrid Animals Inventory Backup/Restore =====
static bool copyFile(const char *src, const char *dst) {
    std::ifstream in(src, std::ios::binary);
    if (!in) return false;
    std::ofstream out(dst, std::ios::binary);
    if (!out) return false;
    out << in.rdbuf();
    return in && out;
}

static void backupInventory() {
    const char *slots[] = {"Slot_0", "Slot_1", "Slot_2"};
    const char *base = "/data/data/com.abstractsoft.hybridanimals/files/";
    std::string backupDir = std::string(base) + "backup/";
    mkdir(backupDir.c_str(), 0777);
    for (const char *slot : slots) {
        std::string src = std::string(base) + slot + "/the_inventory";
        std::string dst = backupDir + slot + "_the_inventory";
        bool ok = copyFile(src.c_str(), dst.c_str());
        LOGI("Backup %s: %s", slot, ok ? "OK" : "FAILED");
    }
}

static void loadBackup() {
    const char *slots[] = {"Slot_0", "Slot_1", "Slot_2"};
    const char *base = "/data/data/com.abstractsoft.hybridanimals/files/";
    std::string backupDir = std::string(base) + "backup/";
    for (const char *slot : slots) {
        std::string src = backupDir + slot + "_the_inventory";
        std::string dst = std::string(base) + slot + "/the_inventory";
        bool ok = copyFile(src.c_str(), dst.c_str());
        LOGI("Restore %s: %s", slot, ok ? "OK" : "FAILED");
    }
}
// ===== End Backup/Restore =====

//Target main lib here
#define targetLibName OBFUSCATE("libil2cpp.so")

void Changes(JNIEnv *env, jclass clazz, jobject obj, jint featNum, jstring featName, jint value, jlong Lvalue, jboolean boolean, jstring text) {
    switch (featNum) {
        case 0: // Backup Inventory
            backupInventory();
            break;
        case 1: // Load Backup
            loadBackup();
            break;
        default:
            break;
    }
}

// we will run our hacks in a new thread so our while loop doesn't block process main thread
void hack_thread() {
    // This loop should be always enabled in unity game
    // because libil2cpp.so is not loaded into memory immediately.
    while (!isLibraryLoaded(targetLibName)) {
        sleep(1); // Wait for target lib be loaded.
    }

#if defined(__aarch64__)
    //Put your ARM64 hooks here if needed in the future
#elif defined(__arm__)
    //Put your ARMv7 hooks here if needed in the future
#endif

    LOGI(OBFUSCATE("Done"));
}

// Functions with `__attribute__((constructor))` are executed immediately when System.loadLibrary("lib_name") is called.
__attribute__((constructor))
void lib_main() {
    // Create a new thread so it does not block the main thread, means the game would not freeze
    std::thread(hack_thread).detach();
}
