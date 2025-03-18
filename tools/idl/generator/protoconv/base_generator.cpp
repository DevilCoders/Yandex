#include "protoconv/base_generator.h"

#include "common/common.h"
#include "cpp/common.h"
#include "cpp/type_name_maker.h"
#include "protoconv/common.h"
#include "protoconv/proto_file.h"
#include "tpl/tpl.h"

#include <yandex/maps/idl/config.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/protobuf.h>
#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/nodes/struct.h>
#include <yandex/maps/idl/nodes/variant.h>
#include <yandex/maps/idl/targets.h>

#include <boost/algorithm/string.hpp>

#include <ctemplate/template.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace protoconv {

BaseGenerator::BaseGenerator(
    const Idl* idl,
    common::ImportMaker importMaker)
    : idl_(idl),
      importMaker_(importMaker),
      mainDict_("MAIN_DICTIONARY")
{
}

BaseGenerator::~BaseGenerator()
{
}

void BaseGenerator::onVisited(const nodes::Interface& i)
{
    traverseWithScope(scope_, i, this);
}

namespace {

/**
 * Enables or disables CONST_REF section in .tpl - .proto enums need not be
 * taken by constant reference.
 */
template <typename Node>
void handlePbConstRefness(ctemplate::TemplateDictionary* dict)
{
    dict->ShowSection("CONST_REF");
}

template <>
void handlePbConstRefness<nodes::Enum>(ctemplate::TemplateDictionary* /* dict */)
{
}

} // namespace

template <typename Node>
ctemplate::TemplateDictionary* BaseGenerator::addFunctionDict(const Node& node)
{
    const auto& config = idl_->env->config;

    auto functionDict = mainDict_.AddSectionDictionary("FUNCTION");

    const auto& typeInfo = idl_->type(scope_, Scope(node.name.original()));
    functionDict->SetValue(
        "CPP_TYPE_FULL_NAME", cpp::fullName(typeInfo.fullNameAsScope[CPP]));

    handlePbConstRefness<Node>(functionDict);

    auto pbTypeNameWithScope = node.protoMessage->pathInProto;
    functionDict->SetValue("PB_TYPE_NAME", pbTypeNameWithScope.last());
    --pbTypeNameWithScope;

    auto protoNamespacePrefix = ProtoFile(
        config.inProtoRoot,
        config.baseProtoPackage,
        node.protoMessage->pathToProto).relativeNamespacePrefix();
    functionDict->SetValue("PB_TYPE_SCOPE",
        protoNamespacePrefix + pbTypeNameWithScope.asPrefix("::"));

    functionDict->ShowSection(
        isDecoderEmpty(node) ? "HAS_NO_BODY" : "HAS_BODY");

    return functionDict;
}

// Only Enum, Struct and Variant can have decode(...) functions.
template
ctemplate::TemplateDictionary* BaseGenerator::addFunctionDict(
    const nodes::Enum& e);
template
ctemplate::TemplateDictionary* BaseGenerator::addFunctionDict(
    const nodes::Struct& s);
template
ctemplate::TemplateDictionary* BaseGenerator::addFunctionDict(
    const nodes::Variant& v);

std::string BaseGenerator::expand()
{
    common::setNamespace(
        Scope(idl_->env->config.baseProtoPackage, '.'), &mainDict_);

    return tpl::expand("file.tpl", &mainDict_);
}

} // namespace protoconv
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
