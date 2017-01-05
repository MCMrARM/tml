#include <tml/modresources.h>

#include <fstream>
#include "fileutil.h"

using namespace tml;

std::unique_ptr<std::istream> DirectoryModResources::open(std::string path) {
    return std::unique_ptr<std::istream>(new std::ifstream(basePath + "/" + path, std::ifstream::binary));
}

bool DirectoryModResources::contains(std::string path) {
    return FileUtil::fileExists(basePath + "/" + path);
}

long long DirectoryModResources::getLastModifyTime(std::string path) {
    return FileUtil::getTimestamp(basePath + "/" + path);
}