#pragma once

#include "common/type_name_maker.h"

#include <yandex/maps/idl/framework.h>
#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/name.h>
#include <yandex/maps/idl/nodes/type_ref.h>
#include <yandex/maps/idl/scope.h>
#include <yandex/maps/idl/type_info.h>

#include <ctemplate/template.h>

#include <cstddef>
#include <set>
#include <string>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace common {

/**
 * Contains info for type's forward declaration.
 */
struct ForwardDeclaration {
    /**
     * Name of a type to forward declare.
     */
    std::string typeName;

    /**
     * File path where the type is defined. It is used to avoid forward
     * declaring types whose files are already imported for some reason.
     */
    std::string importPath;

    /**
     * Some languages may need the whole info. It is nullptr for types defined
     * outside Idl.
     */
    const TypeInfo* typeInfo;
};

/**
 * Needed to hold forward declarations in a set.
 */
bool operator<(const ForwardDeclaration& l, const ForwardDeclaration& r);

/**
 * Gathers import paths and forward declarations. Then fills them (grouped and
 * sorted if needed) into the given dictionary.
 */
class ImportMaker {
public:
    /**
     * Constructs import maker.
     *
     * Import group tags are "main" words in import paths, e.g.
     *     "yandex"       in "<yandex/maps/.../map.h>",
     *     "sqlite"       in "<sqlite/sqlite.h>",
     *     ""             in "<vector>",
     *     "YandexMapKit" in "<YandexMapkit/YMKSearchAddress.h>",
     *     "ru"           in "ru.yandex.maps.mapkit.search.Link", or
     *     "android"      in "android.app.Activity"
     * They go in the order their groups will be placed in the output file.
     *
     * Wildcard group index is where (counting from 0) all non-matched groups
     * will be inserted.
     */
    ImportMaker(
        const Idl* idl,
        const TypeNameMaker* typeNameMaker,
        std::vector<std::string> importGroupTags,
        std::size_t wildcardGroupIndex);

    virtual ~ImportMaker();

    const Framework* runtimeFramework() const
    {
        return idl_->env->runtimeFramework();
    }

    /**
     * Adds all needed imports and forward declarations for given params.
     *
     * @param dontImportIfPossible - true for types that often create cyclic
     * dependencies. Some languages don't support such dependencies and
     * require forward declarations instead of imports. For all other types
     * this parameter must be false.
     */
    virtual void addAll(
        const FullTypeRef& /* typeRef */,
        bool /* withSerialization */,
        bool /* dontImportIfPossible */ = false)
    {
        // Empty implementation for protobuf generator
    }

    /**
     * Adds new import path.
     *
     * @param path - not just a path, but the whole value that goes into .tpl,
     * i.e. it will probably include <>, "" or any other wrapper.
     */
    void addImportPath(const std::string& importPath);

    void addCppRuntimeImportPath(const std::string& relativePath);
    void addCsRuntimeImportPath(const std::string& relativePath);
    void addJavaRuntimeImportPath(const std::string& relativePath);
    void addObjCRuntimeImportPath(const std::string& relativePath);

    /**
     * Gets all collected import paths.
     *
     * @return
     */
    const std::set<std::string>& getImportPaths() const;

    /**
     * Adds given interface's fully-qualified name to
     * alreadyDefinedInterfaceNames_.
     */
    void addDefinedType(const FullTypeRef& interfaceTypeRef);

    /**
     * Searches for a type by given parameters, and tries to add it to
     * forward declarations.
     *
     * @param importPath - see docs for ForwardDeclaration::importPath
     *
     * @return true - if successful. An example of failure is when in C++ a
     *                type from another namespace is being forward
     *                declared. That is not allowed, instead, the type will
     *                be included.
     */
    virtual bool addForwardDeclaration(
        const FullTypeRef& typeRef,
        const std::string& importPath);

    /**
     * Same as above but for a "virtual" type that is not declared in any
     * .idl but still needs a forward declaration, e.g. YRTCollection
     */
    virtual void addForwardDeclaration(
        const std::string& typeName,
        const std::string& importPath,
        const TypeInfo* typeInfo = nullptr);

    /**
     * Fills given dictionary with import paths and forward declarations.
     *
     * Import paths exclude "self" path to avoid a header importing itself),
     * and .tpl file must have sections like these ones (example is for C++):
     * {{#IMPORT_SECTION}}
     *   ...
     *   {{#IMPORT}}#include {{IMPORT_PATH}}{{/IMPORT}}
     *   ...
     * {{#IMPORT_SECTION}}.
     *
     * For forward declarations, the .tpl file should have sections like these
     * (example for Objective-C):
     * {{#FWD_DECLS}}
     *   ...
     *   {{#FWD_DECL}}@class {{TYPE_NAME}};{{/FWD_DECL}}
     *   ...
     * {{/FWD_DECLS}}
     */
    virtual void fill(
        const std::string& selfImportPath,
        ctemplate::TemplateDictionary* parentDict);

protected:
    /**
     * Groups accumulated import paths into separate sets, and stores them in
     * sorted order in a vector.
     */
    std::vector<std::set<std::string>> groupImportPaths();

protected:
    const Idl* idl_;
    const TypeNameMaker* typeNameMaker_;

    std::vector<std::string> importGroupTags_;
    std::size_t wildcardGroupIndex_;

    /**
     * All accumulated, but not yet grouped paths.
     */
    std::set<std::string> importPaths_;

    /**
     * These interface types must not be added to forward declarations, as
     * they are already defined somewhere before in the same file.
     */
    std::set<std::string> alreadyDefinedInterfaceNames_;

    /**
     * Accumulated forward declarations.
     */
    std::set<ForwardDeclaration> forwardDeclarations_;
};

} // namespace common
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
