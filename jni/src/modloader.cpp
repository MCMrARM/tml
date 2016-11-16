#include <tml/modloader.h>

#include <tml/mod.h>

using namespace tml;

const char* ModLoader::MODLOADER_PKGID = "io.mrarm:tml";

void ModLoader::addMod(std::unique_ptr<ModResources> resources) {
    std::unique_ptr<Mod> mod (new Mod(this, std::move(resources)));
    if (mods.count(mod->getMeta().getId()) > 0)
        throw std::runtime_error("The mod is trying to use a identifier already used by anther mod");
    mods[mod->getMeta().getId()] = std::move(mod);
}

void ModLoader::resolveDependenciesAndLoad() {
    for (const auto& mod : mods) {
        for (auto& dep : mod.second->meta.dependencies) {
            if (mods.count(dep.id) > 0) {
                Mod& depMod = *mods.at(dep.id);
                if (depMod.getMeta().getVersion() >= dep.version) {
                    dep.mod = &depMod;
                } else {
                    loaderLog.error("Mod %s depends on a newer version of %s", mod.first.c_str(), dep.id.c_str());
                }
            } else {
                loaderLog.error("Mod %s depends on %s but it is not installed", mod.first.c_str(), dep.id.c_str());
            }
        }
    }
    for (const auto& mod : mods) {
        if (!mod.second->getMeta().areAllDependenciesResolved()) {
            loaderLog.error("Not loading mod %s - failed to resolve some dependencies", mod.first.c_str());
            continue;
        }
        mod.second->load();
    }
}