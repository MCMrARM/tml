#pragma once

#include <string>
#include <vector>
#include <istream>

namespace tml {

class FileUtil {

public:
    struct DirectoryFile {
        std::string name;
        bool isDirectory;
    };

    /**
     * Returns the parent directory of the specified file. It doesn't do any IO operations - instead it removes
     * everything after the last slash (unless the slash is at the end of the file - meaning it's a directory - then the
     * slash before the last one will be used)
     */
    static std::string getParent(std::string path);

    /**
     * Check if the specified file exists (using the stat() function)
     */
    static bool fileExists(std::string path);

    /**
     * Returns when the file was last modified, or 0 on failure.
     */
    static unsigned long getTimestamp(std::string path);

    /**
     * Create the required directory, recursive.
     */
    static bool createDirs(std::string path, unsigned short perm = 0700);

    /**
     * Returns all files in the specified directory
     */
    static std::vector<DirectoryFile> getFilesIn(std::string path, bool includeHiddenFiles = false);

    /**
     * Calculates the SHA3-512 checksum for the specified file path. If it fails (for example because the file doesn't
     * exists), it'll return false.
     */
    static bool calculateSHA512(std::string path, char* out);

    /**
     * Calculates the SHA3-512 checksum for the specified stream. If it fails, it'll return false.
     */
    static bool calculateSHA512(std::istream& stream, char* out);

};

}
