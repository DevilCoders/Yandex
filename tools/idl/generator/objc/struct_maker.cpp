#include "objc/struct_maker.h"

#include "objc/common.h"
#include "objc/serialization_addition.h"

#include <yandex/maps/idl/utils/exception.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/type_ref.h>
#include <yandex/maps/idl/utils.h>

#include <ostream>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace objc {

StructMaker::StructMaker(
    const common::PodDecider* podDecider,
    const common::TypeNameMaker* typeNameMaker,
    const common::EnumFieldMaker* enumFieldMaker,
    common::ImportMaker* importMaker,
    const common::DocMaker* docMaker,
    ctemplate::TemplateDictionary* rootDict)
    : common::StructMaker(
          podDecider,
          typeNameMaker,
          enumFieldMaker,
          importMaker,
          docMaker),
      rootDict_(rootDict)
{
}

ctemplate::TemplateDictionary* StructMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    FullScope scope,
    const nodes::Struct& s)
{
    if (!needGenerateNode(scope, s))
        return parentDict;

    if (scope.scope.size() > 0) {
        auto headerPath = filePath(scope.idl, true);
        importMaker_->addForwardDeclaration(
            FullTypeRef(scope, s.name), headerPath);
    }

    auto dict = common::StructMaker::make(rootDict_, scope, s);

    auto isInterface = [scope] (const nodes::StructField& f) {
        return FullTypeRef(scope, f.typeRef).is<nodes::Interface>();
    };

    if (s.kind != nodes::StructKind::Options || !s.nodes.count<nodes::StructField>(isInterface)) {
        addStructSerialization(podDecider_, typeNameMaker_, importMaker_, dict, scope, s);
    }

    FullTypeRef typeRef(scope, s.name);

    if (objc::supportsOptionalAnnotation(typeRef)) {
        dict->ShowSection(objc::hasNullableAnnotation(typeRef) ? "OPTIONAL_RESULT" : "NON_OPTIONAL_RESULT");
    }

    setRuntimeFrameworkPrefix(scope.idl->env, dict);

    return dict;
}

ctemplate::TemplateDictionary* StructMaker::makeField(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Struct& s,
    const nodes::StructField& f)
{
    if (!needGenerateNode(scope, f))
        return parentDict;
    return common::StructMaker::makeField(parentDict, scope, s, f);
}

namespace {

/**
 * Calculates property ownership type for given field. Only relevant for
 * "options" structs because other structs have only readonly properties.
 */
std::string propertyOwnershipType(
    const FullScope& scope,
    const nodes::StructField& f)
{
    const auto id = f.typeRef.id;

    if (id == nodes::TypeId::Bool || id == nodes::TypeId::Int ||
            id == nodes::TypeId::Uint || id == nodes::TypeId::Int64 ||
            id == nodes::TypeId::Float || id == nodes::TypeId::Double ||
            id == nodes::TypeId::TimeInterval ||
            id == nodes::TypeId::Point) {
        return f.typeRef.isOptional ? "copy" : "assign";
    }

    if (id == nodes::TypeId::String ||
            id == nodes::TypeId::AbsTimestamp ||
            id == nodes::TypeId::RelTimestamp) {
        return "copy";
    }

    if (id == nodes::TypeId::Bytes || id == nodes::TypeId::Color ||
            id == nodes::TypeId::Vector || id == nodes::TypeId::Dictionary ||
            id == nodes::TypeId::AnyCollection) {
        return "strong";
    }

    if (id == nodes::TypeId::Custom) {
        if (FullTypeRef(scope, f.typeRef).is<nodes::Enum>()) {
            return f.typeRef.isOptional ? "copy" : "assign";
        }

        return "strong";
    }

    INTERNAL_ERROR("Couldn't recognize Idl type id: " << id);
}

} // namespace

std::string StructMaker::fieldValue(
    const FullScope& scope,
    const nodes::StructField& f) const
{
    /** IDL timeinterval is different for ios & android. On android the timeinterval
     * determined as milliseconds, but on ios used seconds. Here we moving ios seconds
     * to milliseconds
     */

    const double MS_IN_SEC = 1000;

    if (f.typeRef.id == nodes::TypeId::TimeInterval) {
        auto fieldStringValue = *f.defaultValue;
        auto fieldFloatValue = atof(fieldStringValue.c_str());
        fieldStringValue = std::to_string(fieldFloatValue / MS_IN_SEC);

        return fieldStringValue;
    }

    return common::StructMaker::fieldValue(scope, f);
}

void StructMaker::fillFieldDict(
    ctemplate::TemplateDictionary* dict,
    const FullScope& scope,
    const nodes::Struct& s,
    const nodes::StructField& f)
{
    common::StructMaker::fillFieldDict(dict, scope, s, f);

    dict->SetValue(
        "PROPERTY_OWNERSHIP_TYPE", propertyOwnershipType(scope, f));

    FullTypeRef fieldTypeRef(scope, f.typeRef);
    if (supportsOptionalAnnotation(fieldTypeRef)) {
        dict->ShowSection(hasNullableAnnotation(fieldTypeRef) ? "OPTIONAL" : "NON_OPTIONAL");
    }
}


bool StructMaker::addConstructorField(
    ctemplate::TemplateDictionary* parentDict,
    FullScope scope,
    const nodes::StructField& f,
    unsigned int fieldsNumber)
{
    if (!needGenerateNode(scope, f))
        return false;
    return common::StructMaker::addConstructorField(parentDict, scope, f, fieldsNumber);
}

} // namespace objc
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
