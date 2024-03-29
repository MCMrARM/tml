#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <jni.h>
#include <android/asset_manager.h>
#include "log.h"
#include "modmeta.h"

namespace tml {

class Mod;
class ModResources;
class ModCodeLoader;
class NativeModCodeLoader;
class HookManager;

class ModLoader : public LogPrinter {

private:
    std::map<std::string, std::pair<Mod*, std::unique_ptr<ModCodeLoader>>> loaders;
    std::map<std::string, std::map<ModVersion, std::unique_ptr<Mod>>> mods;
    std::vector<std::pair<Mod*, std::unique_ptr<LogPrinter>>> logPrinters;

    bool loadMod(Mod& mod);
    void initMod(Mod& mod);

protected:
    std::string internalDir;
    std::string modDataStoragePath;
    Log loaderLog;
    HookManager* hookManager;
    void* mcpeLib;
    AAssetManager* assetManager;
    long long assetsLastModifyTime;

    friend class Mod;

    void registerCodeLoader(Mod& ownerMod, std::string name, std::unique_ptr<ModCodeLoader> loader);

    void registerLogPrinter(Mod& ownerMod, std::unique_ptr<LogPrinter> printer);

public:
    static const char* MODLOADER_PKGID;

    ModLoader(std::string internalDir);

    ~ModLoader();

    JavaVM* getJVM() const;

    inline Log& getLog() { return loaderLog; }

    virtual void printLogMessage(LogLevel level, const std::string& tag, const char* msg, va_list va);

    ModCodeLoader* getCodeLoader(std::string name);

    Mod* findMod(std::string id, const ModDependencyVersionList& versions) const;

    void addMod(std::unique_ptr<ModResources> resources);
    void addModFromDirectory(std::string path);
    void addModFromZip(std::string path);
    void addModFromAssets(std::string path);
    void addAllModsFromDirectory(std::string path);

    void setAndroidAssetManager(AAssetManager* manager, long long lastModifyTime);

    std::vector<Mod*> getMods() const;

    void resolveDependenciesAndLoad();
    void updateHookManagerLoadedLibs();

    std::string const& getModDataStoragePath() { return modDataStoragePath; }

};

}
