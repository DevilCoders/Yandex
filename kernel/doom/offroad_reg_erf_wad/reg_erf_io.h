#pragma once

#include <kernel/doom/offroad_reg_herf_wad/reg_herf_io.h>

class TRegErfInfo;
namespace NDoom {
    using TRegErfIo = TRegErfGeneralIo<TRegErfInfo, RegErfIndexType, DefaultRegErfIoModel>;
}
using TRegErfAccessor = NDoom::TGeneralRegErfAccessor<NDoom::TRegErfIo>;
