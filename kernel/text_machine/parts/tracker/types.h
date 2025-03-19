#pragma once

#include <kernel/text_machine/parts/common/types.h>
#include <kernel/text_machine/parts/common/weights.h>
#include <kernel/text_machine/parts/common/bag_of_words.h>
#include <kernel/text_machine/parts/common/queries_helper.h>

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    MACHINE_PARTS(Tracker) {
        struct TQueryInfo {
            const TCoreSharedState& CoreState;
            const TQuery& Query;
            const TWeights& MainWeights;
            const TWeights& ExactWeights;
        };

        struct TDocInfo {
        };

        struct TMultiQueryInfo {
            const TCoreSharedState& CoreState;
            const TQueriesHelper& QueriesHelper;
            const TBagOfWords* BagOfWords;
        };

        using TBlockHitInfo = TBlockHit;
        using THitInfo = THit;
    } // MACHINE_PARTS(Tracker)
} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>
