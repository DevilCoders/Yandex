#pragma once

#include "common/import_maker.h"

#include <yandex/maps/idl/generator/protoconv/generator.h>

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/nodes.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/scope.h>

#include <ctemplate/template.h>

#include <set>
#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace protoconv {

/**
 * Structs with no proto-based fields have empty decode method.
 */
template <typename Node>
bool isDecoderEmpty(const Node& /* node */) { return false; }

template <>
inline bool isDecoderEmpty<nodes::Struct>(const nodes::Struct& s)
{
    return s.nodes.count<nodes::StructField>(
        [] (const nodes::StructField& f) { return f.protoField; }) == 0;
}

/**
 * Abstract base class for protobuf converter generators. Has common code
 * useful for both header and cpp files.
 */
class BaseGenerator : public nodes::Visitor {
public:
    BaseGenerator(const Idl* idl, common::ImportMaker importMaker);

    virtual ~BaseGenerator() = 0;

    /**
     * Simply traverses inside the interface, keeping track of scope.
     */
    virtual void onVisited(const nodes::Interface& i) override;

protected:
    /**
     * Adds given node's decode(...) function's dictionary.
     */
    template<typename Node>
    ctemplate::TemplateDictionary* addFunctionDict(const Node& node);

    /**
     * Expands the template. Should only be called at the end.
     */
    std::string expand();

protected:
    const Idl* idl_;

    common::ImportMaker importMaker_;

    Scope scope_;

    /**
     * Main dictionary that holds whole file.
     */
    ctemplate::TemplateDictionary mainDict_;
};

} // namespace protoconv
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
