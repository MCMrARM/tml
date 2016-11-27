#pragma once

#include <string>
#include <vector>
#include <iostream>

namespace tml {

class Mod;
class ModResources;
class ModCodeLoader;
class ModDependency;

struct ModVersion {
    int major = 0, minor = 0, patch = 0;
    ModVersion() { }
    ModVersion(const char* str);
    ModVersion(int major, int minor, int patch) {
        this->major = major;
        this->minor = minor;
        this->patch = patch;
    }
    bool operator==(const ModVersion& v) const { return (v.major == major && v.minor == minor && v.patch == patch); }
    bool operator!=(const ModVersion& v) const { return !(v == *this); }

    bool operator>(const ModVersion& v) const;
    bool operator>=(const ModVersion&) const;
    bool operator<(const ModVersion&) const;
    bool operator<=(const ModVersion&) const;
};

struct ModDependencyVersion {
    ModVersion from, to;

    ModDependencyVersion(ModVersion v) : from(v), to(v) { }
    ModDependencyVersion(ModVersion from, ModVersion to) : from(from), to(to) { }
    ModDependencyVersion(const char* str);
};

struct ModDependency {
    std::string id;
    std::vector<ModDependencyVersion> version;

    /**
     * Checks if the specified dependency version is supported by this mod.
     */
    bool isVersionSupported(const ModVersion& version) const;

    /**
     * This property will be assigned automatically by the mod manager after the dependency is resolved (it will never be null when loading your mod).
     */
    Mod* mod = nullptr;
};

struct ModCode {
    std::string loaderName;
    std::string codePath;
};

class ModMeta {

private:
    std::string name;
    std::string desc;
    std::string author;
    std::string id;
    ModVersion version;
    std::vector<ModCode> code;
    std::vector<ModDependency> dependencies;

    friend class ModLoader;

public:
    ModMeta(ModResources& resources);
    ModMeta(std::istream& ins);

    /**
     * Returns the mod's name (definied in the package.yaml file)
     */
    std::string const& getName() const { return name; }

    /**
     * Returns the mod's description (definied in the package.yaml file)
     */
    std::string const& getDescription() const { return desc; }

    /**
     * Returns the mod's author (definied in the package.yaml file)
     */
    std::string const& getAuthor() const { return author; }

    /**
     * Returns the mod's internal id (definied in the package.yaml file)
     */
    std::string const& getId() const { return id; }

    /**
     * Returns the mod's version (definied in the package.yaml file)
     */
    ModVersion const& getVersion() const { return version; }

    /**
     * Returns the mod's code files (definied in the package.yaml file)
     */
    std::vector<ModCode> const& getCode() const { return code; }

    /**
     * Returns the mod's dependencies (definied in the package.yaml file)
     */
    std::vector<ModDependency> const& getDependencies() const { return dependencies; }

    /**
     * Returns if all of the mod's dependencies were resolved.
     */
    bool areAllDependenciesResolved() const;

};

}
