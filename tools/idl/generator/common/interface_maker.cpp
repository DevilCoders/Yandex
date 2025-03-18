#include "common/interface_maker.h"

#include "common/common.h"
#include "common/doc_maker.h"
#include "common/function_maker.h"
#include "common/import_maker.h"
#include "common/pod_decider.h"
#include "common/type_name_maker.h"
#include "cpp/type_name_maker.h"
#include "tpl/tpl.h"

#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/nodes/nodes.h>
#include <yandex/maps/idl/targets.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace common {

InterfaceMaker::InterfaceMaker(
    const PodDecider* podDecider,
    const TypeNameMaker* typeNameMaker,
    ImportMaker* importMaker,
    const DocMaker* docMaker,
    FunctionMaker* functionMaker,
    bool isHeader,
    bool generateBaseMethods)
    : podDecider_(podDecider),
      typeNameMaker_(typeNameMaker),
      importMaker_(importMaker),
      docMaker_(docMaker),
      functionMaker_(functionMaker),
      isHeader_(isHeader),
      generateBaseMethods_(generateBaseMethods)
{
}

InterfaceMaker::~InterfaceMaker()
{
}

ctemplate::TemplateDictionary* InterfaceMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    FullScope scope,
    const nodes::Interface& i)
{
    FullTypeRef typeRef(scope, i.name);
    importMaker_->addDefinedType(typeRef);

    auto dict = createDict(parentDict, scope, i);

    dict->ShowSection(i.isStatic ? "IS_STATIC" : "IS_NOT_STATIC");
    if (isExcludeDoc(i.doc)) {
        dict->ShowSection("EXCLUDE");
    }

    dict->ShowSection(i.ownership == nodes::Interface::Ownership::Strong ?
        "STRONG_INTERFACE" : "NOT_STRONG_INTERFACE");
    dict->ShowSection(i.ownership == nodes::Interface::Ownership::Shared ?
        "SHARED_INTERFACE" : "NOT_SHARED_INTERFACE");
    dict->ShowSection(i.ownership == nodes::Interface::Ownership::Weak ?
        "WEAK_INTERFACE" : "NOT_WEAK_INTERFACE");

    if (docMaker_) {
        docMaker_->make(dict, "DOCS", scope, i.doc);
    }

    dict->ShowSection(scope.scope.isEmpty() ? "TOP_LEVEL" : "INNER_LEVEL");

    dict->SetValue("CONSTRUCTOR_NAME",
        typeNameMaker_->makeConstructorName(typeRef.info()));
    dict->SetValue("INSTANCE_NAME",
        typeNameMaker_->makeInstanceName(typeRef.info()));
    dict->SetValue("TYPE_NAME", typeNameMaker_->make(typeRef));
    dict->SetValue("SCOPE_PREFIX", scope.scope.asPrefix("::"));

    dict->SetValue("CPP_TYPE_NAME", cpp::fullName(typeRef));

    if (i.base) {
        FullTypeRef baseTypeRef(scope, *i.base);
        importMaker_->addAll(baseTypeRef, false);

        dict->ShowSection("HAS_PARENT");
        dict->ShowSection("HAS_PARENT_INTERFACE");

        auto baseInterfaceDict = dict->AddSectionDictionary("PARENT");
        if (baseTypeRef.isVirtualInterface()) {
            baseInterfaceDict->ShowSection("VIRTUAL");
        }
        baseInterfaceDict->SetValue("NAME", typeNameMaker_->make(baseTypeRef));
    } else {
        dict->ShowSection("NO_PARENT");
    }

    FullTypeRef topMostBaseTypeRef(*topMostBaseTypeInfo(scope, i));
    dict->SetValue("TOP_MOST_BASE_TYPE_NAME",
        typeNameMaker_->make(topMostBaseTypeRef));

    lambdaTraverseWithScope(scope.scope, i,
        [&] (const nodes::Function& f) { makeFunction(dict, scope, i, f); },
        [&] (const nodes::Property& p) { makeProperty(dict, scope, p); });

    if (generateBaseMethods_) {
        // Iterate over methods and properties of all base interfaces
        auto iPtr = &i;
        auto typeInfoPtr = &scope.type(i.name.original());
        while (iPtr->base) {
            typeInfoPtr = &typeInfoPtr->idl->type(
                typeInfoPtr->scope.original(), *iPtr->base);
            iPtr = std::get<const nodes::Interface*>(typeInfoPtr->type);

            FullScope scope{ typeInfoPtr->idl, typeInfoPtr->scope.original() };

            lambdaTraverseWithScope(scope.scope, *iPtr,
                [&] (const nodes::Function& f)
                {
                    makeFunction(dict, scope, *iPtr, f, true);
                },
                [&] (const nodes::Property& p)
                {
                    makeProperty(dict, scope, p, true);
                });
        }
    }

    return dict;
}

