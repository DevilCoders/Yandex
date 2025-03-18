#pragma once

#include <yandex/maps/idl/full_type_ref.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace common {

class PodDecider {
public:
    virtual ~PodDecider() { }

    /**
     * Tells whether given type reference refers to a plain-old-data type.
     */
    virtual bool isPod(const FullTypeRef& ref) const;
};

} // namespace common
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
