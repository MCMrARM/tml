#include "hookmanager.h"

#include <set>
#include <dlfcn.h>
#include <sys/mman.h>
#include <tml/modloader.h>
#include <linkerutils/linker.h>
#include <linkerutils/linkerutils.h>

using namespace tml;

HookManager::HookManager(ModLoader* loader) : log(loader, "HookManager") {
    //
}

int _getline(char** lineptr, size_t* n, FILE* stream) {
    if (*lineptr == nullptr || *n < 16) {
        *n = 64;
        *lineptr = (char*) (*lineptr == nullptr ? malloc(*n) : realloc(*lineptr, *n));
        if (*lineptr == nullptr)
            return -1;
    }
    int c, pos = 0;
    while ((c = getc(stream)) != EOF) {
        (*lineptr)[pos++] = (char) c;
        if (pos >= *n) {
            *n *= 2;
            *lineptr = (char*) realloc(*lineptr, *n);
            if (*lineptr == nullptr)
                return -1;
        }
        if (c == '\n')
            break;
    }
    (*lineptr)[pos] = 0;
    return pos;
}

void HookManager::updateLoadedLibs() {
    log.trace("Updating loaded libs...");
    // first find the libraries
    FILE* file = fopen("/proc/self/maps", "r");
    if (file == NULL)
        throw std::runtime_error("Failed to open /proc/self/maps");

    std::set<std::string> libsToFind;
    for (auto& e : librariesByPath) {
        libsToFind.insert(e.first);
    }
    char* _name = NULL;
    size_t len = 0;
    while (!feof(file)) {
        long unsigned int start, end;
        long long unsigned int file_offset;
        long long inode;
        char r, w, x, shared;
        int dev_major, dev_minor;
        if (fscanf(file, (sizeof(void*) == 8 ? "%16lx-%16lx %c%c%c%c %Lx %x:%x %Lu"
                                             : "%08lx-%08lx %c%c%c%c %Lx %x:%x %Lu"), &start, &end,
                   &r, &w, &x, &shared, &file_offset, &dev_major, &dev_minor, &inode) < 9)
            break;
        if (_getline(&_name, &len, file) <= 0)
            break;
        char* name;
        {
            size_t name_start;
            for (name_start = 0; name_start < len; name_start++) {
                if (_name[name_start] != ' ')
                    break;
            }
            name = &_name[name_start];
            ssize_t name_len;
            for (name_len = (ssize_t) strlen(name) - 1; name_len >= 0; name_len--) {
                if (name[name_len] != ' ' && name[name_len] != '\n') {
                    name[name_len + 1] = '\0';
                    break;
                }
            }
        }
        if (name[0] != '/')
            continue; // we're not interested in this map
        log.trace("Found map: %s %c%c%c %lx-%lx", name, r, w, x, start, end);
        if (len >= 18 && memcmp(name, "/usr/lib/valgrind/", 18) == 0)
            continue;
        if (len >= 8 && memcmp(name, "/system/", 8) == 0 &&
                !(len >= 25 && memcmp(name, "/system/lib/libandroid.so", 25 + 1) == 0)) {
            continue;
        }
        std::string nameStd(name);
        if (libsToFind.count(nameStd) > 0)
            libsToFind.erase(nameStd);
        if (librariesByPath.count(nameStd) <= 0) {
            // this lib is new; add it
            if (createLibraryInfo(nameStd) == nullptr)
                continue;
        }
        librariesByPath.at(nameStd)->addMap(LibraryMemMap(start, end, r == 'r', w == 'w', x == 'x'));
    }
    // remove libs we didn't find
    for (auto& e : libsToFind) {
        destroyLibraryInfo(librariesByPath.at(e));
    }
}

