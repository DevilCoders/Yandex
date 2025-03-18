#include "cpp/serialization_addition.h"

#include "cpp/import_maker.h"
#include "tpl/tpl.h"

#include <yandex/maps/idl/framework.h>
#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/targets.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace cpp {

void addStructSerialization(
    common::ImportMaker* importMaker,
    ctemplate::TemplateDictionary* serializationDict,
    const FullScope& scope,
    const nodes::Struct& s,
    bool /* isHeader */)
{
    FullTypeRef structTypeRef(scope, s.name);
    serializationDict->SetValue("SCOPE_PREFIX", scope.scope.asPrefix("::"));
    serializationDict->SetValue("CONSTRUCTOR_NAME", s.name[CPP]);

    auto fieldScope = scope + s.name.original();
    s.nodes.lambdaTraverse(
        [&](const nodes::StructField& f)
        {
            FullTypeRef typeRef(fieldScope, f.typeRef);

            importMaker->addAll(typeRef, true);

            auto fieldDict = serializationDict->AddSectionDictionary("FIELD");
            fieldDict->SetValue("FIELD_NAME", f.name);

            if (typeRef.isBridged()) {
                fieldDict->ShowSection("BRIDGED");
            }
            if (!typeRef.isOptional()) {
                fieldDict->ShowSection("NOT_OPTIONAL");
            }
        });
}

ExtSerialStructMaker::ExtSerialStructMaker(
    const common::TypeNameMaker* typeNameMaker,
    common::ImportMaker* importMaker,
    ctemplate::TemplateDictionary* rootDict,
    bool isHeader)
    : common::StructMaker(
          nullptr, typeNameMaker, nullptr, importMaker, nullptr),
      rootDict_(rootDict),
      isHeader_(isHeader)
{
    if (!isHeader) {
        importMaker_->addCppRuntimeImportPath(
            "bindings/internal/archive_generator.h");
        importMaker_->addCppRuntimeImportPath(
            "bindings/internal/archive_reader.h");
        importMaker_->addCppRuntimeImportPath(
            "bindings/internal/archive_writer.h");
    }
}

ctemplate::TemplateDictionary* ExtSerialStructMaker::make(
    ctemplate::TemplateDictionary* /* parentDict */,
    FullScope scope,
    const nodes::Struct& s)
{
    if (s.kind != nodes::StructKind::Options &&
            !s.customCodeLink.baseHeader) {
        if (dict_ == nullptr) {
            dict_ = rootDict_->AddSectionDictionary("SERIALIZATION");
        }

        auto serializationDict = tpl::addSectionedInclude(
            dict_, "SERIAL", "struct_serialization.tpl");

        serializationDict->SetValue(
            "RUNTIME_NAMESPACE_PREFIX",
            "::" + scope.idl->env->runtimeFramework()->cppNamespace.asPrefix("::"));

        serializationDict->SetValue("STRUCT_TYPE",
            typeNameMaker_->make(FullTypeRef(scope, s.name)));

        addStructSerialization(
            importMaker_, serializationDict, scope, s, isHeader_);
    }

    return dict_;
}

} // namespace cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
