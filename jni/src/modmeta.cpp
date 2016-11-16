#include <tml/modmeta.h>

#include <tml/mod.h>

using namespace tml;

bool ModMeta::areAllDependenciesResolved() const {
    for (auto& dep : dependencies) {
        if (!dep.mod->getMeta().areAllDependenciesResolved())
            return false;
    }
    return true;
}