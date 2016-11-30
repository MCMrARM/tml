#pragma once

#include <memory>
#include "modmeta.h"
#include "modresources.h"
#include "modcodeloader.h"
#include "log.h"

namespace tml {

class ModLoader;
class ModHook;

class Mod {

private:
    friend class ModLoader;

    ModLoader* loader;
    std::unique_ptr<ModResources> resources;
    ModMeta meta;
    Log log;
    std::vector<std::unique_ptr<ModLoadedCode>> loadedCode;

    void load();
    void init();

public:
    Mod(ModLoader* loader, std::unique_ptr<ModResources> resources);

    /**
     * Returns the mod's associated resources object, using which you can access your packaged files.
     */
    ModResources& getResources() { return *resources; }

    /**
     * Returns the mod's metadata (the stuff definied in package.yaml).
     */
    ModMeta const& getMeta() const { return meta; }

    /**
     * Returns your mod's logger.
     */
    Log& getLog() { return log; }

    /**
     * Hooks a function and returns a pointer using which you can remove the hook.
     */
    ModHook* hook(void* sym, void* func, void** orig);

    /**
     * Removes a hook.
     */
    void removeHook(ModHook* h);

    /**
     * Registers a log printer (useful if you want to for example upload the logs to computer for debugging).
     * All TML and mod log messages will be sent to the specified log printer.
     */
    void registerLogPrinter(std::unique_ptr<LogPrinter> printer);

};

}
