#include "nativemodcodeloader.h"

#include <cstring>
#include <fstream>
#include <dlfcn.h>
#include <tml/mod.h>
#include <tml/modloader.h>
#include "fileutil.h"

using namespace tml;

struct ExtractedModInfo_v1 {
    long long timestamp;
    long long sourceTimestamp;
    char sha512[64];
};

bool NativeModCodeLoader::extractIfNeeded(Mod& mod, std::string path, std::string localPath) {
    char sha512[64];
    bool calculatedSha512;
    std::string infoPath = localPath + ".emi";
    // Check if we need to extract the file (it generally will be handled by the hub)
    if (FileUtil::fileExists(localPath) && FileUtil::fileExists(infoPath)) {
        // Some version has already been extracted; check it
        FILE* file = fopen(infoPath.c_str(), "r");
        int version = -1;
        fread(&version, sizeof(int), 1, file);
        if (version == 1) {
            ExtractedModInfo_v1 modInfo;
            if (fread(&modInfo, sizeof(ExtractedModInfo_v1), 1, file) == 1) {
                // first of all, check if timestamp is right
                if (FileUtil::getTimestamp(localPath) == modInfo.timestamp &&
                    mod.getResources().getLastModifyTime(path) == modInfo.sourceTimestamp) {
                    // the timestamps look right - it should be enough - after all we are only using it to make sure
                    // the file gets updated, not to securely protect it
                    return true;
                }
                // calculate SHA512
                char sha512a[64];
                if (FileUtil::calculateSHA512(*mod.getResources().open(path), sha512a) &&
                    FileUtil::calculateSHA512(localPath, sha512) &&
                    memcmp(sha512, modInfo.sha512, sizeof(sha512)) == 0 &&
                    memcmp(sha512, sha512a, sizeof(sha512)) == 0) {
                    // the checksum is correct
                    return true;
                }
            }
        }
    }
    // extract the file!
    calculatedSha512 = FileUtil::calculateSHA512(*mod.getResources().open(path), sha512);
    if (!calculatedSha512) {
        loader.getLog().fatal("Failed to load native mod code '%s' from mod %s (%s)", path.c_str(),
                              mod.getMeta().getName().c_str(), mod.getMeta().getId().c_str());
        return false;
    }
    {
        std::ofstream file(localPath, std::ofstream::binary);
        auto stream = mod.getResources().open(path);
        char buffer[8 * 1024];
        while (stream->good()) {
            stream->read(buffer, sizeof(buffer));
            std::streamsize n = stream->gcount();
            if (n > 0)
                file.write(buffer, n);
        }
    }
    // verify that the file was successfully extracted
    {
        char sha512a[64];
        if (!FileUtil::calculateSHA512(localPath, sha512a) || memcmp(sha512, sha512a, sizeof(sha512)) != 0) {
            loader.getLog().fatal("Failed to extract the file (checksum mismatch)");
            return false;
        }
    }

    // write metadata
    {
        FILE* file = fopen(infoPath.c_str(), "w");
        int metaVersion = 1;
        ExtractedModInfo_v1 metaInfo;
        memcpy(metaInfo.sha512, sha512, sizeof(metaInfo.sha512));
        metaInfo.timestamp = (long long) FileUtil::getTimestamp(localPath);
        metaInfo.sourceTimestamp = mod.getResources().getLastModifyTime(path);
        fwrite(&metaVersion, sizeof(int), 1, file);
        fwrite(&metaInfo, sizeof(ExtractedModInfo_v1), 1, file);
        fflush(file);
        fclose(file);
    }
    return true;
}

std::unique_ptr<ModLoadedCode> NativeModCodeLoader::loadCode(Mod& mod, std::string path) {
    // possible formats: native/ARCH/libPATH.so native/ARCH/PATH.so native/ARCH/PATH
#ifdef __i386
    std::string prefix = "native/x86/";
#else
    std::string prefix = "native/armeabi-v7a/";
#endif
    if (mod.getResources().contains(prefix + path))
        path = prefix + "lib" + path + ".so";
    else if (mod.getResources().contains(prefix + path + ".so"))
        path = prefix + path + ".so";
    else if (mod.getResources().contains(prefix + "lib" + path + ".so"))
        path = prefix + "lib" + path + ".so";
    else {
        loader.getLog().error("Cannot find native mod code '%s' from mod %s (%s)", path.c_str(),
                              mod.getMeta().getName().c_str(), mod.getMeta().getId().c_str());
        return std::unique_ptr<ModLoadedCode>();
    }
    loader.getLog().info("Loading native mod code '%s' from mod %s (%s)", path.c_str(), mod.getMeta().getName().c_str(),
                         mod.getMeta().getId().c_str());
    std::string localPath =
            libsPrivatePath + "/" + mod.getMeta().getId() + "/" + mod.getMeta().getVersion().toString() + "/" + path;
    FileUtil::createDirs(FileUtil::getParent(localPath));
    if (!extractIfNeeded(mod, path, localPath))
        return std::unique_ptr<ModLoadedCode>();
    loader.getLog().trace("Loading native mod code: %s", localPath.c_str());
    void* lib = dlopen(localPath.c_str(), RTLD_LAZY);
    if (lib == nullptr) {
        loader.getLog().error("Failed to load native mod code: %s", dlerror());
        return std::unique_ptr<ModLoadedCode>();
    }
    return std::unique_ptr<ModLoadedCode>(new NativeModLoadedCode(mod, lib));
}

void NativeModLoadedCode::init() {
    int (* initSym)(Mod&) = (int (*)(Mod&)) dlsym(lib, "tml_init");
    if (initSym != nullptr)
        initSym(mod);
}

void NativeModLoadedCode::onMinecraftInitialized(MinecraftClient* minecraft) {
    int (* mcSym)(Mod&, MinecraftClient*) = (int (*)(Mod&, MinecraftClient*)) dlsym(lib, "tml_mcinit");
    if (mcSym != nullptr)
        mcSym(mod, minecraft);
}