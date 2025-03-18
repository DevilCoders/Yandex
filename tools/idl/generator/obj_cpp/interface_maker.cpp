#include "obj_cpp/interface_maker.h"

#include "common/common.h"
#include "common/generator.h"
#include "common/import_maker.h"
#include "cpp/common.h"
#include "cpp/import_maker.h"
#include "cpp/type_name_maker.h"
#include "obj_cpp/common.h"
#include "objc/common.h"
#include "objc/import_maker.h"
#include "objc/type_name_maker.h"
#include "tpl/tpl.h"

#include <yandex/maps/idl/framework.h>
#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/targets.h>
#include <yandex/maps/idl/utils.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace obj_cpp {

InterfaceMaker::InterfaceMaker(
    const common::PodDecider* podDecider,
    const common::TypeNameMaker* typeNameMaker,
    common::ImportMaker* importMaker,
    const common::DocMaker* docMaker,
    common::FunctionMaker* functionMaker,
    bool isHeader,
    ctemplate::TemplateDictionary* rootDict)
    : common::InterfaceMaker(
          podDecider,
          typeNameMaker,
          importMaker,
          docMaker,
          functionMaker,
          isHeader),
      rootDict_(rootDict),
      visited_(false),
      visitedNotStatic_(false),
      visitedStrongRefListenerProperty_(false)
{
}

ctemplate::TemplateDictionary* InterfaceMaker::make(
    ctemplate::TemplateDictionary* /* parentDict */,
    FullScope scope,
    const nodes::Interface& i)
{
    visited_ = true;
    if (!i.isStatic) {
        visitedNotStatic_ = true;
    }

    if (!needGenerateNode(scope, i))
        return nullptr;

    FullTypeRef typeRef(scope, i.name);
    const auto& typeInfo = typeRef.info();
    if (isHeader_) {
        importMaker_->addImportPath("<memory>");

        if (scope.scope.isEmpty()) {
            importMaker_->addImportPath(cpp::filePath(typeInfo, true).inAngles());
            importMaker_->addImportPath(objc::filePath(typeInfo.idl, true).inAngles());
        }
    } else {
        if (i.isStatic) {
            importMaker_->addImportPath(cpp::filePath(typeInfo, true).inAngles());
            importMaker_->addImportPath(objc::filePath(typeInfo.idl, true).inAngles());
        } else {
            importMaker_->addImportPath(obj_cpp::filePath(typeInfo.idl, true).inAngles());
        }

        if (i.ownership != nodes::Interface::Ownership::Strong) {
            auto topMostBaseTypeInfo = common::topMostBaseTypeInfo(scope, i);
            importMaker_->addImportPath(filePath(topMostBaseTypeInfo->idl, true).inAngles());
        }

        importMaker_->addCppRuntimeImportPath("ios/object.h");
    }

    auto dict = common::InterfaceMaker::make(rootDict_, scope, i);

    objc::setRuntimeFrameworkPrefix(scope.idl->env, dict);
    common::setNamespace(
        scope.idl->cppNamespace,
        dict->AddSectionDictionary("CPP_NAMESPACE"));

    dict->SetValue("OBJC_TYPE_NAME", objc::TypeNameMaker().make(typeRef));

    const auto& listenerTypes = subscribedListenerTypes_.listenerTypes();
    if (!listenerTypes.empty()) {
        dict->ShowSection("SUBSCRIPTIONS");
        importMaker_->addObjCRuntimeImportPath("Subscription.h");

        for (const auto& l : listenerTypes) {
            if (scope.idl->env->config.isPublic) {
                common::HasPublicUsagesVisitor visitor(l);
                i.nodes.traverse(&visitor);
                if (!visitor.isUsed())
                    continue;
            } 

            auto subscriptionDict = dict->AddSectionDictionary("SUBSCRIPTION");

            subscriptionDict->SetValue(
                "LISTENER_TYPE_NAME", cpp::fullName(l.fullNameAsScope[CPP]));

            subscriptionDict->SetValue("LISTENER_BINDING_TYPE_NAME",
                cpp::listenerBindingTypeName("ios", l));

            subscriptionDict->SetValue(
                "LISTENER_INSTANCE_NAME", typeNameMaker_->makeInstanceName(l));
        }
    }

    if (i.isStatic && i.name.isDefined(OBJC)) {
        dict->SetValueAndShowSection(
            "CATEGORY", i.name.original(), "IS_CATEGORY");
    }

    common::setNamespace(scope.idl->cppNamespace, dict);

    return dict;
}

ctemplate::TemplateDictionary* InterfaceMaker::createDict(
    ctemplate::TemplateDictionary* /* parentDict */,
    const FullScope& scope,
    const nodes::Interface& /* i */) const
{
    auto dict = tpl::addSectionedInclude(rootDict_, "TOPLEVEL_CHILD", "interface.tpl");

    dict->SetValue("RUNTIME_NAMESPACE_PREFIX",
        "::" + scope.idl->env->runtimeFramework()->cppNamespace.asPrefix("::"));

    return dict;
}

ctemplate::TemplateDictionary* InterfaceMaker::makeProperty(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Property& p,
    bool isBaseItem)
{
    if (!needGenerateNode(scope, p))
        return parentDict;

    auto dict = common::InterfaceMaker::makeProperty(
        parentDict, scope, p, isBaseItem);

    FullTypeRef typeRef(scope, p.typeRef);
    dict->SetValue("OBJC_PROPERTY_TYPE",
        objc::TypeNameMaker().makeRef(typeRef));
    common::addNullAsserts(dict, typeRef, p.name);

    bool isToPlatform = true;
    if (p.typeRef.id == nodes::TypeId::Custom) {
        if (typeRef.is<nodes::Enum>()) {
            if (!typeRef.isOptional()) {
                isToPlatform = false;
                dict->ShowSection("STATIC_CAST");
            }
        } else {
            const auto l = typeRef.as<nodes::Listener>();
            if (l) {
                isToPlatform = false;
                dict->SetValue("LISTENER_BINDING_TYPE_NAME",
                    cpp::listenerBindingTypeName("ios", typeRef.info()));

                if (l->isStrongRef) {
                    visitedStrongRefListenerProperty_ = true;

                    dict->AddSectionDictionary("STRONG_LISTENER_PROPERTY");
                } else {
                    dict->AddSectionDictionary("WEAK_LISTENER_PROPERTY");
                }
            }
        }
    }
    if (isToPlatform) {
        dict->ShowSection("TO_PLATFORM");
    }

    return dict;
}

} // namespace obj_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
