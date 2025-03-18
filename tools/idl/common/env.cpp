#include <yandex/maps/idl/env.h>

#include "validators/validators.h"

#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/nodes/name.h>
#include <yandex/maps/idl/nodes/nodes.h>
#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/nodes/variant.h>
#include <yandex/maps/idl/parser/parse_framework.h>
#include <yandex/maps/idl/parser/parse_idl.h>
#include <yandex/maps/idl/targets.h>
#include <yandex/maps/idl/functions.h>
#include <yandex/maps/idl/utils/errors.h>
#include <yandex/maps/idl/utils/exception.h>
#include <yandex/maps/idl/utils/make_unique.h>

#include <boost/functional/hash.hpp>

#include <cstddef>
#include <fstream>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {

Environment::Environment(Config config)
    : config(config),
      runtimeFramework_(getFramework(utils::Path("runtime.framework")))
{
}

const Idl* Environment::idl(const utils::Path& relativePath)
{
    auto ptr = idls_.getCached(relativePath);
    if (!ptr) {
        ptr = idls_.get(config.inIdlSearchPaths, relativePath,
            [this] (
                const utils::Path& searchPath,
                const utils::Path& relativePath,
                const std::string& fileContents)
            {
                return buildIdl(searchPath, relativePath, fileContents);
            });

        registerTypes(ptr);

        auto errors = validate(ptr);
        if (!errors.empty()) {
            throw utils::GroupedError(
                relativePath, "contains following errors", errors);
        }
    }
    return ptr;
}

namespace {

/**
 * "Appends" name to given hash code. See comments to the hashAppend(...)
 * below for why we need this function.
 */
std::size_t hashAppend(std::size_t baseHash, const std::string& name)
{
    auto hash = baseHash;
    boost::hash_combine(hash, name);
    return hash;
}

/**
 * "Appends" scope - part-by-part - to given hash code.
 *
 * boost::hash_combine(...) computes second argument's hash code, and then
 * combines it with the first argument. On the other hand, we need to append
 * scope parts one-by-one - that's why we use boost::hash_range.
 */
std::size_t hashAppend(std::size_t baseHash, const Scope& scope)
{
    auto hash = baseHash;
    boost::hash_range(hash, scope.begin(), scope.end());
    return hash;
}
std::size_t hashAppend(std::size_t baseHash, const ExtendedScope& scope)
{
    auto hash = baseHash;
    for (const auto& name : scope) {
        boost::hash_combine(hash, name.original());
    }
    return hash;
}

} // namespace

const TypeInfo& Environment::type(
    const Idl* idl,
    const Scope& scope,
    const Scope& qualifiedName) const
{
    if (!qualifiedName.isEmpty() && qualifiedName.first().empty()) {
        // Fully-qualified name - search for it directly:
        auto found = types_.find(hashAppend(0, qualifiedName.subScope(1)));
        REQUIRE(found != types_.end(),
            "Could not find fully-qualified type name '" <<
                qualifiedName << "'");

        return found->second;
    }

    // Create all possible hashes, in the order of search priority
    const auto numberOfAllHashes =
        1 + idl->idlNamespace.size() + scope.size();
    std::vector<std::size_t> hashes(numberOfAllHashes);

    std::size_t currentPrefixHash = 0;
    for (std::size_t i = 0; i < idl->idlNamespace.size(); ++i) {
        auto hash = hashAppend(currentPrefixHash, qualifiedName);
        hashes[numberOfAllHashes - 1 - i] = hash;

        currentPrefixHash = hashAppend(currentPrefixHash, idl->idlNamespace[i]);
    }
    for (std::size_t i = 0; i < scope.size(); ++i) {
        auto hash = hashAppend(currentPrefixHash, qualifiedName);
        hashes[numberOfAllHashes - 1 - idl->idlNamespace.size() - i] = hash;

        currentPrefixHash = hashAppend(currentPrefixHash, scope[i]);
    }
    hashes[0] = hashAppend(currentPrefixHash, qualifiedName);

    // Find first matching type
    for (auto hash : hashes) {
        auto found = types_.find(hash);
        if (found != types_.end()) {
            return found->second;
        }
    }

    // Not found!
    INTERNAL_ERROR("Could not find '" << qualifiedName <<
        "' from '" << (idl->idlNamespace + scope) << "'");
}

