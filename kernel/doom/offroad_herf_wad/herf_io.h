#pragma once
#include <kernel/doom/offroad_erf_wad/erf_io.h>

class THostErfInfo;
namespace NDoom {
    using THostErfIo = TErfGeneralIo<THostErfInfo, HostErfIndexType, DefaultHostErfIoModel>;
} // namespace NDoom

using THostErfAccessor = TGeneralErfAccessor<NDoom::THostErfIo>;
