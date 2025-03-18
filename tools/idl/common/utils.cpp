#include <yandex/maps/idl/utils.h>

namespace yandex::maps::idl {

bool hasInternalTypes(const FullScope& scope, const nodes::Function& f)
{
    if (isInternalTypeRef(scope, f.result.typeRef))
        return true;

    for (const auto& parameter: f.parameters) {
        if (isInternalTypeRef(scope, parameter.typeRef))
            return true;
    }

    return false;
}

bool hasInternalTypes(const FullScope& scope, const nodes::Property& p)
{
    return isInternalTypeRef(scope, p.typeRef);
}

bool disableInternalChecks(const FullScope& scope)
{
    return scope.idl->env->config.disableInternalChecks;
}

bool needGenerateNode(const FullScope& scope, const nodes::Function& f)
{
    if (scope.idl->env->config.isPublic && hasInternalDoc(f))
        return false;
    return true;
}

bool needGenerateNode(const FullScope& scope, const nodes::Property& p)
{
    if (scope.idl->env->config.isPublic && hasInternalDoc(p))
        return false;
    return true;
}

bool needGenerateNode(const FullScope& scope, const nodes::StructField& structField)
{
    if (scope.idl->env->config.isPublic && hasInternalDoc(structField))
        return false;
    return true;
}

} // namespace yandex::maps::idl