const Framework* Environment::getFramework(const utils::Path& relativePath)
{
    auto ptr = frameworks_.getCached(relativePath);
    if (!ptr) {
        ptr = frameworks_.get(config.inFrameworkSearchPaths, relativePath,
            [] (
                const utils::Path& searchPath,
                const utils::Path& relativePath,
                const std::string& fileContents)
            {
                return utils::make_unique<const Framework>(
                    parser::parseFramework(searchPath + relativePath, fileContents));
            });
    }
    return ptr;
}

std::unique_ptr<const Idl> Environment::buildIdl(
    const utils::Path& searchPath,
    const utils::Path& relativePath,
    const std::string& fileContents)
{
    auto root = parser::parseIdl(searchPath + relativePath, fileContents);

    auto framework =
        getFramework(relativePath.first().withExtension("framework"));

    auto idlNamespace = Scope(relativePath.parent());

    auto innerNamespace = idlNamespace.subScope(1);
    auto cppNamespace = framework->cppNamespace + innerNamespace;
    auto csNamespace =
        framework->csNamespace + innerNamespace.camelCased(true);
    auto javaPackage = framework->javaPackage + innerNamespace;

    auto objcTypePrefix = framework->objcFrameworkPrefix;
    if (!root.objcInfix.empty()) {
        objcTypePrefix += root.objcInfix;
    }

    return utils::make_unique<const Idl>(Idl{
        this,

        std::move(relativePath),
        std::move(root),

        std::move(framework->bundleGuard),

        std::move(idlNamespace),

        std::move(cppNamespace),
        std::move(csNamespace),
        std::move(javaPackage),
        std::move(framework->objcFramework),

        std::move(objcTypePrefix)
    });
}

namespace {

/**
 * Traverses given Idl's syntax tree, and registers the data about all
 * types declared inside it.
 */
class TypeRegistrar : public nodes::Visitor {
public:
    TypeRegistrar(
        std::unordered_map<std::size_t, TypeInfo>* types,
        std::map<std::string, std::set<std::string>>* duplicates,
        const Idl* idl)
        : types_(types),
          duplicates_(duplicates),
          idl_(idl),
          namespaceHash_(hashAppend(0, idl_->idlNamespace))
    {
    }

private:
    using nodes::Visitor::onVisited;

    void onVisited(const nodes::Enum& e) override
    {
        registerType(&e);
    }
    void onVisited(const nodes::Interface& i) override
    {
        registerType(&i);
        traverseWithExtendedScope(extendedScope_, i, this);
    }
    void onVisited(const nodes::Listener& l) override
    {
        registerType(&l);
    }
    void onVisited(const nodes::Struct& s) override
    {
        registerType(&s);
        traverseWithExtendedScope(extendedScope_, s, this);
    }
    void onVisited(const nodes::Variant& v) override
    {
        registerType(&v);
    }

    Scope targetSpecificScope(const std::string& targetLang) const
    {
        Scope result;
        for (const auto& item : extendedScope_) {
            result += item[targetLang];
        }
        return result;
    }

    Scope fullIdlNameAsScope(const nodes::Name& name) const
    {
        return idl_->idlNamespace + targetSpecificScope("") + name.original();
    }

    Scope fullCppNameAsScope(const nodes::Name& name) const
    {
        return idl_->cppNamespace + targetSpecificScope("") + name.original();
    }
    Scope fullCsNameAsScope(const nodes::Name& name) const
    {
        auto scope = idl_->csNamespace;
        if (extendedScope_.empty()) {
            return scope + name[CS];
        } else {
            return scope + (extendedScope_.back()[CS] + name[CS]);
        }
    }
    Scope fullJavaNameAsScope(const nodes::Name& name) const
    {
        return idl_->javaPackage + targetSpecificScope(JAVA) + name[JAVA];
    }
    Scope fullObjCNameAsScope(const nodes::Name& name) const
    {
        auto scope = idl_->objcTypePrefix;
        if (!extendedScope_.empty()) {
            scope += extendedScope_.back()[OBJC];
        }
        return scope + name[OBJC];
    }

    std::optional<TypeInfo> enclosingTypeInfo()
    {
        if (!extendedScope_.empty()) {
            auto enclosingExtendedScope = extendedScope_;
            enclosingExtendedScope.pop_back();
            auto hash = hashAppend(hashAppend(namespaceHash_, enclosingExtendedScope), extendedScope_.back().original());
            auto found = types_->find(hash);
            if (found != types_->end())
                return found->second;
        }
        return std::nullopt;
    }

