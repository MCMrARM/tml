#include <tml/modmeta.h>

#include <tml/mod.h>
#include <cstdlib>
#include <yaml.h>

using namespace tml;

ModVersion::ModVersion(const char* str) {
    sscanf(str, "%i.%i.%i", &major, &minor, &patch);
}

ModVersion getDepVersionPart(const char* str) {
    const char* strEnd = strchr(str, '-');

    ModVersion ret;
    ret.major = std::atoi(str);
    str = strchr(str, '.');
    if (str == nullptr || (strEnd != nullptr && strEnd <= str))
        return ret;
    if (str[1] == '*') {
        ret.minor = ret.patch = -1;
        return ret;
    }
    ret.minor = std::atoi(&str[1]);
    str = strchr(&str[1], '.');
    if (str == nullptr || (strEnd != nullptr && strEnd <= str))
        return ret;
    ret.patch = (str[1] == '*' ? -1 : std::atoi(&str[1]));
    return ret;

}

ModDependencyVersion::ModDependencyVersion(const char* str) {
    const char* strTo = strchr(str, '-');
    if (strTo != nullptr) {
        from = getDepVersionPart(str);
        to = getDepVersionPart(strTo + 1);
        return;
    }
    to = from = getDepVersionPart(str);
}

int yamlCppStreamWrapper(void* data, unsigned char* buffer, size_t size, size_t* size_read) {
    std::istream* str = (std::istream*) data;
    if (str->eof()) {
        *size_read = 0;
        return 1;
    }
    if (!*str)
        return 0;
    str->read((char*) buffer, size);
    *size_read = (size_t) str->gcount();
    return 1;
}

#define CString(keyNode, valueNode, yamlName, varName) \
    if (strcmp((char*) keyNode->data.scalar.value, yamlName) == 0 && valueNode->type == YAML_SCALAR_NODE) { \
        varName = std::string((char*) valueNode->data.scalar.value); \
    }

