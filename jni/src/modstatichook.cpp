#include <tml/modstatichook.h>

#include <tml/mod.h>
#include <cstring>

using namespace tml;

Mod* StaticHookManager::currentMod = nullptr;

void StaticHookManager::registerHook(const char* sym, void* hook, void** org) {
    const char* ls = strchr(sym, ':');
    if (ls != nullptr) {
        std::string lib(sym, ls - sym);
        currentMod->queueHook(lib, std::string(ls + 1), hook, org);
    } else {
        currentMod->queueHook(std::string(), std::string(sym), hook, org);
    }
}