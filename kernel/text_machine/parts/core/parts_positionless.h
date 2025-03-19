#pragma once

#include "parts_base.h"

#include <kernel/text_machine/parts/tracker/motor.h>
#include <kernel/text_machine/parts/accumulators/positionless_accumulator_parts.h>

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    MACHINE_PARTS(Core) {
        UNIT_FAMILY(TPositionlessTrackerFamily)

        UNIT_FAMILY_INFO_BEGIN(TPositionlessTrackerFamily)
            UNIT_METHOD_FORWARD_ARG(
                AddStreamBlockHit,
                ::NLingBoost::TStreamTypeDescr,
                Stream);
        UNIT_FAMILY_INFO_END()

        template <typename TrackerType>
        UNIT(TPositionlessTrackerStub) {
            REQUIRE_MACHINE(Tracker, TrackerType);

            using TTracker = TrackerType;

            UNIT_FAMILY_STATE(TPositionlessTrackerFamily) {
                TTracker PositionlessTracker;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, PositionlessTracker);
                }
            };

            UNIT_PROCESSOR_CUSTOM(TProcessor) {
                UNIT_PROCESSOR_METHODS

                REQUIRE_UNIT(
                    TCoreUnit
                );

                static void ScatterStatic(TScatterMethod::CollectStreams, const TStaticStreamsInfo& info) {
                    TTracker::ScatterStatic(TScatterMethod::CollectStreams(), info);
                }
                static void ScatterStatic(TScatterMethod::CollectValues, const TStaticValuesInfo& info) {
                    TTracker::ScatterStatic(TScatterMethod::CollectValues(), info);
                }

                void NewMultiQuery(TMemoryPool& pool, const TMultiQueryInfo& info) {
                    Vars().PositionlessTracker.NewMultiQuery(pool, Proc<TCoreUnit>().GetSharedState(),
                        info.QueriesHelper, nullptr);
                }
                void Scatter(TScatterMethod::RegisterValues, const TRegisterValuesInfo& info) {
                    Vars().PositionlessTracker.Scatter(TScatterMethod::RegisterValues(), info);
                }
                Y_FORCE_INLINE void NewDoc(const TDocInfo&) {
                    Vars().PositionlessTracker.NewDoc();
                }
                template <EStreamType Stream>
                Y_FORCE_INLINE void AddStreamBlockHit(const TBlockHitInfo& hit, TStreamSelector<Stream>) {
                    Vars().PositionlessTracker.template AddBlockHit<Stream>(hit.BlockHit);
                }
                Y_FORCE_INLINE void FinishDoc(const TDocInfo&) {
                    Vars().PositionlessTracker.FinishDoc();
                }
            };
        };
    } // MACHINE_PARTS(Core)
} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>
