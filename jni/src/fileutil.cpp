#include "fileutil.h"

#include <sys/stat.h>
#include <dirent.h>

using namespace tml;

std::string FileUtil::getParent(std::string path) {
    if (path[path.length() - 1] == '/')
        path = path.substr(0, path.length() - 1);
    auto li = path.find_last_of('/');
    if (li != std::string::npos)
        return path.substr(li + 1);
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

std::vector<FileUtil::DirectoryFile> FileUtil::getFilesIn(std::string path) {
    std::vector<DirectoryFile> ret;
    DIR* dir = opendir(path.c_str());
    if (dir == nullptr)
        return ret;
    dirent* de;
    while ((de = readdir(dir)) != nullptr) {
        ret.push_back({std::string(de->d_name), (de->d_type & DT_DIR) != 0});
    }
    closedir(dir);
    return ret;
}

bool FileUtil::calculateSHA512(std::istream& stream, char* out) {
    return false;
}

bool FileUtil::calculateSHA512(std::string path, char* out) {
    return false;
}