#pragma once

#include <string>
#include <vector>

namespace tml {

/**
 * The class resposible for fetching the mod's files. You generally will not need to subclass it, unless you want
 * to make a custom mod loader.
 */
class ModResources {

public:
    virtual ~ModResources() { }

    /**
     * Opens a stream with the specific file.
     */
    virtual std::unique_ptr<std::istream> open(std::string path) = 0;

    /**
     * Return the last modification time. If you aren't able to determine this, return 0.
     * This is only important when getting ready for production. In testing it should be fast enough to return zero.
     */
    virtual long long getLastModifyTime(std::string path) { return 0; }

};

class DirectoryModResources : public ModResources {

protected:
    std::string basePath;

public:
    DirectoryModResources(std::string basePath) : basePath(basePath) { }

    virtual std::unique_ptr<std::istream> open(std::string path);

    virtual long long getLastModifyTime(std::string path);

};

}
