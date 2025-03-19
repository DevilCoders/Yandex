#pragma once

#include "types.h"

#include "positionless_units.h"
#include "annotation_units.h"
#include "field_set_units.h"
#include "bow_units.h"
#include "parts_plane.h"
#include "parts_window.h"
#include "parts_atten.h"
#include "parts_field_set.h"

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    MACHINE_PARTS(Tracker) {
        MACHINE_MOTOR {
        public:
            using TModule = M;

        public:
            void NewMultiQuery(TMemoryPool& pool,
                const TCoreSharedState& coreState,
                const TQueriesHelper& queriesHelper,
                const TBagOfWords* bagOfWords)
            {
                TMultiQueryInfo info{coreState, queriesHelper, bagOfWords};
                M::NewMultiQuery(pool, info);
            }
            void NewQuery(TMemoryPool& pool,
                const TCoreSharedState& coreState,
                const TQuery& query,
                const TWeights& mainWeights,
                const TWeights& exactWeights)
            {
                TQueryInfo info{coreState, query, mainWeights, exactWeights};
                M::NewQuery(pool, info);
            }
            void NewDoc() {
                TDocInfo info{};
                M::NewDoc(info);
            }
            template <EStreamType Stream>
            Y_FORCE_INLINE void AddHit(const THit& hit) {
                const THitInfo& info = hit;
                M::AddHit(info);
                M::AddStreamHit(info, TStreamSelector<Stream>());
                M::FinishHit(info);
            }
            template <EStreamType Stream>
            Y_FORCE_INLINE void AddBlockHit(const TBlockHit& hit) {
                const TBlockHitInfo& info = hit;
                M::AddBlockHit(info);
                M::AddStreamBlockHit(info, TStreamSelector<Stream>());
                M::FinishBlockHit(info);
            }
            void FinishDoc() {
                TDocInfo info{};
                M::FinishDoc(info);
            }
        };
    } // MACHINE_PARTS(Tracker)
} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>

