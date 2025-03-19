#pragma once

#include "types.h"

#include <kernel/text_machine/parts/common/scatter.h>

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    MACHINE_PARTS(Tracker) {
        UNIT_INFO_BEGIN(TCoreUnit)
        UNIT_INFO_END()

        UNIT(TCoreUnit) {
            UNIT_STATE {
                TStreamsMap Streams;
                THitWeightsMap HitWeights;

                bool FirstHitInStream = false;
                TOpenStreamsMap OpenStreams;

                const TQuery* Query = nullptr;
                TWeights MainWeights;
                TWeights ExactWeights;
                TWeightsHolder PairWordWeights;
                TWeightsHolder MainIdf;

                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, MainWeights);
                    SAVE_JSON_VAR(value, ExactWeights);
                    SAVE_JSON_VAR(value, FirstHitInStream);
                    SAVE_JSON_VAR(value, OpenStreams);
                }
            };

            UNIT_PROCESSOR {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQueryInfo& info) {
                    Init(pool, info.CoreState.Streams, info.CoreState.HitWeights, info.CoreState.StreamRemap);

                    Vars().Query = &info.Query;
                    Vars().MainWeights = info.MainWeights;
                    Vars().ExactWeights = info.ExactWeights;
                    //TODO: MainIdf точно нужен?
                    Vars().PairWordWeights.Init(pool, info.MainWeights.size());
                    Vars().MainIdf.Init(pool, info.MainWeights.size());
                    float sumWeights = 1e-10;
                    if (info.ExactWeights.size()) {
                        Vars().PairWordWeights[0] = 0;
                        Vars().MainIdf[0] = info.Query.GetIdf(0);
                        for (size_t i = 1; i < info.MainWeights.size(); ++i) {
                            float pairWeight = info.Query.GetIdf(i) + info.Query.GetIdf(i - 1);
                            Vars().PairWordWeights[i] = pairWeight;
                            sumWeights += pairWeight;
                            Vars().MainIdf[i] = info.Query.GetIdf(i);
                        }
                        for (size_t i = 1; i < info.MainWeights.size(); ++i) {
                            Vars().PairWordWeights[i] /= sumWeights;
                        }
                    }
                }
                Y_FORCE_INLINE void NewMultiQuery(TMemoryPool& pool, const TMultiQueryInfo& info) {
                    Init(pool, info.CoreState.Streams, info.CoreState.HitWeights, info.CoreState.StreamRemap);
                }
                Y_FORCE_INLINE void NewDoc(const TDocInfo&) {
                    OpenStreams.Fill(false);
                    FirstHitInStream = false;
                }
                Y_FORCE_INLINE void AddHit(const THitInfo& hit) {
                    FirstHitInStream = !OpenStreams[hit.Position.Annotation->Stream->Type];
                    OpenStreams[hit.Position.Annotation->Stream->Type] = true;
                }

            private:
                Y_FORCE_INLINE void Init(TMemoryPool& pool,
                    const TStreamsMap& streams,
                    const THitWeightsMap& hitWeights,
                    const TStreamRemap& streamRemap)
                {
                    Vars().Streams = streams;
                    Vars().HitWeights = hitWeights;
                    Vars().OpenStreams = TOpenStreamsMap(pool, streamRemap, false);
                }
            };
        };
    } // MACHINE_PARTS(Tracker)
} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>
