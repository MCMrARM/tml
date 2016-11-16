#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include "log.h"

namespace tml {

class Mod;
class ModResources;
class ModCodeLoader;
class NativeModCodeLoader;

class ModLoader {

private:
    std::map<std::string, std::unique_ptr<ModCodeLoader>> loaders;
    std::map<std::string, std::unique_ptr<Mod>> mods;

protected:
    Log loaderLog;

    void registerCodeLoader(Mod& ownerMod, std::string name, std::unique_ptr<ModCodeLoader> loader);

public:
    static const char* MODLOADER_PKGID;

    ModLoader(std::string internalDir);

    inline Log& getLog() { return loaderLog; }

    ModCodeLoader* getCodeLoader(std::string name);

    void addMod(std::unique_ptr<ModResources> resources);
    void addModFromDirectory(std::string path);
    void addModFromZip(std::string path);
    void addAllModsFromDirectory(std::string path);

    void resolveDependenciesAndLoad();

};

}
