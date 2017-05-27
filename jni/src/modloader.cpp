#include <tml/modloader.h>

#include <android/log.h>
#include <dlfcn.h>
#include <tml/mod.h>
#include <sys/stat.h>
#include <iterator>
#include "fileutil.h"
#include "nativemodcodeloader.h"
#include "hookmanager.h"

using namespace tml;

const char* ModLoader::MODLOADER_PKGID = "io.mrarm:tml";

ModLoader::ModLoader(std::string internalDir) : internalDir(internalDir), loaderLog(this, "TML") {
    if (internalDir[internalDir.length() - 1] != '/')
        internalDir += "/";
    mkdir(internalDir.c_str(), 0700);
    mkdir((internalDir + "mods/").c_str(), 0700);
    modDataStoragePath = internalDir + "mod_data/";
    mkdir(modDataStoragePath.c_str(), 0700);
    loaders["native"] = {nullptr,
                         std::unique_ptr<ModCodeLoader>(new NativeModCodeLoader(*this, internalDir + "cache/native"))};
    hookManager = new HookManager(this);
}

ModLoader::~ModLoader() {
    delete hookManager;
}

Mod* ModLoader::findMod(std::string id, const ModDependencyVersionList& versions) const {
    if (mods.count(id) <= 0)
        return nullptr;
    Mod* newestMod = nullptr;
    ModVersion newestModVersion;
    for (const auto& mod : mods.at(id)) {
        if (versions.contains(mod.first)) {
            if (newestMod == nullptr || mod.first > newestModVersion) {
                newestMod = mod.second.get();
                newestModVersion = mod.first;
            }
        }
    }
    return newestMod;
}

void ModLoader::addMod(std::unique_ptr<ModResources> resources) {
    std::unique_ptr<Mod> mod(new Mod(this, std::move(resources)));
    if (mods.count(mod->getMeta().getId()) > 0) {
        Mod* secondMod = mods.at(mod->getMeta().getId()).begin()->second.get();
        if (secondMod->getMeta().getVersion() == mod->getMeta().getVersion())
            throw std::runtime_error("Trying to load a mod with an already used name (" + mod->getMeta().getId() +
                                     ") and version");
        if (!mods.at(mod->getMeta().getId()).begin()->second->getMeta().hasDeclaredMultiversionSupport() ||
            !mod->getMeta().hasDeclaredMultiversionSupport())
            throw std::runtime_error("Trying to register another version of " + mod->getMeta().getId() +
                                     " without declaring multiversion support");
    }
    mods[mod->getMeta().getId()][mod->getMeta().getVersion()] = std::move(mod);
}

void ModLoader::addModFromDirectory(std::string path) {
    loaderLog.info("Loading mod from directory: %s", path.c_str());
    std::unique_ptr<ModResources> res(new DirectoryModResources(path));
    addMod(std::move(res));
}

void ModLoader::addModFromZip(std::string path) {
    loaderLog.info("Loading mod from zip: %s", path.c_str());
    std::unique_ptr<ModResources> res(new ZipModResources(path));
    addMod(std::move(res));
}

void ModLoader::addAllModsFromDirectory(std::string path) {
    loaderLog.info("Loading all mod from directory: %s", path.c_str());
    for (auto& f : FileUtil::getFilesIn(path)) {
        if (f.isDirectory) {
            addModFromDirectory(path + "/" + f.name);
        } else if (f.name.length() >= 4 && memcmp(&f.name[f.name.length() - 4], ".tbp", 4) == 0) {
            addModFromZip(path + "/" + f.name);
        }
    }
}

std::vector<Mod*> ModLoader::getMods() const {
    std::vector<Mod*> ret;
    for (const auto& modVersion : mods) {
        for (const auto& mod : modVersion.second) {
            ret.push_back(mod.second.get());
        }
    }
    return std::move(ret);
}

