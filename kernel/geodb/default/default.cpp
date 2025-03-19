#include "default.h"

#include <kernel/geodb/geodb.h>

#include <library/cpp/archive/yarchive.h>

#include <util/generic/singleton.h>
#include <util/memory/blob.h>
#include <util/system/types.h>

extern "C" {
    extern const ui8 KERNEL_GEODB_DEFAULT_GEODB_SERIALIZED[];
    extern const ui32 KERNEL_GEODB_DEFAULT_GEODB_SERIALIZEDSize;
}

namespace {
    struct TDefaultGeoDBHolder {
        TDefaultGeoDBHolder() {
            auto&& reader = TArchiveReader{TBlob::NoCopy(
                KERNEL_GEODB_DEFAULT_GEODB_SERIALIZED, KERNEL_GEODB_DEFAULT_GEODB_SERIALIZEDSize
            )};
            const auto input = reader.ObjectByKey("/geodb.serialized");
            GeoDB.Load(input.Get());
        }

        NGeoDB::TGeoKeeper GeoDB;
    };
}

const NGeoDB::TGeoKeeper& NGeoDB::DefaultGeoDB() {
    return Default<TDefaultGeoDBHolder>().GeoDB;
}
