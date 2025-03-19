#pragma once

#include "configure.h"

#include <kernel/text_machine/parts/common/types.h>
#include <kernel/text_machine/parts/common/scatter.h>
#include <kernel/text_machine/parts/common/storage.h>
#include <kernel/text_machine/parts/common/floats.h>
#include <kernel/text_machine/parts/common/queries_helper.h>
#include <kernel/text_machine/interface/text_machine.h>

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    MACHINE_PARTS(Core) {
        struct TConfigureInfo {
        };

        struct TMultiQueryInfo {
            const TQueriesHelper& QueriesHelper;
        };

        struct TDocInfo {
        };

        struct TAnnotationInfo {
        };

        struct THitInfo {
            const THit& Hit;
            size_t LocalQueryId;
        };

        struct TBlockHitInfo {
            const TBlockHit& BlockHit;
        };

        struct TSaveFeaturesInfo {
            TFeaturesBuffer& Features;
        };
    } // MACHINE_PARTS(Core)
} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>
