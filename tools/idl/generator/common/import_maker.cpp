#include "common/import_maker.h"

#include <boost/regex.hpp>

#include <algorithm>
#include <map>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace common {

bool operator<(const ForwardDeclaration& l, const ForwardDeclaration& r)
{
    return l.typeName < r.typeName;
}

namespace {

/**
 * Returns import path's "group tag".
 */
std::string importGroupTag(const std::string& path)
{
    static const boost::regex REGEX("\\W*(\\w+)\\W+\\w+.*");

    boost::smatch match;
    if (boost::regex_match(path, match, REGEX)) {
        return std::string(match[1]);
    } else {
        return "";
    }
}

} // namespace

ImportMaker::ImportMaker(
    const Idl* idl,
    const TypeNameMaker* typeNameMaker,
    std::vector<std::string> importGroupTags,
    size_t wildcardGroupIndex)
    : idl_(idl),
      typeNameMaker_(typeNameMaker),
      importGroupTags_(importGroupTags),
      wildcardGroupIndex_(wildcardGroupIndex)
{
}

ImportMaker::~ImportMaker()
{
}

void ImportMaker::addImportPath(const std::string& importPath)
{
    importPaths_.insert(importPath);
}

void ImportMaker::addCppRuntimeImportPath(const std::string& relativePath)
{
    auto path = runtimeFramework()->cppNamespace.asPath() + relativePath;
    addImportPath(path.inAngles());
}
void ImportMaker::addCsRuntimeImportPath(const std::string& relativePath)
{
    auto path = runtimeFramework()->csNamespace.asPath() + relativePath;
    addImportPath(path.inAngles());
}
void ImportMaker::addJavaRuntimeImportPath(const std::string& relativePath)
{
    addImportPath(
        runtimeFramework()->javaPackage.asPrefix(".") + relativePath);
}
void ImportMaker::addObjCRuntimeImportPath(const std::string& relativePath)
{
    auto path = runtimeFramework()->objcFramework +
        runtimeFramework()->objcFrameworkPrefix.asPath().withSuffix(relativePath);
    addImportPath(path.inAngles());
}

const std::set<std::string>& ImportMaker::getImportPaths() const
{
    return importPaths_;
}

void ImportMaker::addDefinedType(const FullTypeRef& interfaceTypeRef)
{
    alreadyDefinedInterfaceNames_.insert(
        typeNameMaker_->makeConstructorName(interfaceTypeRef.info()));
}

bool ImportMaker::addForwardDeclaration(
    const FullTypeRef& typeRef,
    const std::string& importPath)
{
    if (typeRef.info().idl->idlNamespace == idl_->idlNamespace) {
        auto typeName = typeNameMaker_->makeConstructorName(typeRef.info());
        addForwardDeclaration(typeName, importPath, &typeRef.info());

        return true;
    } else {
        return false;
    }
}

void ImportMaker::addForwardDeclaration(
    const std::string& typeName,
    const std::string& importPath,
    const TypeInfo* typeInfo)
{
    if (alreadyDefinedInterfaceNames_.find(typeName) ==
            alreadyDefinedInterfaceNames_.end()) {
        forwardDeclarations_.insert({ typeName, importPath, typeInfo });
    }
}

void ImportMaker::fill(
    const std::string& selfImportPath,
    ctemplate::TemplateDictionary* parentDict)
{
    // Import paths
    importPaths_.erase(selfImportPath);
    for (const auto& set : groupImportPaths()) {
        if (!set.empty()) {
            auto dict = parentDict->AddSectionDictionary("IMPORT_SECTION");
            for (const auto& path : set) {
                dict->SetValueAndShowSection("IMPORT_PATH", path, "IMPORT");
            }
        }
    }

    // Forward declarations
    std::map<std::string, const TypeInfo*> fwdDeclsToAdd;
    for (const auto& fwdDecl : forwardDeclarations_) {
        if (importPaths_.find(fwdDecl.importPath) == importPaths_.end()) {
            fwdDeclsToAdd[fwdDecl.typeName] = fwdDecl.typeInfo;
        }
    }
    if (fwdDeclsToAdd.size() > 0) {
        auto fwdDeclsDict = parentDict->AddSectionDictionary("FWD_DECLS");
        for (const auto& fwdDecl : fwdDeclsToAdd) {
            auto fwdDeclDict = fwdDeclsDict->AddSectionDictionary("FWD_DECL");
            if (fwdDecl.second) {
                if (std::holds_alternative<const nodes::Listener*>(fwdDecl.second->type)) {
                    fwdDeclDict->ShowSection("LISTENER");
                } else if (std::holds_alternative<const nodes::Interface*>(fwdDecl.second->type)) {
                    fwdDeclDict->ShowSection("INTERFACE");
                } else if (std::holds_alternative<const nodes::Struct*>(fwdDecl.second->type)) {
                    fwdDeclDict->ShowSection("STRUCT");
                } else if (std::holds_alternative<const nodes::Variant*>(fwdDecl.second->type)) {
                    fwdDeclDict->ShowSection("VARIANT");
                }
            } else {
                fwdDeclDict->ShowSection("INTERFACE");
            }

            fwdDeclDict->SetValue("TYPE_NAME", fwdDecl.first);
        }
    }
}

std::vector<std::set<std::string>> ImportMaker::groupImportPaths()
{
    // Grouping is done by storing each group (set) in a map with positional
    // index as key.
    std::map<size_t, std::set<std::string>> sets;

    for (const auto& path : importPaths_) {
        auto iterator = std::find(importGroupTags_.begin(),
            importGroupTags_.end(), importGroupTag(path));

        size_t groupIndex = iterator - importGroupTags_.begin();
        if (iterator == importGroupTags_.end()) {
            groupIndex = wildcardGroupIndex_;
        } else if (groupIndex >= wildcardGroupIndex_) {
            ++groupIndex;
        }

        sets[groupIndex].insert(path);
    }

    // Convert map into a vector (map is an implementation detail).
    std::vector<std::set<std::string>> result;
    for (const auto& entry : sets) {
        result.push_back(entry.second);
    }
    return result;
}

} // namespace common
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
