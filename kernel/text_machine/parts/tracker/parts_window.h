#pragma once

#include "parts_base.h"

#include <kernel/text_machine/parts/accumulators/window_accumulator.h>

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    MACHINE_PARTS(Tracker) {
        UNIT_INFO_BEGIN(TMinWindowUnit)
            UNIT_METHOD_INSTANTIATE_ARG(
                AddStreamHitBasic,
                ::NLingBoost::TStreamTypeDescr,
                StreamType,
                TStream::Body);
        UNIT_INFO_END()

        UNIT(TMinWindowUnit) {
            UNIT_STATE{
                TMinWindow MinWindow;
                size_t BodyLength = Max<size_t>();

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, MinWindow);
                    SAVE_JSON_VAR(value, BodyLength);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                static void ScatterStatic(TScatterMethod::CollectStreams, const TStaticStreamsInfo& info) {
                    info.Streams.insert(TStream::Body);
                }

                Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQueryInfo& info) {
                    MinWindow.Init(pool, info.Query.Words.size());
                }

                Y_FORCE_INLINE void NewDoc(const TDocInfo&) {
                    MinWindow.NewDoc();
                    BodyLength = 1;
                }

                template<EStreamType StreamType>
                Y_FORCE_INLINE void AddStreamHitBasic(const THitInfo& hit) {
                    static_assert(StreamType == TStream::Body, "");
                    MinWindow.AddHit(hit.Word.WordId, hit.Position.Annotation->FirstWordPos + (ui32)hit.Position.LeftWordPos);
                }

                Y_FORCE_INLINE void FinishDoc(const TDocInfo&) {
                    if (Vars<TCoreUnit>().Streams[TStream::Body]) {
                        BodyLength = Vars<TCoreUnit>().Streams[TStream::Body]->WordCount + 1;
                    }
                }

            public:
                Y_FORCE_INLINE float CalcMinWindowSize() const {
                    return MinWindow.CalcMinWindowSize();
                }

                Y_FORCE_INLINE float CalcFullMatchCoverageMinOffset() const {
                    return MinWindow.CalcFullMatchCoverageMinOffset();
                }

                Y_FORCE_INLINE float CalcFullMatchCoverageStartingPercent() const {
                    return MinWindow.CalcFullMatchCoverageStartingPercent(BodyLength);
                }

            };
        };
    } // MACHINE_PARTS(Tracker)
} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>

