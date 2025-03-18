#pragma once

#include "protoconv/base_generator.h"

#include <yandex/maps/idl/generator/protoconv/generator.h>

#include <yandex/maps/idl/generator/output_file.h>
#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/nodes/variant.h>
#include <yandex/maps/idl/utils/paths.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace protoconv {

/**
 * Generates protobuf converter's header file based on different "events" it
 * receives.
 */
class HeaderGenerator : public BaseGenerator {
public:
    HeaderGenerator(const Idl* idl, const utils::Path& prefixPath);

    OutputFile generate();

    using BaseGenerator::onVisited;
    virtual void onVisited(const nodes::Enum& e) override;
    virtual void onVisited(const nodes::Struct& s) override;
    virtual void onVisited(const nodes::Variant& v) override;

private:
    /**
     * Adds given node.
     */
    template <typename Node>
    void addNode(const Node& node);

    utils::Path prefixPath_;
};

} // namespace protoconv
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
