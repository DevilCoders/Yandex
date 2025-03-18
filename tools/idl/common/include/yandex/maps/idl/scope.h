#pragma once

#include <yandex/maps/idl/nodes/name.h>
#include <yandex/maps/idl/nodes/nodes.h>
#include <yandex/maps/idl/utils/exception.h>
#include <yandex/maps/idl/utils/paths.h>
#include <yandex/maps/idl/utils/wrapper.h>

#include <cstddef>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {

/**
 * The scope of an identifier.
 *
 * E.g. scope of "C.D e" field in next snippet is { "A", "B" }:
 *
 * "mapkit/search/session.idl":
 * struct A {
 *   struct B {
 *     C.D e;
 *   }
 * }
 *
 * Sometimes, it is a full scope, e.g. { "mapkit", "search", "A", "B" } in the
 * example above. To avoid confusion, please name scope variables accordingly.
 */
class Scope : public utils::Wrapper<std::vector<std::string>> {
public:
    using Items = std::vector<std::string>;

    using Wrapper_::Wrapper;
    Scope() { } // "using" statement above does not inherit this constructor

    explicit Scope(std::string item) : Wrapper({ item }) { }
    explicit Scope(const char* item) : Scope(std::string(item)) { }

    Scope(std::initializer_list<const char*> items);

    /**
     * Splits string str with given char delimiter. E.g. splits
     * ("part1" "delimiter" "part2" "delimiter" "part3") into
     * Scope({"part1", "part2", "part2"}).
     */
    Scope(const std::string& str, char delimiter);

    /**
     * Creates .idl file's namespace from the directories where it is placed.
     *
     * Node: only .idl file's directories are part of its scope / namespace.
     * Do not pass file's relative path here, because it includes file's name,
     * which is NOT a part of the namespace. That's why we check for path's
     * extension inside the constructor.
     */
    explicit Scope(const utils::Path& dirsPath) : Scope(dirsPath, '/')
    {
        REQUIRE(
            dirsPath.extension() == "",
            "Cannot create a namespace from a file - leave dirs only: " <<
                dirsPath.inQuotes());
    }

    bool isEmpty() const { return value_.empty(); }
    std::size_t size() const { return value_.size(); }

    /**
     * {"Some", "Class"} => "Some.Class" for delimiter "."
     * {"name", "space", "Class"} => "name::space::Class" for "::" delimiter
     * {} => "" for any delimiter
     */
    std::string asString(const std::string& delimiter) const;

    /**
     * {"Some", "Class"} => "Some.Class." for delimiter "."
     * {"name", "space", "Class"} => "name::space::Class::" for "::" delimiter
     * {} => "" for any delimiter
     */
    std::string asPrefix(const std::string& delimiter) const;

    operator std::string() const { return asString("."); }

    utils::Path asPath() const { return utils::Path(asString("/")); }

    const std::string& operator[](std::size_t i) const;
    std::string& operator[](std::size_t i);

    const std::string& first() const;
    const std::string& last() const;

    Items::reverse_iterator rbegin() { return value_.rbegin(); }
    Items::reverse_iterator rend() { return value_.rend(); }
    Items::const_reverse_iterator rbegin() const { return value_.rbegin(); }
    Items::const_reverse_iterator rend() const { return value_.rend(); }

    /**
     * Returns a copy of the [start, end) region in this scope.
     */
    Scope subScope(std::size_t start, std::size_t end) const;

    Scope subScope(std::size_t start) const { return subScope(start, size()); }

    /**
     * Returns a copy of this scope, but with all items capitalized.
     */
    Scope capitalized() const;

    /**
     * Returns a copy of this scope, but with all items camel-cased.
     */
    Scope camelCased(bool capitalizeFirstSymbol = false) const;

    Scope& operator+=(const std::string& item);
    Scope& operator+=(const Scope& other);

    Scope& operator--();
};

Scope operator+(const Scope& left, const Scope& right);

Scope operator+(const Scope& scope, const std::string& item);
Scope operator+(const std::string& item, const Scope& scope);

std::ostream& operator<<(std::ostream& out, const Scope& scope);

/**
 * Adds current item's name to scope when constructed, and removes it - when
 * destructed. Makes it easier to maintain scope inside onVisited methods.
 */
class ScopeGuard {
public:
    ScopeGuard(Scope* scope, const std::string& item)
        : scope_(scope)
    {
        *scope_ += item;
    }

    ~ScopeGuard()
    {
        --*scope_;
    }

private:
    Scope* scope_;
};

/**
 * Traverses nodes's children (but not their children), and updates the scope.
 */
template <typename Node>
void traverseWithScope(
    Scope& scope,
    const Node& node,
    nodes::Visitor* visitor)
{
    ScopeGuard guard(&scope, node.name.original());
    node.nodes.traverse(visitor);
}

template <typename Node, typename ...Lambdas>
void lambdaTraverseWithScope(
    Scope& scope,
    const Node& node,
    Lambdas... lambdas)
{
    ScopeGuard guard(&scope, node.name.original());
    node.nodes.lambdaTraverse(lambdas...);
}

/**
 * Scope that holds not only pure Idl names, but also target language-specific
 * overrides.
 */
using ExtendedScope = std::vector<nodes::Name>;

template <typename Node>
void traverseWithExtendedScope(
    ExtendedScope& extendedScope,
    const Node& node,
    nodes::Visitor* visitor)
{
    extendedScope.push_back(node.name);
    node.nodes.traverse(visitor);
    extendedScope.pop_back();
}

} // namespace idl
} // namespace maps
} // namespace yandex
