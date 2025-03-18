#include "common/variant_maker.h"

#include "common/common.h"
#include "common/doc_maker.h"
#include "common/import_maker.h"
#include "common/pod_decider.h"
#include "common/type_name_maker.h"
#include "cpp/type_name_maker.h"
#include "tpl/tpl.h"

#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/targets.h>
#include <yandex/maps/idl/utils/common.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace common {

VariantMaker::VariantMaker(
    const PodDecider* podDecider,
    const TypeNameMaker* typeNameMaker,
    ImportMaker* importMaker,
    const DocMaker* docMaker)
    : podDecider_(podDecider),
      typeNameMaker_(typeNameMaker),
      importMaker_(importMaker),
      docMaker_(docMaker)
{
}

VariantMaker::~VariantMaker()
{
}

ctemplate::TemplateDictionary* VariantMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    FullScope scope,
    const nodes::Variant& v)
{
    auto dict = createDict(parentDict, scope);

    if (docMaker_) {
        docMaker_->make(dict, "DOCS", scope, v.doc);
    }

    if (scope.scope.isEmpty()) {
        dict->ShowSection("TOP_LEVEL");
    } else {
        dict->ShowSection("INNER_LEVEL");
    }

    FullTypeRef typeRef(scope, v.name);
    dict->SetValue("CONSTRUCTOR_NAME",
        typeNameMaker_->makeConstructorName(typeRef.info()));
    dict->SetValue("INSTANCE_NAME",
        typeNameMaker_->makeInstanceName(typeRef.info()));
    dict->SetValue("TYPE_NAME", typeNameMaker_->make(typeRef));
    dict->SetValue("SCOPE_PREFIX", scope.scope.asPrefix("::"));

    dict->SetValue("CPP_TYPE_NAME", cpp::fullName(typeRef));

    ScopeGuard guard(&scope.scope, v.name.original());
    int fieldIndex = 0;
    for (const auto& f : v.fields) {
        makeField(dict, scope, v, f, fieldIndex);
        ++fieldIndex;
    }

    return dict;
}

ctemplate::TemplateDictionary* VariantMaker::createDict(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& /*scope*/) const
{
    return tpl::addSectionedInclude(parentDict, "CHILD", "variant.tpl");
}

ctemplate::TemplateDictionary* VariantMaker::makeField(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Variant& v,
    const nodes::VariantField& f,
    int fieldIndex) const
{
    FullTypeRef typeRef(scope, f.typeRef);

    importMaker_->addAll(typeRef, false);

    auto dict = parentDict->AddSectionDictionary("TYPES");
    dict->SetValue("CLASS_TYPE", fieldName(scope, v, f.typeRef));
    dict->SetValue("OPTIONAL_CLASS_TYPE",
        fieldName(scope, v, f.typeRef, true));

    dict->SetValue("FIELD_NAME", f.name);
    dict->SetIntValue("FIELD_INDEX", fieldIndex);

    dict->ShowSection(podDecider_->isPod(typeRef) ? "POD" : "NOT_POD");

    addNullAsserts(dict, typeRef, f.name);

    return dict;
}

std::string VariantMaker::fieldName(
    const FullScope& scope,
    const nodes::Variant& /* v */,
    const nodes::TypeRef& fieldTypeRef,
    bool isOptional) const
{
    return typeNameMaker_->makeRef(
        FullTypeRef(scope, fieldTypeRef).asOptional(isOptional));
}

} // namespace common
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
