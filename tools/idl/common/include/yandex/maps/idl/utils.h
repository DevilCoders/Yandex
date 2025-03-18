#pragma once

#include <yandex/maps/idl/idl.h>
#include <yandex/maps/idl/full_type_ref.h>
#include <yandex/maps/idl/nodes/type_ref.h>

namespace yandex::maps::idl {

inline bool isInternalTypeRef(const FullScope& scope, const nodes::TypeRef& typeRef)
{
    return FullTypeRef(scope, typeRef).isInternal();
}

bool hasInternalTypes(const FullScope& scope, const nodes::Function& f);

bool hasInternalTypes(const FullScope& scope, const nodes::Property& p);

bool disableInternalChecks(const FullScope& scope);

/**
 * We should not generate a node for platform (iOS/Android) with @internal tag for public release
 */
template <typename Node>
bool needGenerateNode(const FullScope& scope, const Node& n)
{
    if (!scope.idl->env->config.isPublic)
        return true;
    return !FullTypeRef(scope, n.name).isInternal();
}

bool needGenerateNode(const FullScope& scope, const nodes::Function& f);
bool needGenerateNode(const FullScope& scope, const nodes::Property& p);
bool needGenerateNode(const FullScope& scope, const nodes::StructField& structField);

} // namespace yandex::maps::idl