HookManager::LibraryInfo* HookManager::createLibraryInfo(std::string const& path) {
    LibraryInfo* li = new LibraryInfo();
    li->ptr = dlopen(path.c_str(), RTLD_LAZY);
    if (li->ptr == nullptr) {
        log.trace("Not creating library info for: %s - error: %s", path.c_str(), dlerror());
        return nullptr;
    }
    li->path = path;

    log.trace("Creating library info for: %s", path.c_str());
    if (strlen(path.c_str()) <= 0)
        return nullptr;

    Elf32_Ehdr header;
    FILE* file = fopen(path.c_str(), "r");
    if (file == nullptr)
        return nullptr;

    if (fread(&header, sizeof(Elf32_Ehdr), 1, file) != 1) {
        log.error("Failed to read header!");
        fclose(file);
        return nullptr;
    }

    fseek(file, (long) header.e_shoff, SEEK_SET);

    char shdr[header.e_shentsize * header.e_shnum];
    if (fread(&shdr, header.e_shentsize, header.e_shnum, file) != header.e_shnum) {
        log.error("Failed to read shdr!");
        fclose(file);
        return nullptr;
    }
    log.trace("header.e_shnum = %i", header.e_shnum);

    // find strtab
    char* strtab = nullptr;
    for (int i = 0; i < header.e_shnum; i++) {
        Elf32_Shdr& entry = *((Elf32_Shdr*) &shdr[header.e_shentsize * i]);
        if (entry.sh_type == SHT_STRTAB) {
            log.trace("Found STRTAB");
            strtab = new char[entry.sh_size];
            fseek(file, (long) entry.sh_offset, SEEK_SET);
            if (fread(strtab, 1, entry.sh_size, file) != entry.sh_size) {
                log.error("Failed to read STRTAB!");
                fclose(file);
                return nullptr;
            }
        }
    }
    if (strtab == nullptr) {
        log.error("STRTAB not found!");
        fclose(file);
        return nullptr;
    }

    // find .got, .got.plt and .data.rel.ro
    for (int i = 0; i < header.e_shnum; i++) {
        Elf32_Shdr& entry = *((Elf32_Shdr*) &shdr[header.e_shentsize * i]);
        if (strcmp(&strtab[entry.sh_name], ".got") == 0) {
            log.trace("Found .got!");
            li->gotOff = entry.sh_addr;
            li->gotSize = entry.sh_size;
        } else if (strcmp(&strtab[entry.sh_name], ".got.plt") == 0) {
            log.trace("Found .got.plt!");
            li->gotPltOff = entry.sh_addr;
            li->gotPltSize = entry.sh_size;
        } else if (strcmp(&strtab[entry.sh_name], ".data.rel.ro") == 0) {
            log.trace("Found .data.rel.ro!");
            li->dataRelRoOff = entry.sh_addr;
            li->dataRelRoSize = entry.sh_size;
        }
    }
    fclose(file);

    libraries[li->ptr] = li;
    librariesByPath[li->path] = li;
    return li;
}

