#include <tml/modloader.h>

#include <android/log.h>
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