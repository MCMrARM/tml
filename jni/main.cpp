#include <jni.h>
#include <android/log.h>
#include <dlfcn.h>
#include <tml/modloader.h>

using namespace tml;

JavaVM* javaVM;
void* nativeLibMCPE;
std::unique_ptr<ModLoader> modLoader;

extern "C" {

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    javaVM = vm;

    JNIEnv* env;
    int envStatus = vm->GetEnv((void**) &env, JNI_VERSION_1_6);
    if (envStatus == JNI_EDETACHED) {
        vm->AttachCurrentThread(&env, nullptr);
    }

    nativeLibMCPE = dlopen("libminecraftpe.so", RTLD_LAZY);

    return JNI_VERSION_1_2;

}

JNIEXPORT void JNICALL Java_io_mrarm_tml_TML_nativeLoadMod(JNIEnv* env, jclass cl, jstring internalDir) {
    //
}

__attribute__ ((visibility ("default"))) void mod_init() {
    modLoader = std::unique_ptr<ModLoader>(new ModLoader("."));
}

}
