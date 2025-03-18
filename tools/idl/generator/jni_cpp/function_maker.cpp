#include "jni_cpp/function_maker.h"

#include "common/common.h"
#include "common/import_maker.h"
#include "cpp/common.h"
#include "cpp/import_maker.h"
#include "cpp/type_name_maker.h"
#include "jni_cpp/common.h"
#include "jni_cpp/jni.h"

#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/nodes/interface.h>
#include <yandex/maps/idl/nodes/listener.h>
#include <yandex/maps/idl/nodes/type_ref.h>
#include <yandex/maps/idl/targets.h>
#include <yandex/maps/idl/utils.h>

#include <variant>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace jni_cpp {

namespace {

// Parameter of classic listener type
const char* const LISTENER_SECTION = "LISTENER";

// Parameter that is lambda listener's method
const char* const LAMBDA_SECTION = "LAMBDA";

// Parameter of type for which common::refersToErrorType(...)
// returns true or for which id is TypeId::Error
const char* const ERROR_SECTION = "ERROR";

// Parameter of a type that is none of the above
const char* const SIMPLE_SECTION = "SIMPLE";

std::string sectionName(const FullTypeRef& paramTypeRef)
{
    if (paramTypeRef.id() == nodes::TypeId::Custom) {
        if (paramTypeRef.isError()) {
            return ERROR_SECTION;
        }

        auto l = paramTypeRef.as<nodes::Listener>();
        if (l) {
            return l->isLambda ? LAMBDA_SECTION : LISTENER_SECTION;
        }
    }

    return SIMPLE_SECTION;
}

} // namespace

ctemplate::TemplateDictionary* FunctionMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Function& f,
    bool isLambda)
{
    if (!needGenerateNode(scope, f))
        return parentDict;

    auto dict = cpp::FunctionMaker::make(parentDict, scope, f, isLambda);

    dict->SetValue("JNI_FUNCTION_NAME", f.name[JAVA].first());

    FullTypeRef resultTypeRef(scope, f.result.typeRef);

    dict->SetValue("JNI_RESULT_TYPE_NAME",
        jniTypeName(resultTypeRef, false));
    dict->SetValue("JNI_RESULT_GLOBAL_REF_TYPE_NAME",
        jniTypeName(resultTypeRef, true));
    dict->SetValue("JNI_RESULT_TYPE_REF",
        jniTypeRef(resultTypeRef, false));

    auto resultSectionName = sectionName(resultTypeRef);
    auto resultSectionDict = dict->AddSectionDictionary(resultSectionName);

    if (resultSectionName == LISTENER_SECTION) {
        resultSectionDict->SetValue("LISTENER_BINDING_TYPE_NAME",
            cpp::listenerBindingTypeName("android", resultTypeRef.info()));

        generatedListenerReturnValue_ = true;
        if (resultTypeRef.isStrongListener()) {
            generatedStrongRefListenerReturnValue_ = true;
        }
    }

    return dict;
}

ctemplate::TemplateDictionary* FunctionMaker::createDict(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& /* scope */,
    bool /* isLambda */) const
{
    return parentDict->AddSectionDictionary("METHOD");
}

void FunctionMaker::makeParameter(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Function& function)
{
    const nodes::FunctionParameter& p =
        function.parameters[parameterIndexInIdl_];

    auto dict = parentDict->AddSectionDictionary("PARAMETER");
    createParameterDict(dict, scope, p, function);

    FullTypeRef typeRef(scope, p.typeRef);
    common::addNullAsserts(parentDict, typeRef, p.name);

    // Main parameter info
    auto parameterSectionName = sectionName(typeRef);
    if (parameterSectionName == LISTENER_SECTION) {
        // PARAM_TYPE value for a listener is wrapped in shared_ptr<...>,
        // so we create variables with clean listener scope and name:

        const auto& typeInfo = typeRef.info();
        dict->SetValue(
            "LISTENER_TYPE_NAME", cpp::fullName(typeInfo.fullNameAsScope[CPP]));
        dict->SetValue("LISTENER_NAME", p.typeRef.name->last());

        dict->SetValue("LISTENER_BINDING_TYPE_NAME",
            cpp::listenerBindingTypeName("android", typeInfo));

        const nodes::Listener* l = std::get<const nodes::Listener*>(typeInfo.type);
        if (l->isStrongRef) {
            generatedStrongRefListenerParameter_ = true;

            dict->AddSectionDictionary("STRONG_LISTENER_PARAM");
        } else {
            dict->AddSectionDictionary("WEAK_LISTENER_PARAM");
        }
    } else if (parameterSectionName == LAMBDA_SECTION) {
        const auto& typeInfo = typeRef.info();

        auto listenerScopeSuffix = typeInfo.scope.original();
        if (!listenerScopeSuffix.isEmpty()) {
            --listenerScopeSuffix;
        }
        dict->SetValue("LISTENER_SCOPE",
            (typeInfo.idl->cppNamespace + listenerScopeSuffix).asPrefix("::"));
    } else if (p.typeRef.id == nodes::TypeId::Custom) {
        if (typeRef.isStrongInterface()) {
            dict->ShowSection("MOVE");
        }
    }

    auto coreDict = dict->AddSectionDictionary(parameterSectionName);
    cpp::FunctionMaker::makeParameter(coreDict, scope, function);

    // JNI parameter info
    dict->SetValue("JNI_PARAM_NAME", p.name);
    dict->SetValue("JNI_PARAM_TYPE_REF", jniTypeRef(typeRef, false));

    auto jniParamDict = dict->AddSectionDictionary("JNI_PARAM");
    jniParamDict->SetValue("PARAM_TYPE", jniTypeName(typeRef, false));
    jniParamDict->SetValue("PARAM_NAME", p.name);
}

} // namespace jni_cpp
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
