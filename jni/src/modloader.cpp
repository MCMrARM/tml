#include <tml/modloader.h>

#include <android/log.h>
#include <tml/mod.h>

using namespace tml;

const char* ModLoader::MODLOADER_PKGID = "io.mrarm:tml";

Mod* ModLoader::findMod(std::string id, const ModDependencyVersionList& versions) {
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
    for (const auto& modVersions : mods) {
        for (const auto& mod : modVersions.second) {
            if (!mod.second->getMeta().areAllDependenciesResolved()) {
                loaderLog.error("Not loading mod %s - failed to resolve some dependencies", modVersions.first.c_str());
                continue;
            }
            mod.second->load();
        }
    }
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