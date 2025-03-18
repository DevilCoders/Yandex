#include "common/struct_maker.h"

#include "common/common.h"
#include "common/doc_maker.h"
#include "common/enum_field_maker.h"
#include "common/import_maker.h"
#include "common/pod_decider.h"
#include "common/type_name_maker.h"
#include "cpp/common.h"
#include "cpp/type_name_maker.h"
#include "java/annotation_addition.h"
#include "objc/common.h"
#include "tpl/tpl.h"

#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/functions.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/nodes.h>
#include <yandex/maps/idl/targets.h>

#include <boost/regex.hpp>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace common {

StructMaker::StructMaker(
    const PodDecider* podDecider,
    const TypeNameMaker* typeNameMaker,
    const EnumFieldMaker* enumFieldMaker,
    ImportMaker* importMaker,
    const DocMaker* docMaker)
    : podDecider_(podDecider),
      typeNameMaker_(typeNameMaker),
      enumFieldMaker_(enumFieldMaker),
      importMaker_(importMaker),
      docMaker_(docMaker)
{
}

StructMaker::~StructMaker()
{
}

ctemplate::TemplateDictionary* StructMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    FullScope scope,
    const nodes::Struct& s)
{
    auto dict = createDict(parentDict, scope, s);
    if (isExcludeDoc(s.doc)) {
        dict->ShowSection("EXCLUDE");
    }

    if (docMaker_) {
        docMaker_->make(dict, "DOCS", scope, s.doc);
    }
    dict->ShowSection(scope.scope.isEmpty() ? "TOP_LEVEL" : "INNER_LEVEL");

    if (s.kind == nodes::StructKind::Bridged) {
        dict->ShowSection("IS_NOT_OPTIONS");
        dict->ShowSection("BRIDGED");
    } else if (s.kind == nodes::StructKind::Options) {
        dict->ShowSection("IS_OPTIONS");
        dict->ShowSection("LITE");
    } else {
        dict->ShowSection("IS_NOT_OPTIONS");
        dict->ShowSection("LITE");
    }

    FullTypeRef typeRef(scope, s.name);
    dict->SetValue("CONSTRUCTOR_NAME",
        typeNameMaker_->makeConstructorName(typeRef.info()));
    dict->SetValue("INSTANCE_NAME",
        typeNameMaker_->makeInstanceName(typeRef.info()));
    dict->SetValue("TYPE_NAME", typeNameMaker_->make(typeRef));
    dict->SetValue("SCOPE_PREFIX", scope.scope.asPrefix("::"));

    addConstructor(dict, scope, s);

    dict->SetValue("NATIVE_NAME", cpp::nativeName(typeRef.info()));
    dict->SetValue("CPP_TYPE_NAME", cpp::fullName(typeRef));

    lambdaTraverseWithScope(scope.scope, s,
        [&] (const nodes::StructField& f) {
            if (f.defaultValue) {
                dict->ShowSection("HAVE_DEFAULT_VALUE");
            }
            makeField(dict, scope, s, f);
        });

    return dict;
}

ctemplate::TemplateDictionary* StructMaker::createDict(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& /*scope*/,
    const nodes::Struct& s) const
{
    auto tplName = s.kind == nodes::StructKind::Options ?
        "options_struct.tpl" : "struct.tpl";
    return tpl::addSectionedInclude(parentDict, "CHILD", tplName);
}

ctemplate::TemplateDictionary* StructMaker::makeField(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Struct& s,
    const nodes::StructField& f)
{
    importMaker_->addAll(FullTypeRef(scope, f.typeRef), false);

    auto fieldDict = parentDict->AddSectionDictionary("FIELD");
    fillFieldDict(fieldDict, scope, s, f);

    return fieldDict;
}

