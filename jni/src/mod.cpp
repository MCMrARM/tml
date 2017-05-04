#include <tml/mod.h>

#include <dlfcn.h>
#include <tml/modloader.h>
#include "hookmanager.h"
#include "fileutil.h"

using namespace tml;

Mod::Mod(ModLoader* loader, std::unique_ptr<ModResources> resources) : loader(loader), resources(std::move(resources)),
                                                                       meta(*this->resources),
                                                                       log(loader, meta.getName()) {
    dataPath = loader->getModDataStoragePath() + meta.getId() + "/";
    FileUtil::createDirs(dataPath);
}

void Mod::load() {
    if (loaded)
        return;
    StaticHookManager::currentMod = this;
    for (const ModCode& code : meta.getCode()) {
        ModCodeLoader* codeLoader = loader->getCodeLoader(code.loaderName);
        if (codeLoader == nullptr) {
            loader->getLog().error("Cannot load mod code '%s' from the mod %s - loader %s doesn't exists!",
                                   code.codePath.c_str(), meta.getId().c_str(), code.loaderName.c_str());
            continue;
        }
        loader->getLog().trace("Loading mod code '%s' from the mod %s", code.codePath.c_str(), code.loaderName.c_str());
        auto c = codeLoader->loadCode(*this, code.codePath);
        if (c)
            loadedCode.push_back(std::move(c));
        else
            loader->getLog().error("Failed to load mod code '%s' from the mod %s", code.codePath.c_str(),
                                   code.loaderName.c_str());
    }
    StaticHookManager::currentMod = nullptr;
    loaded = true;
}

void Mod::init() {
    if (initialized)
        return;
    for (auto& hk : queuedHooks) {
        void* lib = getMCPELibrary();
        if (hk.lib.length() > 0)
            lib = dlopen(hk.lib.c_str(), RTLD_LAZY);
        hook(lib, hk.sym.c_str(), hk.func, hk.org);
    }
    queuedHooks.clear();
    for (auto& code : loadedCode) {
        code->init();
    }
    initialized = true;
}

void* Mod::getMCPELibrary() const {
    return loader->mcpeLib;
}

void Mod::queueHook(const std::string& lib, const std::string& sym, void* func, void** orig) {
    queuedHooks.push_back({lib, sym, func, orig});
}

ModHook* Mod::hook(void* lib, const char* str, void* func, void** orig) {
    return (ModHook*) (void*) loader->hookManager->hook(lib, str, func, orig);
}

ModHook* Mod::hook(const char* str, void* func, void** orig) {
    return hook(getMCPELibrary(), str, func, orig);
}

void Mod::removeHook(ModHook* h) {
    loader->hookManager->unhook((HookManager::HookInfo*) (void*) h);
}

void Mod::resolveSymbol(void* lib, const char* str, void** ptr) {
    loader->hookManager->addCustomRef(ptr, lib, str);
}

void Mod::resolveSymbol(const char* str, void** ptr) {
    resolveSymbol(getMCPELibrary(), str, ptr);
}

void Mod::releaseSymbolRef(void** ptr) {
    loader->hookManager->removeCustomRef(ptr);
}

void Mod::registerLogPrinter(std::unique_ptr<LogPrinter> printer) {
    loader->registerLogPrinter(*this, std::move(printer));
}