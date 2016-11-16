#pragma once

#include <string>
#include <istream>

namespace tml {

class FileUtil {

public:
    static bool fileExists(std::string path);
    static long long getTimestamp(std::string path);
    static std::string getParent(std::string path);
    static bool calculateSHA512(std::string path, char* out);
    static bool calculateSHA512(std::istream& stream, char* out);

};

}
