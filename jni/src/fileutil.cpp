#include "fileutil.h"

#include <fstream>
#include <sys/stat.h>
#include <dirent.h>
extern "C" {
#include "KeccakSponge.h"
}
using namespace tml;

std::string FileUtil::getParent(std::string path) {
    if (path[path.length() - 1] == '/')
        path = path.substr(0, path.length() - 1);
    auto li = path.find_last_of('/');
    if (li != std::string::npos)
        return path.substr(0, li + 1);
    else
        return "";
}

bool FileUtil::fileExists(std::string path) {
    struct stat buf;
    return (stat(path.c_str(), &buf) == 0);
}

unsigned long FileUtil::getTimestamp(std::string path) {
    struct stat buf;
    if (stat(path.c_str(), &buf) != 0)
        return 0;
    return buf.st_mtime;
}

bool FileUtil::createDirs(std::string path, unsigned short perm) {
    if (path.length() == 0)
        return true;
    struct stat buf;
    if (stat(path.c_str(), &buf) == 0)
        return S_ISDIR(buf.st_mode);
    if (!FileUtil::createDirs(FileUtil::getParent(path)))
        return false;
    return mkdir(path.c_str(), (mode_t) perm) == 0;
}

std::vector<FileUtil::DirectoryFile> FileUtil::getFilesIn(std::string path, bool includeHiddenFiles) {
    std::vector<DirectoryFile> ret;
    DIR* dir = opendir(path.c_str());
    if (dir == nullptr)
        return ret;
    dirent* de;
    while ((de = readdir(dir)) != nullptr) {
        if (!includeHiddenFiles && de->d_name[0] == '.')
            continue;
        ret.push_back({std::string(de->d_name), (de->d_type & DT_DIR) != 0});
    }
    closedir(dir);
    return ret;
}

bool FileUtil::calculateSHA512(std::istream& stream, char* out) {
    KeccakWidth1600_SpongeInstance sponge;
    KeccakWidth1600_SpongeInitialize(&sponge, 576, 1024);
    char buffer[8 * 1024];
    while (stream.good()) {
        stream.read(buffer, sizeof(buffer));
        std::streamsize n = stream.gcount();
        if (n > 0)
            KeccakWidth1600_SpongeAbsorb(&sponge, (unsigned char*) buffer, (size_t) n);
    }
    KeccakWidth1600_SpongeSqueeze(&sponge, (unsigned char*) out, 512/8);
    return true;
}

bool FileUtil::calculateSHA512(std::string path, char* out) {
    std::ifstream fs (path, std::ifstream::binary);
    if (!fs)
        return false;
    return calculateSHA512(fs, out);
}