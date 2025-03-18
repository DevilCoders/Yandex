#include "common.h"

#include <yandex/maps/idl/targets.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace common {

void addNullAsserts(
    ctemplate::TemplateDictionary* dict,
    const FullTypeRef& typeRef,
    const std::string& name)
{
    if (typeRef.isOptional() || !typeRef.canBeRequired()) {
        return; // Optional or always optional - do not assert!
    }

    if (typeRef.isCppNullable()) {
        dict->SetValueAndShowSection(
            "FIELD_NAME", name, "CPP_NULL_ASSERT");
    }
    if (typeRef.isCsNullable()) {
        dict->SetValueAndShowSection(
            "FIELD_NAME", name, "CS_NULL_ASSERT");
    }
    if (typeRef.isJavaNullable()) {
        dict->SetValueAndShowSection(
            "FIELD_NAME", name, "JAVA_NULL_ASSERT");
    }
    if (typeRef.isObjCNullable()) {
        dict->SetValueAndShowSection(
            "FIELD_NAME", name, "OBJC_NULL_ASSERT");
    }
}

void setNamespace(
    const Scope& nameSpace,
    ctemplate::TemplateDictionary* dict)
{
    for (const auto& name : nameSpace) {
        dict->SetValueAndShowSection("NAME", name, "NAMESPACE");
    }

    for (auto it = nameSpace.rbegin(); it != nameSpace.rend(); ++it) {
        dict->SetValueAndShowSection("NAME", *it, "CLOSING_NAMESPACE");
    }
}

void setNamespace(
    const std::string& targetLang,
    ctemplate::TemplateDictionary* dict,
    const Idl* idl)
{
    if (targetLang == "") {
        setNamespace(idl->idlNamespace, dict);
    } else if (targetLang == CS) {
        setNamespace(idl->csNamespace, dict);
    } else if (targetLang == JAVA) {
        setNamespace(idl->javaPackage, dict);
    } else if (targetLang == OBJC) {
        setNamespace(Scope(idl->objcFramework), dict);
    }
}

} // namespace common
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
