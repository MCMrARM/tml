#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <android/asset_manager.h>

struct zip;

namespace tml {

/**
 * The class resposible for fetching the mod's files. You generally will not need to subclass it, unless you want
 * to make a custom mod loader.
 */
class ModResources {

public:
    struct DirectoryFile {
        std::string fileName;
        bool isDirectory;
    };

    virtual ~ModResources() { }

    /**
     * Opens a stream with the specific file.
     */
    virtual std::unique_ptr<std::istream> open(const std::string& path) = 0;

    /**
     * Checks if the specific file exists.
     */
    virtual bool contains(const std::string& path) = 0;

    /**
     * List the files in the specific directory (returns filenames, not full paths). If an implementation can't provide
     * this, an empty array will be returned (however it might cause stuff to break).
     */
    virtual std::vector<DirectoryFile> list(const std::string& path) { return std::vector<DirectoryFile>(); }

    /**
     * Get the specified file's size (in bytes). If the file doesn't exist on an error occurs, this function will
     * return -1.
     */
    virtual long long getSize(const std::string& path) = 0;

    /**
     * Return the last modification time. If you aren't able to determine this, return 0.
     * This is only important when getting ready for production. In testing it should be fast enough to return zero.
     */
    virtual long long getLastModifyTime(const std::string& path) { return 0; }

};

class DirectoryModResources : public ModResources {

protected:
    std::string basePath;

public:
    DirectoryModResources(const std::string& basePath) : basePath(basePath) { }

    virtual std::unique_ptr<std::istream> open(const std::string& path);

    virtual bool contains(const std::string& path);

    virtual std::vector<DirectoryFile> list(const std::string& path);

    virtual long long getSize(const std::string& path);

    virtual long long getLastModifyTime(const std::string& path);

};

class ZipModResources : public ModResources {

protected:
    zip* file = nullptr;
    long long fileLastModify;
    std::map<std::string, uint64_t> fileMap;

public:
    ZipModResources(const std::string& path);

    ~ZipModResources();

    virtual std::unique_ptr<std::istream> open(const std::string& path);

    virtual bool contains(const std::string& path);

    virtual std::vector<DirectoryFile> list(const std::string& path);

    virtual long long getSize(const std::string& path);

    /**
     * Always returns the modification time of the zip.
     */
    virtual long long getLastModifyTime(const std::string& path);

};

class AndroidAssetsModResources : public ModResources {

protected:
    AAssetManager* manager = nullptr;
    std::string basePath;
    long long lastModifyTime;
    std::set<std::string> directories;

public:
    AndroidAssetsModResources(AAssetManager* manager, const std::string& basePath, long long lastModifyTime);

    virtual std::unique_ptr<std::istream> open(const std::string& path);

    virtual bool contains(const std::string& path);

    virtual std::vector<DirectoryFile> list(const std::string& path);

    virtual long long getSize(const std::string& path);

    /**
     * Always returns the modification time of the zip.
     */
    virtual long long getLastModifyTime(const std::string& path) { return lastModifyTime; }

};

}