bool ModLoader::loadMod(Mod& mod) {
    if (!mod.getMeta().areAllDependenciesResolved()) {
        loaderLog.error("Not loading mod %s - failed to resolve some dependencies", mod.getMeta().getId().c_str());
        return false;
    }
    for (const auto& dep : mod.getMeta().getDependencies())
        if (!dep.mod->isLoaded())
            loadMod(*dep.mod);
    mod.load();
    return true;
}

void ModLoader::initMod(Mod &mod) {
    for (const auto& dep : mod.getMeta().getDependencies())
        if (!dep.mod->initialized)
            initMod(*dep.mod);
    try {
        mod.init();
    } catch (std::exception e) {
        loaderLog.error("Failed to init mod %s: %s", mod.getMeta().getId().c_str(), e.what());
    }
}

void ModLoader::resolveDependenciesAndLoad() {
    for (const auto& modVersions : mods) {
        for (const auto& mod : modVersions.second) {
            for (auto& dep : mod.second->meta.dependencies) {
                if (mods.count(dep.id) > 0) {
                    Mod* depMod = findMod(dep.id, dep.version);
                    if (depMod != nullptr) {
                        dep.mod = depMod;
                    } else {
                        loaderLog.error("Mod %s depends on a unsupported version of %s", modVersions.first.c_str(),
                                        dep.id.c_str());
                    }
                } else {
                    loaderLog.error("Mod %s depends on %s but it is not installed", modVersions.first.c_str(),
                                    dep.id.c_str());
                }
            }
        }
    }

    loaderLog.trace("Initializing hook system...");
    mcpeLib = dlopen("libminecraftpe.so", RTLD_LAZY);
    if (mcpeLib == nullptr)
        throw std::runtime_error("Failed to dlopen libminecraftpe.so");
    hookManager->updateLoadedLibs();

    loaderLog.trace("Loading mod code...");
    for (auto& modVersions : mods) {
        for (auto it = modVersions.second.begin(); it != modVersions.second.end();) {
            if (!it->second->isLoaded() && !loadMod(*it->second)) {
                // remove it from the list
                it = modVersions.second.erase(it);
                continue;
            }
            it++;
        }
    }

    loaderLog.trace("Updating hook system with the mod libraries...");
    hookManager->updateLoadedLibs();

    loaderLog.trace("Initializing mods...");
    for (const auto& modVersions : mods) {
        for (const auto& mod : modVersions.second) {
            initMod(*mod.second);
        }
    }
}

void ModLoader::updateHookManagerLoadedLibs() {
    hookManager->updateLoadedLibs();
}

ModCodeLoader* ModLoader::getCodeLoader(std::string name) {
    if (loaders.count(name) > 0)
        return loaders.at(name).second.get();
    return nullptr;
}

void ModLoader::registerLogPrinter(Mod& ownerMod, std::unique_ptr<LogPrinter> printer) {
    logPrinters.push_back({&ownerMod, std::move(printer)});
}

void ModLoader::printLogMessage(LogLevel level, const std::string& tag, const char* msg, va_list va) {
    int androidLogLevel;
    switch (level) {
        case LogLevel::TRACE:
            androidLogLevel = ANDROID_LOG_VERBOSE;
            break;
        case LogLevel::DEBUG:
            androidLogLevel = ANDROID_LOG_DEBUG;
            break;
        case LogLevel::INFO:
            androidLogLevel = ANDROID_LOG_INFO;
            break;
        case LogLevel::WARN:
            androidLogLevel = ANDROID_LOG_WARN;
            break;
        case LogLevel::ERROR:
            androidLogLevel = ANDROID_LOG_ERROR;
            break;
        case LogLevel::FATAL:
            androidLogLevel = ANDROID_LOG_FATAL;
            break;
    }
    __android_log_vprint(androidLogLevel, tag.c_str(), msg, va);

    for (auto& printer : logPrinters) {
        printer.second->printLogMessage(level, tag, msg, va);
    }
}
