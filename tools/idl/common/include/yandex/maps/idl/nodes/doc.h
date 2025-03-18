#pragma once

#include <yandex/maps/idl/nodes/type_ref.h>
#include <yandex/maps/idl/scope.h>

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {
namespace nodes {

/**
 * Represents a link tag inside the doc object. See wiki for more info.
 */
struct DocLink {
    /**
     * Scope of the field / function that link refers to.
     *
     * Can be empty for "relative" references, e.g. {@link #someMember...}.
     *
     * Also, scope doesn't have to be a type - can refer to a namespace
     * instead. But that must be the namespace of at least one of the
     * imported files, because we need to have its target-specific "forms".
     */
    Scope scope;

    /**
     * Can be empty if link refers to the type, and not its field / function,
     * e.g. {@link name.space.Some.Type}.
     */
    std::string memberName;

    /**
     * Vector is not initialized if this link refers to something other then a
     * function - it's the only way to check whether link refers to a function!
     */
    std::optional<std::vector<TypeRef>> parameterTypeRefs;
};

/**
 * Represents parsed documentation block - text with inline links.
 */
struct DocBlock {
    /**
     * Documentation string ready for boost::format, i.e. has %1% where first
     * link will go, %2% for second, ...
     */
    std::string format;

    std::vector<DocLink> links;

    void operator+=(const DocBlock& added) {
        links.insert(links.end(), added.links.begin(), added.links.end());
        format += added.format;
    }
};

/**
 * Represents parsed documentation for a function.
 */
struct Doc {
    enum Status {
        /**
        * Block of code has commercial content
        */
        Commercial,

        /**
         * Define exclusion this block of code from the documentation
         */
        Internal,
        /**
         * Define exclusion this block of code from the documentation
         */
        Undocumented,
        Default
    };
    Status status = Status::Default;

    /**
     * Description section:
     *  - for types and fields it is simply all documentation
     *  - for functions it is everything before first @param or @return
     */
    DocBlock description;

    /**
     * Parameter sections. Each one is a pair of parameter name and its
     * documentation. For types and fields it is empty.
     */
    std::vector<std::pair<std::string, DocBlock>> parameters;

    /**
     * Function return value documentation. For types and fields it has empty
     * format and links fields.
     */
    DocBlock result;
};

inline bool isExcludeDoc(const std::optional<Doc>& doc) 
{
    return doc && (doc->status == Doc::Status::Internal || doc->status == Doc::Status::Undocumented);
}

template <typename Node>
inline bool hasInternalDoc(const Node& n)
{
    return n.doc && n.doc->status == Doc::Status::Internal;
}

} // namespace nodes
} // namespace idl
} // namespace maps
} // namespace yandex
