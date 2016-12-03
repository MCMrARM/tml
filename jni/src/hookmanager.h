#pragma once

#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <sys/exec_elf.h>
#include <tml/log.h>

namespace tml {

class ModLoader;

class HookManager {

private:
    Log log;

    void findSymbolAndAddTo(std::vector<void*>& arr, Elf32_Addr base, Elf32_Addr off, Elf32_Addr size, void* sym);

public:
    struct LibraryMemMap {
        size_t start, end;

        // the original state
        bool r, w, x;
        bool needsHackyPatchToWork = false;

        LibraryMemMap(size_t start, size_t end, bool r, bool w, bool x) : start(start), end(end), r(r), w(w), x(x) { }
    };
    struct LibraryInfo {
        void* ptr;
        std::string path;

        Elf32_Off gotOff = 0, gotSize;
        Elf32_Off gotPltOff = 0, gotPltSize;
        Elf32_Off dataRelRoOff = 0, dataRelRoSize;
        std::vector<LibraryMemMap> memMaps;
        bool mightNeedHackyPatch = false;

        void addMap(LibraryMemMap mmap);
    };

    struct HookSymbol;
    struct HookInfo {
        HookSymbol* symbol;
        HookInfo* parent = nullptr;
        HookInfo* child = nullptr;
        void* overrideSym;
        void** userOrgSym; // a user code reference to the symbol he'll call as the original function
    };
    struct SymbolLibNameDesc {
        void* lib;
        std::string name;
        bool operator==(SymbolLibNameDesc const& s2) const { return (lib == s2.lib && name == s2.name); }
    };
    struct SymbolLibNameDescHash {
        std::size_t operator()(SymbolLibNameDesc const& s) const {
            std::size_t h1 = std::hash<std::string>()(s.name);
            std::size_t h2 = std::hash<size_t>()((size_t) s.lib);
            return h1 ^ (h2 << 1);
        }
    };
    struct HookSymbol {
        SymbolLibNameDesc libNameDesc;

        void* originalSym = nullptr;
        void* usedSymbol = nullptr;

        std::unordered_map<void*, std::vector<void*>> usage; // pairs of library pointer => pointers in the GOT table

        HookInfo* hook = nullptr;

        void useSymbol(HookManager* mgr, void* newSym);
    };

    HookManager(ModLoader* loader);

    std::unordered_map<void*, LibraryInfo*> libraries; // library => LibraryInfo
    std::unordered_map<std::string, LibraryInfo*> librariesByPath; // library path => LibraryInfo
    std::unordered_map<SymbolLibNameDesc, HookSymbol*, SymbolLibNameDescHash> symbols; // { library, symbol name } => HookSymbol*
    std::set<void*> hookedSymbolRefs;

    void updateLoadedLibs();

    LibraryInfo* createLibraryInfo(std::string const& path);

    void destroyLibraryInfo(LibraryInfo* libraryInfo);

    HookSymbol* getSymbol(void* lib, std::string const& str);

    void destroySymbol(HookSymbol* symbol);

    HookInfo* hook(void* lib, std::string const& sym, void* override, void** org);

    void unhook(HookInfo* hook);

};

}
