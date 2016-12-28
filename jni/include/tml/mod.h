#pragma once

#include <memory>
#include "modmeta.h"
#include "modresources.h"
#include "modcodeloader.h"
#include "modstatichook.h"
#include "log.h"

namespace tml {

class ModLoader;
class ModHook;

class Mod {

private:
    friend class ModLoader;

    friend class StaticHookManager;

    struct QueuedHook {
        std::string lib, sym;
        void* func;
        void** org;
    };

    ModLoader* loader;
    std::unique_ptr<ModResources> resources;
    ModMeta meta;
    Log log;
    std::vector<std::unique_ptr<ModLoadedCode>> loadedCode;
    std::vector<QueuedHook> queuedHooks;

    void load();
    void init();

    void queueHook(const std::string& lib, const std::string& sym, void* func, void** orig);

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
     * Returns a pointer to MCPE's library.
     */
    void* getMCPELibrary() const;

    /**
     * Hooks a function and returns a pointer using which you can remove the hook.
     */
    ModHook* hook(void* lib, const char* str, void* func, void** orig);

    /**
     * Hooks a MCPE function and returns a pointer using which you can remove the hook.
     */
    ModHook* hook(const char* str, void* func, void** orig);

    /**
     * Removes a hook.
     */
    void removeHook(ModHook* h);

    /**
     * Resolves a symbol from the specified library and stores it in the specified pointer.
     */
    void resolveSymbol(void* lib, const char* str, void** ptr);

    /**
     * Resolves a MCPE symbol and stores it in the specified pointer.
     */
    void resolveSymbol(const char* str, void** ptr);

    /**
     * Removes a symbol reference.
     */
    void releaseSymbolRef(void** ptr);

    /**
     * Registers a log printer (useful if you want to for example upload the logs to computer for debugging).
     * All TML and mod log messages will be sent to the specified log printer.
     */
    void registerLogPrinter(std::unique_ptr<LogPrinter> printer);

};

}