ctemplate::TemplateDictionary* InterfaceMaker::createDict(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& /*scope*/,
    const nodes::Interface& i) const
{
    return tpl::addSectionedInclude(parentDict, "CHILD",
        i.isStatic ? "static_interface.tpl" : "interface.tpl");
}

ctemplate::TemplateDictionary* InterfaceMaker::makeFunction(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Interface& i,
    const nodes::Function& f,
    bool isBaseItem)
{
    for (const auto& parameter : f.parameters) {
        subscribedListenerTypes_.addIfListener({ scope, parameter.typeRef });
    }

    auto itemDict = parentDict->AddSectionDictionary("ITEM");
    itemDict->ShowSection(isBaseItem ? "BASE_ITEM" : "SELF_ITEM");

    auto dict = functionMaker_->make(itemDict, scope, f, false);

    if (docMaker_) {
        docMaker_->make(dict, "DOCS", scope, f);
    }

    dict->ShowSection(i.isStatic ? "IS_STATIC" : "IS_NOT_STATIC");

    if (f.isConst) {
        dict->ShowSection("CONST_METHOD");
    }

    return dict;
}

ctemplate::TemplateDictionary* InterfaceMaker::makeProperty(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Property& p,
    bool isBaseItem)
{
    FullTypeRef typeRef(scope, p.typeRef);
    subscribedListenerTypes_.addIfListener(typeRef);

    importMaker_->addAll(typeRef, false, isHeader_);

    auto itemDict = parentDict->AddSectionDictionary("ITEM");
    itemDict->ShowSection(isBaseItem ? "BASE_ITEM" : "SELF_ITEM");

    auto dict = itemDict->AddSectionDictionary("PROPERTY");

    if (isExcludeDoc(p.doc)) {
        dict->ShowSection("PROPERTY_EXCLUDE");
    }

    if (docMaker_) {
        docMaker_->make(dict, "PROPERTY_DOCS", scope, p);
    }

    dict->SetValue("PROPERTY_NAME", p.name);

    dict->SetValue("PROPERTY_TYPE", typeNameMaker_->makeRef(typeRef));
    dict->ShowSection(
        p.typeRef.id == nodes::TypeId::Bool ? "BOOL" : "NOT_BOOL");

    dict->ShowSection(p.isReadonly ? "READONLY" : "NOT_READONLY");

    dict->ShowSection(podDecider_->isPod(typeRef) ? "POD" : "NOT_POD");
    dict->ShowSection(p.isGenerated ? "GEN" : "NOT_GEN");

    bool isInterface = typeRef.is<nodes::Interface>();
    if (isInterface) {
        if (!typeRef.isConst()) {
            dict->ShowSection("NOT_CONST");
        }
    }
    dict->ShowSection(isInterface ? "INTERFACE" : "NOT_INTERFACE");

    dict->ShowSection(typeRef.is<nodes::Listener>() ? "LISTENER" : "NOT_LISTENER");

    return dict;
}

const TypeInfo* topMostBaseTypeInfo(
    const FullScope& scope,
    const nodes::Interface& i)
{
    return topMostBaseTypeInfo(&scope.type(i.name.original()));
}

const TypeInfo* topMostBaseTypeInfo(const TypeInfo* typeInfo)
{
    auto i = std::get<const nodes::Interface*>(typeInfo->type);

    while (i->base) {
        typeInfo = &typeInfo->idl->type(typeInfo->scope.original(), *i->base);
        i = std::get<const nodes::Interface*>(typeInfo->type);
    }

    return typeInfo;
}

void SubscribedListenerTypes::clear()
{
    listenerTypes_.clear();
}

void SubscribedListenerTypes::addIfListener(const FullTypeRef& typeRef)
{
    if (typeRef.isWeakListener()) {
        listenerTypes_.insert(typeRef.info());
    }
}

const std::set<TypeInfo>& SubscribedListenerTypes::listenerTypes() const
{
    return listenerTypes_;
}

} // namespace common
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