std::string StructMaker::fieldValue(
    const FullScope& scope,
    const nodes::StructField& f) const
{
    std::string defaultValue = *f.defaultValue;

    // If field is of enum type, its value is modified in a target-specific
    // way. Other fields' default values are simply copied.

    if (f.typeRef.id != nodes::TypeId::Custom) {
        return defaultValue;
    }

    FullTypeRef typeRef(scope, f.typeRef);
    if (typeRef.id() != nodes::TypeId::Custom) {
        return defaultValue;
    }

    auto e = typeRef.as<nodes::Enum>();
    if (!e) {
        return defaultValue;
    }

    std::set<std::string> enumFieldNames;
    for (const auto& field : e->fields) {
        enumFieldNames.insert(field.name);
    }

    return boost::regex_replace(defaultValue,
        boost::regex("(\\w+)\\.(\\w+)"),
        [&](boost::smatch match)
        {
            if (match[1] == e->name.original() &&
                    enumFieldNames.find(match[2]) != enumFieldNames.end()) {
                return enumFieldMaker_->makeValue(typeRef, match[2], false);
            } else {
                return std::string(match[0]);
            }
        }
    );
}

void StructMaker::fillFieldDict(
    ctemplate::TemplateDictionary* dict,
    const FullScope& scope,
    const nodes::Struct& /* s */,
    const nodes::StructField& f)
{
    if (docMaker_) {
        docMaker_->make(dict, "FIELD_DOCS", scope, f);
    }

    dict->SetValue("FIELD_NAME", f.name);

    FullTypeRef fieldTypeRef(scope, f.typeRef);
    dict->SetValue("FIELD_TYPE", typeNameMaker_->makeRef(fieldTypeRef));
    dict->ShowSection(podDecider_->isPod(fieldTypeRef) ? "POD" : "NOT_POD");

    if (f.defaultValue) {
        dict->SetValueAndShowSection(
            "VALUE", fieldValue(scope, f), "DEFAULT_VALUE");
    }

    if (fieldTypeRef.isBridged()) {
        dict->ShowSection("BRIDGED_FIELD");
    } else {
        dict->ShowSection("LITE_FIELD");
    }

    if (isExcludeDoc(f.doc)) {
        dict->ShowSection("EXCLUDE_FIELD");
    }

    addNullAsserts(dict, fieldTypeRef, f.name);
}

void StructMaker::addConstructor(
    ctemplate::TemplateDictionary* parentDict,
    FullScope scope,
    const nodes::Struct& s)
{
    const auto tplName = s.kind == nodes::StructKind::Bridged ?
        "bridged_constructor.tpl" : "lite_constructor.tpl";
    auto dict = tpl::addSectionedInclude(parentDict, "CTORS", tplName);

    FullTypeRef typeRef(scope, s.name);
    dict->SetValue("CONSTRUCTOR_NAME",
        typeNameMaker_->makeConstructorName(typeRef.info()));
    dict->SetValue("INSTANCE_NAME",
        typeNameMaker_->makeInstanceName(typeRef.info()));
    if (objc::supportsOptionalAnnotation(typeRef)) {
        dict->ShowSection(objc::hasNullableAnnotation(typeRef) ? "OPTIONAL_RESULT" : "NON_OPTIONAL_RESULT");
    }
    dict->SetValue("TYPE_NAME", typeNameMaker_->make(typeRef));

    dict->SetValue("SCOPE_PREFIX", scope.scope.asPrefix("::"));

    if (s.kind == nodes::StructKind::Options) {
        dict->ShowSection("OPTIONS");
    } else {
        dict->ShowSection("NOT_OPTIONS");
    }

    int fieldsNumber = 0;
    bool hasBridgedFields = false;

    ScopeGuard guard(&scope.scope, s.name.original());
    s.nodes.lambdaTraverse(
        [&](const nodes::StructField& f)
        {
            if (!addConstructorField(dict, scope, f, fieldsNumber))
                return;

            FullTypeRef fieldTypeRef(scope, f.typeRef);
            if (fieldTypeRef.isBridged())
                hasBridgedFields = true;

            ++fieldsNumber;
        });

    if (fieldsNumber == 1) {
        dict->ShowSection("EXPLICIT");
    }
    if (fieldsNumber >= 1) {
        dict->ShowSection("COLON");
    }

    dict->ShowSection(
        hasBridgedFields ? "HAS_BRIDGED_FIELDS" : "HAS_NO_BRIDGED_FIELDS");
}

