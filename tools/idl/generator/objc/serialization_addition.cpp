#include "objc/serialization_addition.h"

#include "tpl/tpl.h"
#include "objc/common.h"

#include <yandex/maps/idl/nodes/root.h>
#include <yandex/maps/idl/nodes/type_ref.h>
#include <yandex/maps/idl/nodes/variant.h>
#include <yandex/maps/idl/utils.h>

#include <boost/algorithm/string.hpp>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace objc {

namespace {

class VariantVisitor {
public:
    VariantVisitor(
        ctemplate::TemplateDictionary* dict,
        const std::string& typeName,
        bool isOptional)
        : dict_(dict),
          typeName_(typeName),
          isOptional_(isOptional)
    {
    }
    void operator()(const nodes::Struct*) const
    {
        dict_->SetValue("TYPE", "Object");
        dict_->SetValue("HANDLER_TYPE", "Class");
        dict_->SetValueAndShowSection(
            "FIELD_CLASS", typeName_, "CUSTOM");
        dict_->SetValueAndShowSection(
            "FIELD_CLASS", typeName_, "CUSTOM_HANDLER");
    }
    void operator()(const nodes::Variant*) const
    {
        dict_->SetValue("TYPE", "Object");
        dict_->SetValue("HANDLER_TYPE", "Class");
        dict_->SetValueAndShowSection(
            "FIELD_CLASS", typeName_, "CUSTOM");
        dict_->SetValueAndShowSection(
            "FIELD_CLASS", typeName_, "CUSTOM_HANDLER");
    }
    void operator()(const nodes::Interface*) const {}
    void operator()(const nodes::Listener*) const {}
    void operator()(const nodes::Enum*) const
    {
        dict_->SetValue("TYPE", "UnsignedInteger");
        dict_->SetValue("HANDLER_TYPE", "UnsignedInteger");
        if (!isOptional_) {
            dict_->SetValueAndShowSection(
                "ENUM_TYPE", typeName_, "ENUM_CAST");
        }
    }
private:
    ctemplate::TemplateDictionary* dict_;
    const std::string typeName_;
    const bool isOptional_;
};

void handleCustomCase(
    const common::TypeNameMaker* typeNameMaker,
    ctemplate::TemplateDictionary* handlerDict,
    const FullTypeRef& typeRef)
{
    auto typeName = typeNameMaker->make(typeRef);

    std::visit(
        VariantVisitor(handlerDict, typeName, typeRef.isOptional()),
        typeRef.info().type);
}

void handleContainerField(
    const common::TypeNameMaker* typeNameMaker,
    ctemplate::TemplateDictionary* paramsDict,
    const FullTypeRef& typeRef)
{
    int counter = 0;
    for (const auto& subRef : typeRef.subRefs()) {
        auto handlerDict = paramsDict->AddSectionDictionary("ARRAY");

        if (typeRef.subRefs().size() == 1) {
            handlerDict->SetValue("HANDLER", "handler");
        } else if (counter == 0) {
            handlerDict->SetValue("HANDLER", "keyHandler");
        } else if (counter == 1) {
            handlerDict->SetValue("HANDLER", "valueHandler");
        }
        ++counter;

        nodes::TypeId id = subRef.id();
        if (id == nodes::TypeId::Bool) {
            handlerDict->SetValue("HANDLER_TYPE", "Boolean");
        } else if (id == nodes::TypeId::Int) {
            handlerDict->SetValue("HANDLER_TYPE", "Integer");
        } else if (id == nodes::TypeId::Uint) {
            handlerDict->SetValue("HANDLER_TYPE", "UnsignedInteger");
        } else if (id == nodes::TypeId::Int64) {
            handlerDict->SetValue("HANDLER_TYPE", "LongLong");
        } else if (id == nodes::TypeId::Float) {
            handlerDict->SetValue("HANDLER_TYPE", "Float");
        } else if (id == nodes::TypeId::Double) {
            handlerDict->SetValue("HANDLER_TYPE", "Double");
        } else if (id == nodes::TypeId::String) {
            handlerDict->SetValue("HANDLER_TYPE", "String");
        } else if (id == nodes::TypeId::Bytes) {
            handlerDict->SetValue("HANDLER_TYPE", "Bytes");
        } else if (id == nodes::TypeId::Color) {
            handlerDict->SetValue("HANDLER_TYPE", "Color");
        } else if (id == nodes::TypeId::TimeInterval) {
            handlerDict->SetValue("HANDLER_TYPE", "TimeInterval");
        } else if (id == nodes::TypeId::AbsTimestamp ||
                id == nodes::TypeId::RelTimestamp) {
            handlerDict->SetValue("HANDLER_TYPE", "Date");
        } else if (id == nodes::TypeId::Point) {
            handlerDict->SetValue("HANDLER_TYPE", "Point");
        } else if (id == nodes::TypeId::Custom) {
            handleCustomCase(typeNameMaker, handlerDict, subRef);
        }
    }
}

} // namespace

