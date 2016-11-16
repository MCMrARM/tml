#pragma once

#include <memory>
#include "modmeta.h"
#include "modresources.h"
#include "modcodeloader.h"

namespace tml {

class ModLoader;
class ModHook;

class Mod {

private:
    friend class ModLoader;

    ModLoader* loader;
    std::unique_ptr<ModResources> resources;
    ModMeta meta;
    std::vector<std::unique_ptr<ModLoadedCode>> loadedCode;

    void load();

public:
    Mod(ModLoader* loader, std::unique_ptr<ModResources> resources) : resources(std::move(resources)), meta(*resources) { }

    /**
     * Returns the mod's associated resources object, using which you can access your packaged files.
     */
    ModResources& getResources() { return *resources; }

    /**
     * Returns the mod's metadata (the stuff definied in package.yaml).
     */
    ModMeta const& getMeta() const { return meta; }

    /**
     * Hooks a function and returns a pointer using which you can remove the hook.
     */
    ModHook* hook(void* sym, void* func, void** orig);

    /**
     * Removes a hook.
     */
    void removeHook(ModHook* h);

};

}