bool StructMaker::addConstructorField(
    ctemplate::TemplateDictionary* parentDict,
    FullScope scope,
    const nodes::StructField& f,
    unsigned int fieldsNumber)
{
    auto paramDict = parentDict->AddSectionDictionary("PARAM");

    paramDict->SetValue("OBJC_METHOD_NAME_FIELD_PART",
        fieldsNumber == 0 ? utils::capitalizeWord(f.name) : f.name);

    FullTypeRef fieldTypeRef(scope, f.typeRef);
    paramDict->SetValue("TYPE", typeNameMaker_->makeRef(fieldTypeRef));
    if (objc::supportsOptionalAnnotation(fieldTypeRef)) {
        paramDict->ShowSection(objc::hasNullableAnnotation(fieldTypeRef) ? "OPTIONAL" : "NON_OPTIONAL");
    }
    java::addNullabilityAnnotation(paramDict, importMaker_, fieldTypeRef, false);
    if (podDecider_->isPod(fieldTypeRef)) {
        paramDict->ShowSection("POD");
    } else {
        paramDict->ShowSection("NOT_POD");
    }
    paramDict->SetValue("FIELD_NAME", f.name);

    if (fieldTypeRef.isBridged()) {
        paramDict->ShowSection("BRIDGED_FIELD");
    } else {
        paramDict->ShowSection("LITE_FIELD");
    }
    if (fieldTypeRef.isOptional()) {
        paramDict->ShowSection("OPTIONAL_FIELD");
        paramDict->ShowSection(scope.idl->env->config.useStdOptional ? "STD_OPTIONAL" : "BOOST_OPTIONAL");
    } else {
        paramDict->ShowSection("NONOPTIONAL_FIELD");
    }
    if (f.defaultValue) {
        parentDict->ShowSection("HAVE_DEFAULT_VALUE");
    }

    addNullAsserts(parentDict, fieldTypeRef, f.name);
    return true;
}

std::string StructMaker::fillHiddenFieldValue(
    const FullScope& scope,
    const nodes::StructField& f)
{
    if (f.defaultValue)
        return common::StructMaker::fieldValue(scope, f);

    FullTypeRef typeRef(scope, f.typeRef);
    const auto typeName = typeNameMaker_->make(typeRef);

    if (typeRef.id() == nodes::TypeId::Custom) {
        const auto& typeInfo = typeRef.info();
        const auto s = std::get_if<const nodes::Struct*>(&typeInfo.type);
        if (s)
            return "nullptr";
    }

    // Optional vector or dictionary not allowed
    if (typeRef.id() == nodes::TypeId::Vector || typeRef.id() == nodes::TypeId::Dictionary)
        return "std::make_shared<" + typeName + ">()";

    if (typeRef.isOptional())
        return "boost::none";

    REQUIRE(false, "Not allowed @internal field type");
}

ctemplate::TemplateDictionary* StructMaker::createDictForInternalStruct(
    ctemplate::TemplateDictionary* parentDict,
    FullScope scope,
    const nodes::Struct& s)
{
    if (s.kind != nodes::StructKind::Bridged)
        return parentDict;

    FullTypeRef typeRef(scope, s.name);
    const auto& typeInfo = typeRef.info();
    auto dict = createDict(parentDict, scope, s);
    dict->ShowSection("VISIBLE_PLATFORM_OBJECT");
    dict->SetValue("TYPE_NAME", typeNameMaker_->make(typeRef));
    importMaker_->addImportPath(cpp::filePath(typeInfo.idl, true).inAngles());
    return dict;
}

} // namespace common
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
