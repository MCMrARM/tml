#include <tml/mod.h>

#include <tml/modloader.h>

using namespace tml;

void Mod::load() {
    for (const ModCode& code : meta.getCode()) {
        ModCodeLoader* codeLoader = loader->getCodeLoader(code.loaderName);
        if (codeLoader == nullptr) {
            loader->getLog().error("Cannot load mod code %s from the mod %s - loader %s doesn't exists!",
                                   code.codePath.c_str(), meta.getId().c_str(), code.loaderName.c_str());
            continue;
        }
        loader->getLog().trace("Loading mod code %s from the mod %s", code.codePath.c_str(), code.loaderName.c_str());
        auto c = codeLoader->loadCode(*this, code.codePath);
        if (c)
            loadedCode.push_back(std::move(c));
        else
            loader->getLog().error("Failed to load mod code %s from the mod %s", code.codePath.c_str(),
                                   code.loaderName.c_str());
    }
}