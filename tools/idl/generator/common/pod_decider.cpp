#include "common/pod_decider.h"

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace common {

bool PodDecider::isPod(const FullTypeRef& ref) const
{
    if (ref.isOptional()) {
        return false;
    }

    return ref.id() == nodes::TypeId::Bool ||
        ref.id() == nodes::TypeId::Int ||
        ref.id() == nodes::TypeId::Uint ||
        ref.id() == nodes::TypeId::Int64 ||
        ref.id() == nodes::TypeId::Float ||
        ref.id() == nodes::TypeId::Double ||
        ref.id() == nodes::TypeId::TimeInterval ||
        ref.id() == nodes::TypeId::AbsTimestamp ||
        ref.id() == nodes::TypeId::RelTimestamp ||
        ref.id() == nodes::TypeId::Color ||
        ref.is<nodes::Enum>();
}

} // namespace common
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
