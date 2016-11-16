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
    int major, minor, build;
    ModVersion(std::string str);
    ModVersion(int major, int minor, int build) : major(major), minor(minor), build(build) { }
    bool operator>(const ModVersion&) const;
    bool operator>=(const ModVersion&) const;
    bool operator<(const ModVersion&) const;
    bool operator<=(const ModVersion&) const;
};

struct ModDependency {
    std::string id;
    ModVersion version;

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
    ModMeta(std::istream ins);

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
