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
    std::string toString() const;
    bool operator==(const ModVersion& v) const { return (v.major == major && v.minor == minor && v.patch == patch); }
    bool operator!=(const ModVersion& v) const { return !(v == *this); }

    bool operator>(const ModVersion& v) const {
        return (major > v.major || (major == v.major && (minor > v.minor || (minor == v.minor && patch > v.patch))));
    }
    bool operator>=(const ModVersion& v) const { return (*this > v || *this == v); }
    bool operator<(const ModVersion& v) const { return v > *this; }
    bool operator<=(const ModVersion& v) const { return (*this < v || *this == v); }
};

struct ModDependencyVersion {
    ModVersion from, to;

    ModDependencyVersion(ModVersion v) : from(v), to(v) { }
    ModDependencyVersion(ModVersion from, ModVersion to) : from(from), to(to) { }
    ModDependencyVersion(const char* str);
};

struct ModDependencyVersionList {
    std::vector<ModDependencyVersion> list;

    /**
     * Checks if the specified version is a part of this dependency list
     */
    bool contains(const ModVersion& version) const;
};

struct ModDependency {
    std::string id;
    ModDependencyVersionList version;

    /**
     * This property will be assigned automatically by the mod manager after the dependency is resolved (it will never
     * be null when loading your mod).
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
    bool supportsMultiversion = false;

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
     * Returns if the mod has declared multiversion support (if so, we assume that multiple versions of the mod can
     * be loaded at the same time, and that the presence of multiple versions of the mod won't break anything).
     * If your mod/library only adds ModAPI wrappers, you should declare multiversion support. If your mod adds
     * GUI/items/blocks/etc. to the game then you should NOT declare multiversion support.
     */
    bool hasDeclaredMultiversionSupport() const { return supportsMultiversion; }

    /**
     * Returns if all of the mod's dependencies were resolved.
     */
    bool areAllDependenciesResolved() const;

};

}