ModMeta::ModMeta(std::istream& ins) {
    yaml_parser_t parser;
    if (!yaml_parser_initialize(&parser))
        throw std::runtime_error("Failed to initialize YAML Parser");
    yaml_parser_set_input(&parser, yamlCppStreamWrapper, &ins);
    yaml_document_t document;
    if (!yaml_parser_load(&parser, &document))
        throw std::runtime_error("Failed to parse YAML document");

    yaml_node_t* rootNode = yaml_document_get_root_node(&document);
    for (yaml_node_pair_t* pair = rootNode->data.mapping.pairs.start; pair < rootNode->data.mapping.pairs.top; pair++) {
        yaml_node_t* keyNode = yaml_document_get_node(&document, pair->key);
        yaml_node_t* valueNode = yaml_document_get_node(&document, pair->value);
        if (keyNode->type != YAML_SCALAR_NODE)
            continue;

        CString(keyNode, valueNode, "name", name)
        else CString(keyNode, valueNode, "desc", desc)
        else CString(keyNode, valueNode, "author", author)
        else CString(keyNode, valueNode, "id", id)
        else if (strcmp((char*) keyNode->data.scalar.value, "version") == 0 && valueNode->type == YAML_SCALAR_NODE) {
            version = ModVersion((char*) valueNode->data.scalar.value);
        } else if (strcmp((char*) keyNode->data.scalar.value, "code") == 0 && valueNode->type == YAML_SEQUENCE_NODE) {
            for (yaml_node_item_t* itm = valueNode->data.sequence.items.start;
                 itm < valueNode->data.sequence.items.top; itm++) {
                yaml_node_t* codeNode = yaml_document_get_node(&document, *itm);
                if (codeNode->type != YAML_MAPPING_NODE)
                    continue;
                ModCode ent;
                for (yaml_node_pair_t* pair2 = codeNode->data.mapping.pairs.start;
                     pair2 < codeNode->data.mapping.pairs.top; pair2++) {
                    yaml_node_t* keyNode2 = yaml_document_get_node(&document, pair2->key);
                    yaml_node_t* valueNode2 = yaml_document_get_node(&document, pair2->value);
                    if (keyNode2->type != YAML_SCALAR_NODE)
                        continue;
                    CString(keyNode2, valueNode2, "loader", ent.loaderName)
                    else CString(keyNode2, valueNode2, "type", ent.loaderName)
                    else CString(keyNode2, valueNode2, "path", ent.codePath)
                    else CString(keyNode2, valueNode2, "name", ent.codePath)
                }
                if (ent.codePath.empty() || ent.loaderName.empty())
                    throw std::runtime_error("Invalid mod code entry");
                code.push_back(std::move(ent));
            }
        } else if (strcmp((char*) keyNode->data.scalar.value, "dependencies") == 0 &&
                   valueNode->type == YAML_SEQUENCE_NODE) {
            for (yaml_node_item_t* itm = valueNode->data.sequence.items.start;
                 itm < valueNode->data.sequence.items.top; itm++) {
                yaml_node_t* depNode = yaml_document_get_node(&document, *itm);
                if (depNode->type == YAML_SCALAR_NODE) {
                    // simple dependency
                    char* depStr = (char*) depNode->data.scalar.value;
                    char* depVer = strrchr(depStr, ':');
                    if (depVer == nullptr)
                        throw std::runtime_error("A dependency's version is not specified");
                    ModDependency dep;
                    dep.id = std::string(depStr, (size_t) (depVer - depStr));
                    dep.version.push_back(ModDependencyVersion(depVer + 1));
                    dependencies.push_back(dep);
                } else if (depNode->type == YAML_MAPPING_NODE) {
                    ModDependency dep;
                    for (yaml_node_pair_t* pair2 = depNode->data.mapping.pairs.start;
                         pair2 < depNode->data.mapping.pairs.top; pair2++) {
                        yaml_node_t* keyNode2 = yaml_document_get_node(&document, pair2->key);
                        yaml_node_t* valueNode2 = yaml_document_get_node(&document, pair2->value);
                        if (keyNode->type != YAML_SCALAR_NODE)
                            continue;
                        CString(keyNode2, valueNode2, "name", dep.id)
                        else CString(keyNode2, valueNode2, "id", dep.id)
                        else CString(keyNode2, valueNode2, "package", dep.id)
                        else CString(keyNode2, valueNode2, "package_id", dep.id)
                        else if (strcmp((char*) keyNode2->data.scalar.value, "version") == 0 ||
                                 strcmp((char*) keyNode2->data.scalar.value, "versions") == 0) {
                            if (valueNode2->type == YAML_SCALAR_NODE) {
                                dep.version.push_back(ModDependencyVersion((char*) valueNode2->data.scalar.value));
                            } else if (valueNode2->type == YAML_SEQUENCE_NODE) {
                                for (yaml_node_item_t* itm2 = valueNode2->data.sequence.items.start;
                                     itm2 < valueNode2->data.sequence.items.top; itm2++) {
                                    yaml_node_t* depVerNode = yaml_document_get_node(&document, *itm2);
                                    if (depVerNode->type == YAML_SCALAR_NODE)
                                        dep.version.push_back(ModDependencyVersion((char*) depVerNode->data.scalar.value));
                                }
                            }
                        }
                    }
                    if (dep.id.length() == 0)
                        continue;
                    if (dep.version.size() == 0)
                        throw std::runtime_error("A dependency's version is not specified");
                    dependencies.push_back(std::move(dep));
                }
            }
        }
    }
    yaml_document_delete(&document);
    yaml_parser_delete(&parser);
}

ModMeta::ModMeta(ModResources& resources) : ModMeta(*resources.open("package.yaml")) {
    //
}

bool ModMeta::areAllDependenciesResolved() const {
    for (auto& dep : dependencies) {
        if (!dep.mod->getMeta().areAllDependenciesResolved())
            return false;
    }
    return true;
}