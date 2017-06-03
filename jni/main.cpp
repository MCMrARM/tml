#include <jni.h>
#include <android/log.h>
#include <dlfcn.h>
#include <unistd.h>
#include <tml/mod.h>
#include <tml/modloader.h>
#include <android/asset_manager_jni.h>

using namespace tml;

JavaVM* javaVM;
std::unique_ptr<ModLoader> modLoader;

static std::string jniString(JNIEnv* env, jstring str) {
    const char* cstr = env->GetStringUTFChars(str, NULL);
    std::string ret (cstr);
    env->ReleaseStringUTFChars(str, cstr);
    return ret;
}

extern "C" {

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    javaVM = vm;

    JNIEnv* env;
    int envStatus = vm->GetEnv((void**) &env, JNI_VERSION_1_6);
    if (envStatus == JNI_EDETACHED) {
        vm->AttachCurrentThread(&env, nullptr);
    }

    return JNI_VERSION_1_2;

}

JNIEXPORT void JNICALL Java_io_mrarm_mctoolbox_tml_TMLImplementation_nativeLoadTML(JNIEnv* env, jclass cl, jstring internalDir) {
    modLoader = std::unique_ptr<ModLoader>(new ModLoader(jniString(env, internalDir)));
}
JNIEXPORT void JNICALL Java_io_mrarm_mctoolbox_tml_TMLImplementation_nativeAddAllModsFromDir(JNIEnv* env, jclass cl, jstring dir) {
    modLoader->addAllModsFromDirectory(jniString(env, dir));
}
JNIEXPORT void JNICALL Java_io_mrarm_mctoolbox_tml_TMLImplementation_nativeAddAssetMod(JNIEnv* env, jclass cl, jstring mod) {
    modLoader->addModFromAssets(jniString(env, mod));
}
JNIEXPORT void JNICALL Java_io_mrarm_mctoolbox_tml_TMLImplementation_nativeLoadMods(JNIEnv* env, jclass cl) {
    modLoader->resolveDependenciesAndLoad();
}
JNIEXPORT void JNICALL Java_io_mrarm_mctoolbox_tml_TMLImplementation_nativeSetModAssetManager(JNIEnv* env, jclass cl,
                                                                                           jobject assetMgr, jlong lastModifyTime) {
    modLoader->setAndroidAssetManager(AAssetManager_fromJava(env, assetMgr), lastModifyTime);
}
JNIEXPORT void JNICALL Java_io_mrarm_mctoolbox_tml_TMLImplementation_nativeSetMinecraftAssetManager(JNIEnv* env, jclass cl,
                                                                                           jobject assetMgr) {
    void* lib = dlopen("libassetpatch.so", RTLD_LAZY);
    void (*func)(JNIEnv*, jobject) = (void (*)(JNIEnv*, jobject)) dlsym(lib, "tml_set_assetmanager");
    func(env, assetMgr);
}

__attribute__ ((visibility ("default"))) void mod_init() {
    char cwd[512];
    getcwd(cwd, sizeof(cwd));

    modLoader = std::unique_ptr<ModLoader>(new ModLoader(std::string(cwd) + "/tml/"));
    modLoader->addAllModsFromDirectory("tml/mods/");
    modLoader->resolveDependenciesAndLoad();
}

}
