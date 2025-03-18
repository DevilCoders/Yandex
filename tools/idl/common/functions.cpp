#include <yandex/maps/idl/functions.h>

#include <yandex/maps/idl/nodes/nodes.h>

#include <cstddef>

namespace yandex {
namespace maps {
namespace idl {

bool isMatchingFunctionRef(
    const FullScope& functionRefScope,
    const std::string& functionRefName,
    const std::vector<nodes::TypeRef>& functionRefParameterTypeRefs,
    const FullScope& functionScope,
    const nodes::Function& function)
{
    if (functionRefName != function.name.original()[0]) {
        return false;
    }
    if (functionRefParameterTypeRefs.size() != function.parameters.size()) {
        return false;
    }

    for (std::size_t i = 0; i < function.parameters.size(); ++i) {
        const auto& typeRef1 = functionRefParameterTypeRefs[i];
        const auto& typeRef2 = function.parameters[i].typeRef;

        if (typeRef1.id != typeRef2.id ||
                typeRef1.isConst != typeRef2.isConst ||
                typeRef1.isOptional != typeRef2.isOptional) {
            return false;
        }

        if (typeRef1.id == nodes::TypeId::Custom) {
            const auto& typeInfo1 = functionRefScope.type(*typeRef1.name);
            const auto& typeInfo2 = functionScope.type(*typeRef2.name);
            if (typeInfo1 != typeInfo2) {
                return false;
            }
        }
    }

    return true;
}

const nodes::Function* findMethod(
    const FullScope& functionRefScope,
    const std::string& functionRefName,
    const std::vector<nodes::TypeRef>& functionRefParameterTypeRefs,
    const FullScope& interfaceScope,
    const nodes::Interface& i)
{
    const nodes::Function* resultingMethod = nullptr;

    auto methodScope = interfaceScope + i.name.original();

    i.nodes.lambdaTraverse(
        [&](const nodes::Function& m)
        {
            if (isMatchingFunctionRef(functionRefScope, functionRefName,
                    functionRefParameterTypeRefs, methodScope, m)) {
                resultingMethod = &m;
            }
        });

    return resultingMethod;
}

const nodes::Function* findMethod(
    const FullScope& functionRefScope,
    const std::string& functionRefName,
    const std::vector<nodes::TypeRef>& functionRefParameterTypeRefs,
    const FullScope& listenerScope,
    const nodes::Listener& l)
{
    auto methodScope = listenerScope + l.name.original();

    for (const auto& m : l.functions) {
        if (isMatchingFunctionRef(functionRefScope, functionRefName,
                functionRefParameterTypeRefs, methodScope, m)) {
            return &m;
        }
    }
    return nullptr;
}

const nodes::Function* findFunction(
    const FullScope& functionRefScope,
    const std::string& functionRefName,
    const std::vector<nodes::TypeRef>& functionRefParameterTypeRefs,
    const TypeInfo& typeInfo)
{
    FullScope typeScope{ typeInfo.idl, typeInfo.scope.original() };

    auto i = std::get_if<const nodes::Interface*>(&typeInfo.type);
    if (i) {
        return findMethod(functionRefScope, functionRefName,
            functionRefParameterTypeRefs, typeScope, **i);
    }

    auto l = std::get_if<const nodes::Listener*>(&typeInfo.type);
    if (l) {
        return findMethod(functionRefScope, functionRefName,
            functionRefParameterTypeRefs, typeScope, **l);
    }

    return nullptr;
}

} // namespace idl
} // namespace maps
} // namespace yandex
