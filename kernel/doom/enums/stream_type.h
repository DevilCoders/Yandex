#pragma once

#include <kernel/doom/enums/enum_wrapper.h>

namespace NDoom {

Y_DEFINE_ENUM_WRAPPER(EStreamType);

enum {
    MaxStreamTypeCount = 24, // TODO: static_assert
};

} // namespace NDoom
