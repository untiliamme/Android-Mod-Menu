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
#include "Includes/Logger.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.hpp"
#include "Menu/Menu.hpp"
#include "Menu/Jni.hpp"
#include "Includes/Macros.h"

bool autoPlay = false;

struct MemPatches {
    MemoryPatch ; //noDeath
} gPatches;


jobjectArray GetFeatureList(JNIEnv *env, jobject context) {
    jobjectArray ret;

    const char *features[] = {
            OBFUSCATE("Category_Auto Play"),
            OBFUSCATE("Toggle_Auto"),
    };

    int Total_Feature = (sizeof features / sizeof features[0]);
    ret = (jobjectArray)
            env->NewObjectArray(Total_Feature, env->FindClass(OBFUSCATE("java/lang/String")),
                                env->NewStringUTF(""));

    for (int i = 0; i < Total_Feature; i++)
        env->SetObjectArrayElement(ret, i, env->NewStringUTF(features[i]));

    return (ret);
}

bool btnPressed = false;

void Changes(JNIEnv *env, jclass clazz, jobject obj, jint featNum, jstring featName, jint value, jlong Lvalue, jboolean boolean, jstring text) {

    switch (featNum) {
        case 0:
        {
            autoPlay = boolean;
            break;
        }
        case 1:
            btnPressed = true;
            break;
    }
}

//CharacterPlayer
void (*StartInvcibility)(void *instance, float duration);

void (*old_Update)(void *instance);

void Update(void *instance) {
    if (instance != nullptr) {
        if (btnPressed) {
            StartInvcibility(instance, 30);
            btnPressed = false;
        }
    }
    return old_Update(instance);
}

bool (*old_IsautoPlay)(void *instance);

bool IsautoPlay(void *instance) {
    if (instance != NULL) {
        return true;
    }
    return old_IsautoPlay(instance);
}

//Target lib here
#define targetLibName OBFUSCATE("libil2cpp.so")

ElfScanner g_il2cppELF;

// we will run our hacks in a new thread so our while loop doesn't block process main thread
void *hack_thread(void *) {
    LOGI(OBFUSCATE("pthread created"));

    //Check if target lib is loaded
    /*do {
        sleep(1);
    } while (!isLibraryLoaded(targetLibName));*/

    do {
        sleep(1);
        // getElfBaseMap can also find lib base even if it was loaded from zipped base.apk
        g_il2cppELF = ElfScanner::createWithPath(targetLibName);
    } while (!g_il2cppELF.isValid());

    LOGI(OBFUSCATE("%s has been loaded"), (const char *) targetLibName);

#if defined(__aarch64__)
    uintptr_t il2cppBase = g_il2cppELF.base();

    //Il2Cpp: Use RVA offset
    StartInvcibility = (void (*)(void *, float)) getAbsoluteAddress(targetLibName, str2Offset(
            OBFUSCATE("0x107A3BC")));

    MsHookFunction((void*)getAbsoluteAddress(0x332EBC4), (void*)AutoPlay, (void**)&old_AutoPlay);

#elif defined(__arm__)
    //Put your code here if you want the code to be compiled for armv7 only
#endif

    LOGI(OBFUSCATE("Done"));
    return nullptr;
}

__attribute__((constructor))
void lib_main() {
    // Create a new thread so it does not block the main thread, means the game would not freeze
    pthread_t ptid;
    pthread_create(&ptid, NULL, hack_thread, NULL);
}