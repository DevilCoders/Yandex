#pragma once

#include "common/pod_decider.h"

#include <yandex/maps/idl/full_type_ref.h>

namespace yandex {
namespace maps {
namespace idl {
namespace generator {
namespace objc {

class PodDecider : public common::PodDecider {
public:
    virtual bool isPod(const FullTypeRef& ref) const override;
};

} // namespace objc
} // namespace generator
} // namespace idl
} // namespace maps
} // namespace yandex