void HookManager::LibraryInfo::addMap(LibraryMemMap mmap) {
    for (auto& map : memMaps) {
        if (map.start == mmap.start && map.end == mmap.end)
            return;
    }
    memMaps.push_back(mmap);
    if (mmap.r && mmap.x && !mmap.w) {
        if (mprotect((void*) mmap.start, mmap.end - mmap.start, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
            std::cerr << "mprotect() of code section failed. We have to use a hacky patch to patch code in this lib.\n";
            memMaps.back().needsHackyPatchToWork = true;
            mightNeedHackyPatch = true;
        }
    } else if (mmap.r && !mmap.x && !mmap.w) {
        if (mprotect((void*) mmap.start, mmap.end - mmap.start, PROT_READ | PROT_WRITE) != 0) {
            std::cerr << "mprotect() of data section failed, some stuff might be broken.\n";
        }
    }
}

void HookManager::destroyLibraryInfo(LibraryInfo* libraryInfo) {
    libraries.erase(libraryInfo->ptr);
    librariesByPath.erase(libraryInfo->path);
}

void HookManager::findSymbolAndAddTo(std::vector<void*>& arr, Elf32_Addr base, Elf32_Addr soSize, Elf32_Addr off,
                                     Elf32_Addr size, void* sym) {
    if (off == 0 || off >= soSize)
        return;
    size = std::min(size, soSize - off);
    unsigned long addr = base + off + 4;
    while (addr < base + off + size) {
        if (*((void**) addr) == sym) {
            if (hookedSymbolRefs.count((void*) addr) > 0)
                continue;
            arr.push_back((void*) addr);
            hookedSymbolRefs.insert((void*) addr);
        }
        addr += sizeof(void*);
    }
}

void HookManager::HookSymbol::useSymbol(HookManager* mgr, void* newSym) {
    for (auto& up : usage) {
        if (mgr->libraries.count(up.first) <= 0)
            continue;
        for (void* u : up.second)
            *((void**) u) = newSym;
    }
    for (void** u : customRefs)
        *u = newSym;
}

tml::HookManager::HookSymbol* HookManager::getSymbol(void* lib, std::string const& str, bool initialize) {
    SymbolLibNameDesc p = {lib, str};

    HookSymbol* hookSymbol;
    if (symbols.count(p) > 0) {
        hookSymbol = symbols.at(p);
        if (hookSymbol->initialized || !initialize)
            return hookSymbol;
    } else {
        void* sym = dlsym(lib, str.c_str());
        if (sym == nullptr)
            sym = dlsym_weak(lib, str.c_str());
        if (sym == nullptr)
            throw std::runtime_error("Failed to find symbol " + str);
        hookSymbol = new HookSymbol();
        hookSymbol->libNameDesc = p;
        hookSymbol->usedSymbol = hookSymbol->originalSym = sym;
        if (!initialize)
            return hookSymbol;
    }

    for (auto& lp : libraries) {
        soinfo* si = (soinfo*) lp.second->ptr;
        findSymbolAndAddTo(hookSymbol->usage[lp.second->ptr], si->base, si->size, lp.second->gotOff,
                           lp.second->gotSize, hookSymbol->originalSym);
        findSymbolAndAddTo(hookSymbol->usage[lp.second->ptr], si->base, si->size, lp.second->gotPltOff,
                           lp.second->gotPltSize, hookSymbol->originalSym);
        findSymbolAndAddTo(hookSymbol->usage[lp.second->ptr], si->base, si->size, lp.second->dataRelRoOff,
                           lp.second->dataRelRoSize, hookSymbol->originalSym);
    }
    hookSymbol->initialized = true;
    symbols[p] = hookSymbol;
    return hookSymbol;
}

void HookManager::destroySymbol(HookSymbol* symbol) {
    symbol->useSymbol(this, symbol->originalSym);
    for (auto& up : symbol->usage) {
        if (libraries.count(up.first) <= 0)
            continue;
        for (auto& u : up.second)
            hookedSymbolRefs.erase(u);
    }
    for (auto& u : symbol->customRefs)
        customRefToSymbol.erase(u);
    symbols.erase({symbol->libNameDesc.lib, symbol->libNameDesc.name});
    delete symbol;
}

tml::HookManager::HookInfo* HookManager::hook(void* lib, std::string const& sym, void* override, void** org) {
    HookInfo* hookInfo = new HookInfo();
    hookInfo->symbol = getSymbol(lib, sym);
    hookInfo->overrideSym = override;
    hookInfo->userOrgSym = org;
    hookInfo->parent = hookInfo->symbol->hook;
    if (hookInfo->parent != nullptr)
        hookInfo->parent->child = hookInfo;
    if (hookInfo->userOrgSym != nullptr)
        *hookInfo->userOrgSym = (hookInfo->parent != nullptr ? hookInfo->parent->overrideSym :
                                 hookInfo->symbol->usedSymbol);
    hookInfo->symbol->useSymbol(this, override);
    hookInfo->symbol->hook = hookInfo;
    return hookInfo;
}

void HookManager::unhook(HookInfo* hook) {
    if (hook->child == nullptr) {
        if (hook->parent == nullptr) {
            if (hook->symbol->customRefs.size() == 0) {
                destroySymbol(hook->symbol);
            } else {
                hook->symbol->useSymbol(this, hook->symbol->originalSym);
            }
        } else {
            hook->symbol->useSymbol(this, hook->parent->symbol);
        }
    } else {
        hook->child->parent = hook->parent;
        if (hook->parent != nullptr) {
            hook->parent->child = hook->child;
            *hook->child->userOrgSym = hook->parent->overrideSym;
        } else {
            *hook->child->userOrgSym = hook->symbol->originalSym;
        }
    }
    delete hook;
}

void HookManager::addCustomRef(void** ref, void* lib, std::string const& str) {
    removeCustomRef(ref);
    HookSymbol* symbol = getSymbol(lib, str, false);
    symbol->customRefs.insert(ref);
    customRefToSymbol[ref] = symbol;
}

void HookManager::removeCustomRef(void** ref) {
    if (customRefToSymbol.count(ref) > 0) {
        HookSymbol* sym = customRefToSymbol.at(ref);
        customRefToSymbol.erase(ref);
        sym->customRefs.erase(ref);
        if (sym->hook == nullptr && sym->customRefs.size() == 0)
            destroySymbol(sym);
    }
}