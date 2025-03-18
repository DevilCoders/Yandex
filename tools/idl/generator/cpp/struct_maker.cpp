#include "cpp/struct_maker.h"

#include "cpp/import_maker.h"
#include "cpp/serialization_addition.h"
#include "tpl/tpl.h"

#include <yandex/maps/idl/framework.h>
#include <yandex/maps/idl/targets.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace cpp {

StructMaker::StructMaker(
    const common::PodDecider* podDecider,
    const common::TypeNameMaker* typeNameMaker,
    const common::EnumFieldMaker* enumFieldMaker,
    common::ImportMaker* importMaker,
    const common::DocMaker* docMaker,
    bool isHeader,
    bool ignoreCustomCodeLink)
    : common::StructMaker(
          podDecider,
          typeNameMaker,
          enumFieldMaker,
          importMaker,
          docMaker),
      isHeader_(isHeader),
      ignoreCustomCodeLink_(ignoreCustomCodeLink)
{
}

ctemplate::TemplateDictionary* StructMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    FullScope scope,
    const nodes::Struct& s)
{
    if (!ignoreCustomCodeLink_ && s.customCodeLink.baseHeader) {
        importMaker_->addImportPath('<' + *s.customCodeLink.baseHeader + '>');
        return nullptr;
    }

    if (isHeader_) {
        if (s.kind == nodes::StructKind::Bridged) {
            importMaker_->addImportPath("<memory>");

            importMaker_->addCppRuntimeImportPath("bindings/platform.h");
        }
    }

    auto dict = common::StructMaker::make(parentDict, scope, s);

    auto isInterface = [scope] (const nodes::StructField& f) {
        return FullTypeRef(scope, f.typeRef).is<nodes::Interface>();
    };

    if (s.kind == nodes::StructKind::Options && !s.nodes.count<nodes::StructField>(isInterface)) {
        auto serializationDict = tpl::addSectionedInclude(
            dict, "SERIAL", "options_struct_serialization.tpl");

        serializationDict->SetValue(
            "RUNTIME_NAMESPACE_PREFIX",
            "::" + scope.idl->env->runtimeFramework()->cppNamespace.asPrefix("::"));

        addStructSerialization(
            importMaker_, serializationDict, scope, s, isHeader_);
    }

    return dict;
}

ctemplate::TemplateDictionary* StructMaker::makeField(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Struct& s,
    const nodes::StructField& f)
{
    auto dict = common::StructMaker::makeField(parentDict, scope, s, f);

    if (isHeader_) {
        if (f.typeRef.id == nodes::TypeId::Vector ||
                f.typeRef.id == nodes::TypeId::String ||
                f.typeRef.id == nodes::TypeId::Dictionary ||
                f.typeRef.id == nodes::TypeId::Any) {
            importMaker_->addCppRuntimeImportPath("bindings/platform.h");
        }
    }

    return dict;
}

} // namespace cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
