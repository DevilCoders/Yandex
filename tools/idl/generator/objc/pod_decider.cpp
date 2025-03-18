#include "objc/pod_decider.h"

#include <yandex/maps/idl/utils/common.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace objc {

bool PodDecider::isPod(const FullTypeRef& ref) const
{
    if (ref.id() == nodes::TypeId::Any ||
            (!ref.isOptional() && ref.id() == nodes::TypeId::Point)) {
        return true;
    }

    return common::PodDecider::isPod(ref) &&
        ref.id() != nodes::TypeId::AbsTimestamp &&
        ref.id() != nodes::TypeId::RelTimestamp &&
        ref.id() != nodes::TypeId::Color &&
        !ref.isError();
}

} // namespace objc
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
