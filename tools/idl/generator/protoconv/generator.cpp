#include <yandex/maps/idl/generator/protoconv/generator.h>

#include "common/common.h"
#include "protoconv/common.h"
#include "protoconv/cpp_generator.h"
#include "protoconv/header_generator.h"
#include "protoconv/error_checker.h"

#include <yandex/maps/idl/config.h>
#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/utils/errors.h>

#include <string>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace protoconv {

namespace {

/**
 * Tells whether converters will be generated or no.
 */
class NoGeneratedConvertersChecker : public nodes::Visitor {
public:
    bool check(const nodes::Root& root)
    {
        noConvertersWillBeGenerated_ = true;
        root.nodes.traverse(this);
        return noConvertersWillBeGenerated_;
    }

    void onVisited(const nodes::Enum& e) override
    {
        visit(e);
    }
    void onVisited(const nodes::Interface& i) override
    {
        i.nodes.traverse(this);
    }
    void onVisited(const nodes::Struct& s) override
    {
        visit(s);
        s.nodes.traverse(this);
    }
    void onVisited(const nodes::Variant& v) override
    {
        visit(v);
    }

private:
    bool noConvertersWillBeGenerated_;

    template <typename Node>
    void visit(const Node& n)
    {
        if (n.protoMessage) { // if the type needs to have .proto converter...
            if (!n.customCodeLink.protoconvHeader) { // and it's not custom...
                noConvertersWillBeGenerated_ = false;
            }
        }
    }
};

} // namespace

std::vector<OutputFile> generate(const Idl* idl)
{
    if (NoGeneratedConvertersChecker().check(idl->root)) {
        return { };
    }

    const auto& config = idl->env->config;

    auto errors = ErrorChecker(idl).check();
    if (!errors.empty()) {
        throw utils::GroupedError(idl->relativePath,
            "contains following Protobuf-related errors", errors);
    }

    return {
        HeaderGenerator(idl, config.outBaseHeadersRoot).generate(),
        CppGenerator(idl, config.outBaseImplRoot).generate()
    };
}

} // namespace protoconv
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