void addStructSerialization(
    const common::PodDecider* podDecider,
    const common::TypeNameMaker* typeNameMaker,
    common::ImportMaker* importMaker,
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Struct& s)
{
    auto serializationDict = tpl::addSectionedInclude(
        parentDict, "SERIALIZATION", "struct_serialization.tpl");

    setRuntimeFrameworkPrefix(scope.idl->env, serializationDict);

    if (s.kind == nodes::StructKind::Bridged) {
        serializationDict->ShowSection("BRIDGED");
    } else {
        serializationDict->ShowSection("LITE");
    }

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
                    "OPTIONAL_VALUE", "YES", "OPTIONAL");
            }

            addFieldSerialization(podDecider, typeNameMaker, importMaker,
                dict, FullTypeRef(fieldScope, structField.typeRef));
        });
}

void addFieldSerialization(
    const common::PodDecider* podDecider,
    const common::TypeNameMaker* typeNameMaker,
    common::ImportMaker* importMaker,
    ctemplate::TemplateDictionary* paramsDict,
    const FullTypeRef& typeRef)
{
    importMaker->addAll(typeRef, true);

    nodes::TypeId typeId = typeRef.id();
    if (typeId == nodes::TypeId::Bool) {
        paramsDict->SetValue("TYPE", "Bool");
        return;
    } else if (typeId == nodes::TypeId::Int) {
        paramsDict->SetValue("TYPE", "Integer");
        return;
    } else if (typeId == nodes::TypeId::Uint) {
        paramsDict->SetValue("TYPE", "UnsignedInteger");
        return;
    } else if (typeId == nodes::TypeId::Int64) {
        paramsDict->SetValue("TYPE", "LongLong");
        return;
    } else if (typeId == nodes::TypeId::Float) {
        paramsDict->SetValue("TYPE", "Float");
        return;
    } else if (typeId == nodes::TypeId::Double) {
        paramsDict->SetValue("TYPE", "Double");
        return;
    } else if (typeId == nodes::TypeId::String) {
        paramsDict->SetValue("TYPE", "String");
    } else if (typeId == nodes::TypeId::TimeInterval) {
        paramsDict->SetValue("TYPE", "TimeInterval");
        return;
    } else if (typeId == nodes::TypeId::AbsTimestamp ||
            typeId == nodes::TypeId::RelTimestamp) {
        paramsDict->SetValue("TYPE", "Date");
    } else if (typeId == nodes::TypeId::Bytes) {
        paramsDict->SetValue("TYPE", "Data");
    } else if (typeId == nodes::TypeId::Color) {
        paramsDict->SetValue("TYPE", "Color");
    } else if (typeId == nodes::TypeId::Point) {
        paramsDict->SetValue("TYPE", "CGPoint");
    } else if (typeId == nodes::TypeId::Vector) {
        paramsDict->SetValue("TYPE", "Array");
        handleContainerField(typeNameMaker, paramsDict, typeRef);
    } else if (typeId == nodes::TypeId::Dictionary){
        paramsDict->SetValue("TYPE", "Dictionary");
        handleContainerField(typeNameMaker, paramsDict, typeRef);
    } else if (typeId == nodes::TypeId::Custom) {
        handleCustomCase(typeNameMaker, paramsDict, typeRef.asOptional());
    } else if (typeId == nodes::TypeId::AnyCollection) {
        paramsDict->SetValue("TYPE", "Object");
        paramsDict->SetValue("HANDLER_TYPE", "Class");
        paramsDict->SetValueAndShowSection("FIELD_CLASS",
            runtimeFrameworkPrefix(typeRef.idl()->env) + "Collection", "CUSTOM");
    }

    if (!typeRef.isOptional() && !podDecider->isPod(typeRef)) {
        paramsDict->SetValueAndShowSection(
            "OPTIONAL_VALUE", "NO", "OPTIONAL");
    }
}

} // namespace objc
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
