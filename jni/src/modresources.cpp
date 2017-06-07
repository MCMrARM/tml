#include <tml/modresources.h>

#include <fstream>
#include <zip.h>

#include "fileutil.h"
#include "modresources_private.h"

using namespace tml;

std::unique_ptr<std::istream> DirectoryModResources::open(const std::string& path) {
    return std::unique_ptr<std::istream>(new std::ifstream(basePath + "/" + path, std::ifstream::binary));
}

bool DirectoryModResources::contains(const std::string& path) {
    return FileUtil::fileExists(basePath + "/" + path);
}

std::vector<ModResources::DirectoryFile> DirectoryModResources::list(const std::string& path) {
    std::vector<DirectoryFile> ret;
    for (const auto& e : FileUtil::getFilesIn(basePath + "/" + path)) {
        ret.push_back({e.name, e.isDirectory});
    }
    return std::move(ret);
}

long long DirectoryModResources::getSize(const std::string& path) {
    return FileUtil::getSize(basePath + "/" + path);
}

long long DirectoryModResources::getLastModifyTime(const std::string& path) {
    return (long long) FileUtil::getTimestamp(basePath + "/" + path);
}

ZipModResources::ZipModResources(const std::string& path) {
    int err;
    file = zip_open(path.c_str(), 0, &err);
    if (!file)
        throw std::runtime_error("Failed to open zip file: " + path);
    fileLastModify = (long long) FileUtil::getTimestamp(path);
    zip_uint64_t entryCount = (zip_uint64_t) zip_get_num_entries(file, 0);
    for (zip_uint64_t i = 0; i < entryCount; i++) {
        fileMap[zip_get_name(file, i, 0)] = i;
    }
}

ZipModResources::~ZipModResources() {
    if (file != nullptr)
        zip_close(file);
}

std::unique_ptr<std::istream> ZipModResources::open(const std::string& path) {
    zip_file* zf = nullptr;
    if (fileMap.count(path) > 0)
        zf = zip_fopen_index(file, fileMap.at(path), 0);
    return std::unique_ptr<std::istream>(new ZipInputStream(zf));
}

bool ZipModResources::contains(const std::string& path) {
    return fileMap.count(path) > 0;
}

std::vector<ModResources::DirectoryFile> ZipModResources::list(const std::string& path) {
    std::string prefix = path;
    if (prefix[prefix.length() - 1] != '/')
        prefix = prefix + "/";
    std::map<std::string, bool> m; // { name => isDir }
    for (const auto& p : fileMap) {
        if (p.first.length() > prefix.length() && memcmp(p.first.data(), prefix.data(), prefix.length()) == 0) {
            std::string name = p.first.substr(prefix.length());
            auto iof = name.find('/');
            if (iof != std::string::npos) {
                m[name.substr(0, iof)] = true;
            } else {
                m[name] = false;
            }
        }
    }
    std::vector<DirectoryFile> ret;
    for (const auto& p : m) {
        ret.push_back({p.first, p.second});
    }
    return std::move(ret);
}

long long ZipModResources::getSize(const std::string& path) {
    if (fileMap.count(path) <= 0)
        return -1;
    zip_stat_t st;
    if (zip_stat_index(file, fileMap.at(path), 0, &st))
        return -1;
    return st.size;
}

long long ZipModResources::getLastModifyTime(const std::string& path) {
    return fileLastModify;
}

ZipStreamBuffer::~ZipStreamBuffer() {
    if (ownsFile && file != nullptr)
        zip_fclose(file);
}

ZipStreamBuffer::int_type ZipStreamBuffer::underflow() {
    if (file == nullptr)
        return std::char_traits<char>::eof();
    if (this->gptr() == this->egptr()) {
        size_t size = (size_t) zip_fread(file, buffer.data(), buffer.size());
        this->setg(buffer.data(), buffer.data(), buffer.data() + size);
    }
    return this->gptr() == this->egptr()
           ? std::char_traits<char>::eof()
           : std::char_traits<char>::to_int_type(*this->gptr());

}

AndroidAssetsModResources::AndroidAssetsModResources(AAssetManager* manager, const std::string& basePath,
                                                     long long lastModifyTime) : manager(manager), basePath(basePath),
                                                                                 lastModifyTime(lastModifyTime) {
    auto directoriesFile = open("directories.txt");
    if (directoriesFile && *directoriesFile) {
        std::string line;
        while (std::getline(*directoriesFile, line)) {
            if (line.length() > 0) {
                directories.insert(line);
            }
        }
    }
}

std::unique_ptr<std::istream> AndroidAssetsModResources::open(const std::string& path) {
    AAsset* asset = AAssetManager_open(manager, (basePath + "/" + path).c_str(), AASSET_MODE_BUFFER);
    return std::unique_ptr<std::istream>(new AAssetInputStream(asset));
}

bool AndroidAssetsModResources::contains(const std::string& path) {
    AAsset* asset = AAssetManager_open(manager, (basePath + "/" + path).c_str(), AASSET_MODE_UNKNOWN);
    if (asset != nullptr) {
        AAsset_close(asset);
        return true;
    }
    return false;
}

std::vector<ModResources::DirectoryFile> AndroidAssetsModResources::list(const std::string& path) {
    std::string sPath = path;
    if (sPath.length() > 0 && sPath[sPath.length() - 1] == '/')
        sPath = sPath.substr(0, sPath.size() - 1);
    AAssetDir* dir = AAssetManager_openDir(manager, (basePath + "/" + sPath).c_str());
    if (dir == nullptr)
        return std::vector<DirectoryFile>();
    std::vector<DirectoryFile> ret;
    const char* nextFilename;
    while ((nextFilename = AAssetDir_getNextFileName(dir)) != nullptr) {
        ret.push_back({nextFilename, false});
    }
    for (const auto& p : directories) {
        if (p.length() > sPath.length() + 1 && memcmp(p.data(), sPath.data(), sPath.length()) == 0 &&
            p[sPath.length()] == '/') {
            std::string name = p.substr(sPath.length() + 1);
            if (name.find('/') == std::string::npos) {
                ret.push_back({name, true});
            }
        }
    }
    return ret;
}

long long AndroidAssetsModResources::getSize(const std::string& path) {
    AAsset* asset = AAssetManager_open(manager, (basePath + "/" + path).c_str(), AASSET_MODE_BUFFER);
    long long ret = -1;
    if (asset != nullptr) {
        ret = AAsset_getLength64(asset);
        AAsset_close(asset);
    }
    return ret;
}

AAssetStreamBuffer::~AAssetStreamBuffer() {
    if (ownsAsset && asset != nullptr)
        AAsset_close(asset);
}

AAssetStreamBuffer::int_type AAssetStreamBuffer::underflow() {
    if (asset == nullptr)
        return std::char_traits<char>::eof();
    if (this->gptr() == this->egptr()) {
        size_t size = (size_t) AAsset_read(asset, buffer.data(), buffer.size());
        this->setg(buffer.data(), buffer.data(), buffer.data() + size);
    }
    return this->gptr() == this->egptr()
           ? std::char_traits<char>::eof()
           : std::char_traits<char>::to_int_type(*this->gptr());

}