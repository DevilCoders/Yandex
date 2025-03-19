#pragma once

#include <kernel/synnorm/synnorm.h>

//This is standalone version of TSynNormalizer
//Returns preinited object
//Required data is linked with target, expected binary size increase is about 25M
//Required data is build from arcadia, see ya.make
//Required data is not binary prepared, so it also use additional space in RAM

namespace NSynNorm {
    const TSynNormalizer& GetStandAloneSynnormNormalizer();
}
