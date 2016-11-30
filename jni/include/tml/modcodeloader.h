#pragma once

#include <string>
#include <vector>
#include <memory>

class MinecraftClient;

namespace tml {

class Mod;

class ModLoadedCode {

protected:
    Mod& mod;

public:
    ModLoadedCode(Mod& mod) : mod(mod) { }

    virtual ~ModLoadedCode() { }
    virtual void init() = 0;
    virtual void onMinecraftInitialized(MinecraftClient* minecraft) = 0;
    // virtual void callCallback(std::string name, scripting::UTypeList args);

};

class ModCodeLoader {

public:
    virtual ~ModCodeLoader() { }
    virtual std::unique_ptr<ModLoadedCode> loadCode(Mod& mod, std::string path) = 0;

};

}
