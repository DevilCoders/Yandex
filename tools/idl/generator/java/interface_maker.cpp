#include "java/interface_maker.h"

#include "common/common.h"
#include "common/generator.h"
#include "java/annotation_addition.h"
#include "java/import_maker.h"

#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/targets.h>
#include <yandex/maps/idl/utils.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace java {

InterfaceMaker::InterfaceMaker(
    const common::PodDecider* podDecider,
    const common::TypeNameMaker* typeNameMaker,
    common::ImportMaker* importMaker,
    const common::DocMaker* docMaker,
    common::FunctionMaker* functionMaker,
    bool isBinding)
    : common::InterfaceMaker(podDecider, typeNameMaker, importMaker,
          docMaker, functionMaker, false),
      isBinding_(isBinding)
{
}

ctemplate::TemplateDictionary* InterfaceMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    FullScope scope,
    const nodes::Interface& i)
{
    auto dict = common::InterfaceMaker::make(parentDict, scope, i);

    dict->ShowSection(isBinding_ ? "IS_BINDING" : "IS_NOT_BINDING");

    if (isBinding_) {
        importMaker_->addJavaRuntimeImportPath("NativeObject");

        const auto& typeInfo = scope.type(i.name.original());
        importMaker_->addImportPath(
            typeInfo.fullNameAsScope[JAVA].asString("."));

        const auto& listenerTypes = subscribedListenerTypes_.listenerTypes();
        if (!listenerTypes.empty()) {
            bool needSubscriptionImport = false;

            for (const auto& listenerTypeInfo : listenerTypes) {
                if (scope.idl->env->config.isPublic) {
                    common::HasPublicUsagesVisitor visitor(listenerTypeInfo);
                    i.nodes.traverse(&visitor);
                    if (!visitor.isUsed())
                        continue;
                }

                FullTypeRef typeRef(listenerTypeInfo);
                dict->AddSectionDictionary("SUBSCRIPTION")->SetValue(
                    "LISTENER_NAME", typeNameMaker_->make(typeRef));
                needSubscriptionImport = true;
            }

            if (needSubscriptionImport)
                importMaker_->addJavaRuntimeImportPath("subscription.Subscription");
        }

        if (i.base) {
            const auto& baseTypeInfo = scope.type(*i.base);
            auto internalName = baseTypeInfo.idl->javaPackage +
                baseTypeInfo.scope[JAVA] +
                ("internal." + baseTypeInfo.name[JAVA] + "Binding");
            importMaker_->addImportPath(internalName.asString("."));
        }
    }

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

    auto dict = common::InterfaceMaker::makeProperty(parentDict, scope, p, isBaseItem);
    FullTypeRef typeRef(scope, p.typeRef);
    addNullabilityAnnotation(dict, importMaker_, typeRef);
    return dict;
}

} // namespace java
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
