#include "java/serialization_addition.h"

#include "tpl/tpl.h"

#include <yandex/maps/idl/utils.h>
#include <yandex/maps/idl/nodes/enum.h>
#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/nodes/variant.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace java {

namespace {

class CustomCaseHandler {
public:
    CustomCaseHandler(
        ctemplate::TemplateDictionary* paramsDict,
        const std::string& sectionName,
        const std::string& typeName)
        : paramsDict_(paramsDict)
        , sectionName_(sectionName)
        , typeName_(typeName)
    {
    }

    void operator()(const nodes::Enum* e) const
    {
        if (e->isBitField) {
            paramsDict_->SetValue("HANDLER_TYPE", "Integer");
            return;
        }

        paramsDict_->SetValue("HANDLER_TYPE", "Enum");
        paramsDict_->SetValueAndShowSection(
            "GENERIC_CLASS", typeName_, "GENERIC_HANDLER");
        paramsDict_->SetValueAndShowSection(
            "FIELD_CLASS", typeName_, sectionName_);
    }
    void operator()(const nodes::Struct*) const
    {
        paramsDict_->SetValue("HANDLER_TYPE", "Class");
        paramsDict_->SetValueAndShowSection(
            "GENERIC_CLASS", typeName_, "GENERIC_HANDLER");
        paramsDict_->SetValueAndShowSection(
            "FIELD_CLASS", typeName_, sectionName_);
    }
    void operator()(const nodes::Variant*) const
    {
        paramsDict_->SetValue("HANDLER_TYPE", "Class");
        paramsDict_->SetValueAndShowSection(
            "GENERIC_CLASS", typeName_, "GENERIC_HANDLER");
        paramsDict_->SetValueAndShowSection(
            "FIELD_CLASS", typeName_, sectionName_);
    }
    void operator()(const nodes::Interface*) const
    {
    }
    void operator()(const nodes::Listener*) const
    {
    }

private:
    ctemplate::TemplateDictionary* paramsDict_;
    const std::string sectionName_;

    const std::string typeName_;
};

void handleCustomCase(
    const common::TypeNameMaker* typeNameMaker,
    ctemplate::TemplateDictionary* paramsDict,
    const std::string& sectionName,
    const FullTypeRef& typeRef)
{
    auto typeName = typeNameMaker->make(typeRef);

    std::visit(
        CustomCaseHandler(paramsDict, sectionName, typeName),
        typeRef.info().type);
}

void handleContainerJavaField(
    const common::TypeNameMaker* typeNameMaker,
    ctemplate::TemplateDictionary* paramsDict,
    const FullTypeRef& typeRef)
{
    for (const auto& subRef : typeRef.subRefs()) {
        auto handlerDict = paramsDict->AddSectionDictionary("HANDLER");

        nodes::TypeId paramId = subRef.id();
        if (paramId == nodes::TypeId::Bool) {
            handlerDict->SetValue("HANDLER_TYPE", "Boolean");
        } else if (paramId == nodes::TypeId::Int ||
                paramId == nodes::TypeId::Uint ||
                paramId == nodes::TypeId::Color) {
            handlerDict->SetValue("HANDLER_TYPE", "Integer");
        } else if (paramId == nodes::TypeId::Int64) {
            handlerDict->SetValue("HANDLER_TYPE", "Long");
        } else if (paramId == nodes::TypeId::Float) {
            handlerDict->SetValue("HANDLER_TYPE", "Float");
        } else if (paramId == nodes::TypeId::Double) {
            handlerDict->SetValue("HANDLER_TYPE", "Double");
        } else if (paramId == nodes::TypeId::String) {
            handlerDict->SetValue("HANDLER_TYPE", "String");
        } else if (paramId == nodes::TypeId::Bytes) {
            handlerDict->SetValue("HANDLER_TYPE", "Bytes");
        } else if (paramId == nodes::TypeId::TimeInterval ||
                paramId == nodes::TypeId::AbsTimestamp ||
                paramId == nodes::TypeId::RelTimestamp) {
            handlerDict->SetValue("HANDLER_TYPE", "Long");
        } else if (paramId == nodes::TypeId::Point) {
            handlerDict->SetValue("HANDLER_TYPE", "Point");
        } else if (paramId == nodes::TypeId::Custom) {
            handleCustomCase(
                typeNameMaker, handlerDict, "CUSTOM_HANDLER", subRef);
        }
    }
}

} // namespace

void addStructSerialization(
    const common::TypeNameMaker* typeNameMaker,
    common::ImportMaker* importMaker,
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Struct& s)
{
    auto serializationDict = tpl::addSectionedInclude(
        parentDict, "SERIALIZATION", "struct_serialization.tpl");
    serializationDict->ShowSection(
        s.kind == nodes::StructKind::Bridged ? "BRIDGED" : "LITE");

    auto fieldScope = scope + s.name.original();
    s.nodes.lambdaTraverse(
        [&](const nodes::StructField& structField)
        {
            if (!needGenerateNode(scope, structField))
                return;

            auto dict = serializationDict->AddSectionDictionary("FIELD");
            dict->SetValue("FIELD_NAME", structField.name);

            if (structField.typeRef.isOptional) {
                dict->SetValueAndShowSection(
                    "OPTIONAL_VALUE", "true", "OPTIONAL");
            }

            addFieldSerialization(typeNameMaker, importMaker, dict,
                FullTypeRef(fieldScope, structField.typeRef));
        });
}

void addFieldSerialization(
    const common::TypeNameMaker* typeNameMaker,
    common::ImportMaker* importMaker,
    ctemplate::TemplateDictionary* paramsDict,
    const FullTypeRef& typeRef)
{
    importMaker->addAll(typeRef, true);

    nodes::TypeId typeId = typeRef.id();
    if (typeId == nodes::TypeId::Bool) {
        paramsDict->SetValueAndShowSection(
            "DEFAULT_VALUE", "false", "POD_PARAM");
        return;
    } else if (typeId == nodes::TypeId::Int ||
            typeId == nodes::TypeId::Uint ||
            typeId == nodes::TypeId::Int64 ||
            typeId == nodes::TypeId::Float ||
            typeId == nodes::TypeId::Double ||
            typeId == nodes::TypeId::TimeInterval ||
            typeId == nodes::TypeId::AbsTimestamp ||
            typeId == nodes::TypeId::RelTimestamp ||
            typeId == nodes::TypeId::Color) {
        paramsDict->SetValueAndShowSection("DEFAULT_VALUE", "0", "POD_PARAM");
        return;
    }

    paramsDict->ShowSection("NOT_POD_PARAM");

    if (!typeRef.isOptional()) {
        paramsDict->SetValueAndShowSection(
            "OPTIONAL_VALUE", "false", "OPTIONAL");
    }

    if (typeId == nodes::TypeId::Vector) {
        handleContainerJavaField(typeNameMaker, paramsDict, typeRef);
    } else if (typeId == nodes::TypeId::Dictionary){
        handleContainerJavaField(typeNameMaker, paramsDict, typeRef);
    } else if (typeId == nodes::TypeId::Custom) {
        handleCustomCase(typeNameMaker, paramsDict, "CUSTOM", typeRef);
    } else if (typeId == nodes::TypeId::AnyCollection) {
        paramsDict->SetValueAndShowSection(
            "FIELD_CLASS", "Collection", "CUSTOM");
    }
}

} // namespace java
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
