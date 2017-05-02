#pragma once

#include <string>
#include <vector>
#include <tml/modcodeloader.h>

namespace tml {

class ModLoader;

class NativeModLoadedCode : public ModLoadedCode {

protected:
    void* lib;

public:
    NativeModLoadedCode(Mod& mod, void* lib);

    virtual ~NativeModLoadedCode() { }
    virtual void init();
    virtual void onMinecraftInitialized(MinecraftClient* minecraft);

};

class NativeModCodeLoader : public ModCodeLoader {

private:
    ModLoader& loader;
    std::string libsPrivatePath;

public:
    NativeModCodeLoader(ModLoader& loader, std::string libsPrivatePath) : loader(loader),
                                                                          libsPrivatePath(libsPrivatePath) { }

    virtual ~NativeModCodeLoader() { }

    virtual std::unique_ptr<ModLoadedCode> loadCode(Mod& mod, std::string path);

    bool extractIfNeeded(Mod& mod, std::string path, std::string localPath);

};

}
