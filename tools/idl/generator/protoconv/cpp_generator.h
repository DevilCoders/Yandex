#pragma once

#include "protoconv/base_generator.h"

#include <yandex/maps/idl/generator/protoconv/generator.h>

#include <yandex/maps/idl/generator/output_file.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/nodes/variant.h>
#include <yandex/maps/idl/utils/paths.h>

#include <ctemplate/template.h>

#include <functional>
#include <set>
#include <string>
#include <vector>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace protoconv {

class ProtoFile;

class CppGenerator : public BaseGenerator {
public:
    CppGenerator(const Idl* idl, const utils::Path& prefixPath);

    OutputFile generate();

    using BaseGenerator::onVisited;
    virtual void onVisited(const nodes::Enum& e) override;
    virtual void onVisited(const nodes::Struct& s) override;
    virtual void onVisited(const nodes::StructField& f) override;
    virtual void onVisited(const nodes::Variant& v) override;

private:
    /**
     * Adds .proto converter for given node.
     */
    template <typename Node>
    void addNode(
        const Node& node,
        const std::string& tplName,
        std::function<void(
            const Node& node,
            ctemplate::TemplateDictionary* dict)> dictBuilder);

    /**
     * Fills struct's or variant's field converter section.
     */
    template <typename Field>
    void fillFieldConverterSection(
        const Field& field,
        const std::string& pbTypeName,
        const std::string& pbFieldName,
        ctemplate::TemplateDictionary* fieldDict);

    /**
     * Creates C++ type name from TypeRef object,
     * taking into account the current scope where it was referenced.
     */
    std::string typeRefToString(
        const nodes::TypeRef& typeRef) const;

private:
    /**
     * Info about struct that is in the middle of being generated. Owned by
     * stack below.
     */
    struct StructInfo {
        const nodes::Struct* s;

        ctemplate::TemplateDictionary* bodyDictionary;
    };

    /**
     * When generating converters for some struct, we need to remember various
     * info about all its outer structs. Their converters are in the middle of
     * being generated, and we'll be often returning to them. Top of the stack
     * is an info about current innermost structure.
     */
    std::vector<StructInfo> structDataStack_;

    utils::Path prefixPath_;
};

} // namespace protoconv
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
