#pragma once

#include <streambuf>

namespace tml {

class ZipStreamBuffer : public std::streambuf {

private:

    zip_file* file;
    bool ownsFile;
    std::vector<char> buffer;

    virtual int_type underflow();

public:
    ZipStreamBuffer(zip_file* file, bool ownsFile = true, size_t buffer_size = 16 * 1024) : file(file),
                                                                                            ownsFile(ownsFile) {
        buffer.resize(buffer_size);
    }

    ~ZipStreamBuffer();

};

class ZipInputStream : public std::istream {

private:
    ZipStreamBuffer buf;

public:
    ZipInputStream(zip_file* file, bool ownsFile = true) : buf(file, ownsFile), std::istream(&buf) {
    }

};

}