    /**
     * Adds new type info into the types map. Throws an exception if there is
     * already a type with the same fully-qualified name.
     */
    template <typename T>
    void registerType(const T* type)
    {
        auto enclosingType = enclosingTypeInfo();
        bool isInternalEnclosingType = enclosingType ? enclosingType->isInternal : false;

        auto hash = hashAppend(namespaceHash_, extendedScope_);
        hash = hashAppend(hash, type->name.original());

        TypeInfo typeInfo{
            idl_,
            {
                targetSpecificScope(""),
                {
                    { CS, targetSpecificScope(CS) },
                    { JAVA, targetSpecificScope(JAVA) },
                    { OBJC, targetSpecificScope(OBJC) }
                }
            },
            type->name,
            type,
            {
                fullIdlNameAsScope(type->name),
                {
                    { CPP, fullCppNameAsScope(type->name) },
                    { CS, fullCsNameAsScope(type->name) },
                    { JAVA, fullJavaNameAsScope(type->name) },
                    { OBJC, fullObjCNameAsScope(type->name) }
                }
            },
            hasInternalDoc(*type) || isInternalEnclosingType
        };

        auto insertion = types_->insert(std::make_pair(hash, typeInfo));
        if (!insertion.second) {
            (*duplicates_)[typeInfo].insert(idl_->relativePath);

            // Original type will be reinserted multiple times, but it's ok
            (*duplicates_)[insertion.first->second].insert(
                insertion.first->second.idl->relativePath);
        }
    }

private:
    std::unordered_map<std::size_t, TypeInfo>* types_;
    std::map<std::string, std::set<std::string>>* duplicates_;

    const Idl* idl_;
    const std::size_t namespaceHash_;

    ExtendedScope extendedScope_;
};

void ensureNoDuplicates(
    const std::unordered_map<std::size_t, TypeInfo>& types,
    const std::map<std::string, std::set<std::string>>& duplicates)
{
    if (!duplicates.empty()) {
        std::string errorMessage =
            "Following types were declared multiple times:\n";
        for (const auto& duplicate : duplicates) {
            errorMessage += "    '" + duplicate.first + "' declared in:\n";
            for (const auto& path : duplicate.second) {
                errorMessage += "        " + path + "\n";
            }
        }

        throw utils::UsageError() << errorMessage;
    } else { // Duplicates are empty, check target-specific duplicates
        std::string LANG_TABLE[][3] = {
            { "C++",         CPP,  "::" }, // Language, targetLang and delimiter
            { "C#",          CS,   "."  },
            { "Java",        JAVA, "."  },
            { "Objective-C", OBJC, ""   }
        };

        std::string fullErrorMessage;
        for (const auto& lang : LANG_TABLE) {
            std::set<std::string> fullScopes;
            std::map<std::string, std::set<std::string>> fullScopeDuplicates;
            for (const auto& typeInfoKV : types) {
                const auto& typeInfo = typeInfoKV.second;
                auto fullScope = typeInfo.fullNameAsScope[lang[1]];

                if (lang[1] == OBJC) {
                    auto i = std::get_if<const nodes::Interface*>(&typeInfo.type);
                    if (i && (**i).isStatic) {
                        fullScope += typeInfo.name.original();
                    }
                }

                if (!fullScopes.insert(fullScope).second) {
                    fullScopeDuplicates[fullScope.asString(lang[2])].insert(
                        typeInfoKV.second.idl->relativePath);
                }
            }

            if (!fullScopeDuplicates.empty()) {
                auto errorMessage = "Following " + lang[0] +
                    " types were declared multiple times:\n";

                for (const auto& maybeDuplicate : fullScopeDuplicates) {
                    errorMessage +=
                        "    '" + maybeDuplicate.first + "' declared in:\n";
                    for (const auto& path : maybeDuplicate.second) {
                        errorMessage += "        " + path + "\n";
                    }
                }

                fullErrorMessage += errorMessage + "\n";
            }
        }

        if (!fullErrorMessage.empty()) {
            throw utils::UsageError() << fullErrorMessage;
        }
    }
}

} // namespace

void Environment::registerTypes(const Idl* idl)
{
    TypeRegistrar registrar(&types_, &duplicateTypes_, idl);
    idl->root.nodes.traverse(&registrar);

    for (const auto& importPath : idl->root.imports) {
        // Recursively call this method to import new .idl
        Environment::idl(importPath);
    }

    ensureNoDuplicates(types_, duplicateTypes_);
}

} // namespace idl
} // namespace maps
} // namespace yandex
