#include "protoconv/header_generator.h"

#include "common/common.h"
#include "cpp/common.h"
#include "protoconv/common.h"
#include "tpl/tpl.h"

#include <yandex/maps/idl/utils/paths.h>
#include <yandex/maps/idl/nodes/protobuf.h>

#include <ctemplate/template.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace protoconv {

namespace {

void addYandexExportInclude(common::ImportMaker* importMaker)
{
    importMaker->addImportPath("<yandex/maps/export.h>");
}

}

HeaderGenerator::HeaderGenerator(const Idl* idl, const utils::Path& prefixPath)
    : BaseGenerator(
          idl,
          {
              nullptr, nullptr, { "yandex", "boost", "" }, 1
          }
      ),
      prefixPath_(prefixPath)
{
    mainDict_.ShowSection("HEADER");
}

OutputFile HeaderGenerator::generate()
{
    tpl::DirGuard guard("protoconv");

    importMaker_.addImportPath(cpp::filePath(idl_, true).inAngles());

    idl_->root.nodes.traverse(this);

    importMaker_.fill("", &mainDict_);

    return {
        prefixPath_, filePath(idl_, true),
        BaseGenerator::expand(),
        importMaker_.getImportPaths()
    };
}

void HeaderGenerator::onVisited(const nodes::Enum& e)
{
    addNode(e);
}

void HeaderGenerator::onVisited(const nodes::Struct& s)
{
    addNode(s);
    traverseWithScope(scope_, s, this);
}

void HeaderGenerator::onVisited(const nodes::Variant& v)
{
    addNode(v);
}

template <typename Node>
void HeaderGenerator::addNode(const Node& node)
{
    if (node.protoMessage) {
        // custom type declaration header
        if (node.customCodeLink.baseHeader) {
            importMaker_.addImportPath(
                '<' + *node.customCodeLink.baseHeader + '>');
        }

        // protobuf-generated header
        const auto& pathToProto = node.protoMessage->pathToProto;
        importMaker_.addImportPath(
            utils::Path(pathToProto).withExtension("pb.h").inAngles());

        // decode() function declaration
        if (!node.customCodeLink.protoconvHeader) {
            addYandexExportInclude(&importMaker_);
            BaseGenerator::addFunctionDict(node)->ShowSection("DECL");
        }
    }
}

} // namespace protoconv
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
