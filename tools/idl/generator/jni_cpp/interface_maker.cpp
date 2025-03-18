#include "jni_cpp/interface_maker.h"

#include "common/common.h"
#include "common/generator.h"
#include "common/type_name_maker.h"
#include "cpp/common.h"
#include "cpp/import_maker.h"
#include "cpp/type_name_maker.h"
#include "jni_cpp/jni.h"

#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/nodes/nodes.h>
#include <yandex/maps/idl/utils.h>
#include <yandex/maps/idl/targets.h>

#include <boost/algorithm/string.hpp>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace jni_cpp {

InterfaceMaker::InterfaceMaker(
    const common::PodDecider* podDecider,
    const common::TypeNameMaker* typeNameMaker,
    common::ImportMaker* importMaker,
    common::FunctionMaker* functionMaker,
    bool isHeader,
    ctemplate::TemplateDictionary* rootDict)
    : cpp::InterfaceMaker(
          podDecider, typeNameMaker, importMaker, nullptr, functionMaker, isHeader),
      rootDict_(rootDict),
      generatedStrongRefListenerProperty_(false)
{
}

ctemplate::TemplateDictionary* InterfaceMaker::make(
    ctemplate::TemplateDictionary* /* parentDict */,
    FullScope scope,
    const nodes::Interface& i)
{
    if (!needGenerateNode(scope, i))
        return nullptr;

    rootDict_->ShowSection("INTERFACES");

    auto dict = cpp::InterfaceMaker::make(rootDict_, scope, i);

    const auto& typeInfo = scope.type(i.name.original());
    auto interfaceBindingJniTypeRef = jniTypeRef(typeInfo, true, true);
    dict->SetValue("INTERFACE_BINDING_TYPE_REF", interfaceBindingJniTypeRef);
    dict->SetValue("INTERFACE_BINDING_NAME_IN_FUNCTION_NAME",
        mangleJniTypeRefs(interfaceBindingJniTypeRef));

    auto interfaceJniTypeRef = jniTypeRef(typeInfo, true, false);
    dict->SetValue("INTERFACE_NAME_IN_FUNCTION_NAME",
        mangleJniTypeRefs(interfaceJniTypeRef));

    for (const auto& l : subscribedListenerTypes_.listenerTypes()) {
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
            cpp::listenerBindingTypeName("android", l));

        subscriptionDict->SetValue(
            "LISTENER_NAME", typeNameMaker_->makeInstanceName(l));
    }

    return dict;
}

ctemplate::TemplateDictionary* InterfaceMaker::createDict(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& /* scope */,
    const nodes::Interface& /* i */) const
{
    return parentDict->AddSectionDictionary("INTERFACE");
}

ctemplate::TemplateDictionary* InterfaceMaker::makeFunction(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Interface& i,
    const nodes::Function& f,
    bool isBaseItem)
{
    auto dict = cpp::InterfaceMaker::makeFunction(
        parentDict, scope, i, f, isBaseItem);
    dict->SetValue("JNI_PARAM_TYPE_REFS_IN_FUNCTION_NAME",
        jniMethodNameParametersPart(scope, f.parameters));
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

    auto dict = cpp::InterfaceMaker::makeProperty(parentDict, scope, p, isBaseItem);

    FullTypeRef typeRef(scope, p.typeRef);
    common::addNullAsserts(dict, typeRef, p.name);

    dict->SetValue("JNI_PROPERTY_TYPE_NAME", jniTypeName(typeRef, false));
    dict->SetValue("JNI_PROPERTY_TYPE_REF_IN_SETTER_NAME",
        mangleJniTypeRefs(jniTypeRef(typeRef, false)));

    if (p.typeRef.id == nodes::TypeId::Custom) {
        const auto& typeInfo = typeRef.info();

        const auto l = typeRef.as<nodes::Listener>();
        if (l) {
            dict->SetValue("LISTENER_BINDING_TYPE_NAME",
                cpp::listenerBindingTypeName("android", typeInfo));

            if (l->isStrongRef) {
                generatedStrongRefListenerProperty_ = true;

                dict->AddSectionDictionary("STRONG_LISTENER_PROPERTY");
            } else {
                dict->AddSectionDictionary("WEAK_LISTENER_PROPERTY");
            }
        }
    }

    return dict;
}

} // namespace jni_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
