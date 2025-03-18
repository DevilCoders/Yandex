#include "jni_cpp/listener_maker.h"

#include "common/common.h"
#include "cpp/type_name_maker.h"

#include <yandex/maps/idl/utils.h>
#include "jni_cpp/jni.h"

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace jni_cpp {

ListenerMaker::ListenerMaker(
    const common::TypeNameMaker* typeNameMaker,
    common::ImportMaker* importMaker,
    common::FunctionMaker* functionMaker,
    ctemplate::TemplateDictionary* rootDict)
    : cpp::ListenerMaker(typeNameMaker, importMaker, nullptr, functionMaker),
      rootDict_(rootDict)
{
}

ctemplate::TemplateDictionary* ListenerMaker::make(
    ctemplate::TemplateDictionary* /* parentDict */,
    FullScope scope,
    const nodes::Listener& l)
{
    if (!needGenerateNode(scope, l))
        return nullptr;

    importMaker_->addCppRuntimeImportPath("verify_and_run.h");

    auto dict = cpp::ListenerMaker::make(rootDict_, scope, l);

    const auto& typeInfo = scope.type(l.name.original());

    if (l.isLambda) {
        dict->SetValue("SCOPE_PREFIX", scope.scope.asPrefix("::"));
    } else {
        // Listener binding stores its Java full name as std::string:
        importMaker_->addImportPath("<string>");

        dict->SetValue("LISTENER_BINDING_TYPE_NAME",
            cpp::listenerBindingTypeName("android", typeInfo));
    }

    auto listenerJniTypeRef = jniTypeRef(typeInfo, true, false);
    dict->SetValue("LISTENER_BINDING_TYPE_REF", listenerJniTypeRef);

    return dict;
}

ctemplate::TemplateDictionary* ListenerMaker::createDict(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Listener& l) const
{
    auto dict = parentDict->AddSectionDictionary(
        l.isLambda ? "LAMBDA_LISTENER" : "CLASSIC_LISTENER");

    common::setNamespace(
        scope.idl->env->runtimeFramework()->cppNamespace,
        dict->AddSectionDictionary("RUNTIME_NAMESPACE"));

    return dict;
}

} // namespace jni_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
