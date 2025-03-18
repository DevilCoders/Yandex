#include "java/function_maker.h"

#include "java/annotation_addition.h"
#include <yandex/maps/idl/utils.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace java {

ctemplate::TemplateDictionary* FunctionMaker::make(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::Function& f,
    bool isLambda)
{
    if (!needGenerateNode(scope, f))
        return parentDict;

    auto dict = common::FunctionMaker::make(parentDict, scope, f, isLambda);

    FullTypeRef resultTypeRef(scope, f.result.typeRef);
    addNullabilityAnnotation(dict, importMaker_, resultTypeRef, true, "RESULT_");

    return dict;
}

ctemplate::TemplateDictionary* FunctionMaker::createParameterDict(
    ctemplate::TemplateDictionary* parentDict,
    const FullScope& scope,
    const nodes::FunctionParameter& p,
    const nodes::Function& function)
{
    auto dict = common::FunctionMaker::createParameterDict(parentDict, scope, p, function);

    FullTypeRef typeRef(scope, p.typeRef);
    addNullabilityAnnotation(dict, importMaker_, typeRef);

    return dict;
}

} // namespace java
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
