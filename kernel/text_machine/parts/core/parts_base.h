#pragma once

#include "types.h"

#include <kernel/text_machine/parts/common/scatter.h>
#include <kernel/text_machine/parts/common/features_helper.h>

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    inline float CalcAnnAttenuationMultiplier(size_t wordPos) {
        return 1.0f / float(1 + wordPos);
    }

    MACHINE_PARTS(Core) {
        UNIT_INFO_BEGIN(TCoreUnit)
        UNIT_INFO_END()

        UNIT(TCoreUnit) {
            UNIT_STATE {
                const TQueriesHelper* QueriesHelper = nullptr;

                TStreamsMap Streams;
                THitWeightsMap HitWeights;
                bool EnableCompactEnums = true;

                IInfoCollector* Collector = nullptr;

                const TQueriesHelper& GetQueriesHelper() const {
                    Y_ASSERT(QueriesHelper);
                    return *QueriesHelper;
                }
                void SaveToJson(NJson::TJsonValue& value) const {
                    SAVE_JSON_VAR(value, HitWeights);
                }
            };

            UNIT_PROCESSOR {
            public:
                UNIT_PROCESSOR_METHODS

                UNIT_PROCESSOR_STATIC {
                    TVector<EExpansionType> AllowedExpansions;
                    TVector<EStreamType> AllowedStreams;

                    THolder<TExpansionRemap> ExpansionRemap;
                    THolder<TStreamRemap> StreamRemap;

                    THolder<TExpansionRemap> FullExpansionRemap;
                    THolder<TStreamRemap> FullStreamRemap;

                    THolder<TFeaturesHelper> FeaturesHelper;

                    TValuesStream Values;
                    TValueRefsStream ValueRefs;
                    ui64 NumIntCounters = 0;

                    UNIT_PROCESSOR_STATIC_INIT {
                        TSet<EExpansionType> allowedExpansionsSet;
                        TSet<EStreamType> allowedStreamsSet;

                        TMachine::ScatterStatic(TScatterMethod::CollectExpansions(),
                            TStaticExpansionsInfo{allowedExpansionsSet});
                        TMachine::ScatterStatic(TScatterMethod::CollectStreams(),
                            TStaticStreamsInfo{allowedStreamsSet});

                        AllowedExpansions.assign(allowedExpansionsSet.begin(), allowedExpansionsSet.end());
                        AllowedStreams.assign(allowedStreamsSet.begin(), allowedStreamsSet.end());

                        ExpansionRemap.Reset(new TExpansionRemap(MakeArrayRef(AllowedExpansions)));
                        StreamRemap.Reset(new TStreamRemap(MakeArrayRef(AllowedStreams)));

                        FullExpansionRemap.Reset(new TExpansionRemap(TExpansion::GetValues()));
                        FullStreamRemap.Reset(new TStreamRemap(TStream::GetValues()));

                        InitFeatures();
                        InitValues();
                        InitValueRefs();
                    }

                    void InitFeatures() {
                        TFeaturesStream features;
                        auto featuresWriter = features.CreateWriter();

                        TMachine::ScatterStatic(TScatterMethod::CollectFeatures(), TStaticFeaturesInfo{featuresWriter});
                        FeaturesHelper.Reset(new TFeaturesHelper(features));
                    }

                    void InitValues() {
                        auto valuesWriter = Values.CreateWriter();
                        TMachine::ScatterStatic(TScatterMethod::CollectValues(), TStaticValuesInfo{valuesWriter});
                    }

                    void InitValueRefs() {
                        auto valueRefsWriter = ValueRefs.CreateWriter();
                        TMachine::ScatterStatic(TScatterMethod::CollectValueRefs(), TStaticValueRefsInfo{valueRefsWriter});

                        THashMap<TValueIdWithHash, i64> indexById;

                        auto floatsReader = Values.CreateReader();
                        size_t floatIndex = 0;
                        size_t intIndex = 0;
                        NumIntCounters = 0;
                        for (size_t index : xrange(Values.NumItems())) {
                            const auto& item = Values[index];
                            if (item.Id.GetId().template IsValid<EValuePartType::FormsCount>()) {
                                indexById[Values[index].Id] = intIndex++;
                            } else {
                                indexById[Values[index].Id] = floatIndex++;
                            }
                        }
                        NumIntCounters = intIndex;

                        auto valueRefsReader = ValueRefs.CreateReader();
                        for (size_t index : xrange(ValueRefs.NumItems())) {
                            auto& entry = ValueRefs[index];
                            const auto* valueIndexPtr = indexById.FindPtr(entry.Id);
                            if (Y_LIKELY(valueIndexPtr)) {
                                Y_ASSERT(*valueIndexPtr >= 0);
                                entry.ValueIndex = *valueIndexPtr;
                            } else {
                                MACHINE_LOG("Core::TCoreUnit", "MissingFloat", TStringBuilder{}
                                    << "{Id: " << entry.Id.GetId().FullName()  << "}");

                                Y_ASSERT(false);
                            }
                        }
                    }
                };

                TExpansionRemap& GetExpansionRemap(bool useCompactEnums) const {
                    return useCompactEnums ? *Static().ExpansionRemap : *Static().FullExpansionRemap;
                }
                TStreamRemap& GetStreamRemap(bool useCompactEnums) const {
                    return useCompactEnums ? *Static().StreamRemap : *Static().FullStreamRemap;
                }
                const TVector<EExpansionType>& GetAllowedExpansions(bool useCompactEnums) const {
                    return useCompactEnums ? Static().AllowedExpansions : TExpansion::GetValuesVector();
                }
                const TVector<EStreamType>& GetAllowedStreams(bool useCompactEnums) const {
                    return useCompactEnums ? Static().AllowedStreams : TStream::GetValuesVector();
                }

                TCoreSharedState GetSharedState() const {
                    return TCoreSharedState{
                        GetExpansionRemap(Vars().EnableCompactEnums),
                        GetStreamRemap(Vars().EnableCompactEnums),
                        Vars().Streams, Vars().HitWeights};
                }

                void Scatter(TScatterMethod::ConfigureSharedState, const TConfigureSharedStateInfo& info) {
                    Vars().EnableCompactEnums = info.EnableCompactEnums;

                    Vars().Streams = TStreamsMap(info.Pool, GetStreamRemap(Vars().EnableCompactEnums), nullptr);
                    Vars().HitWeights = THitWeightsMap(info.Pool, 0.0f);
                    Vars().Collector = info.Collector;
                }

                Y_FORCE_INLINE void NewMultiQuery(TMemoryPool& /*pool*/, const TMultiQueryInfo& info) {
                    Vars().QueriesHelper = &info.QueriesHelper;
                }
                Y_FORCE_INLINE void NewDoc(const TDocInfo&) {
                    Vars().Streams.Fill(nullptr);
                    Vars().HitWeights.Fill(1.0f);
                }
                Y_FORCE_INLINE void AddBlockHit(const TBlockHitInfo& info) {
                    auto& hit = info.BlockHit;

                    Y_ASSERT(hit.Position.Annotation);
                    Y_ASSERT(hit.Position.Annotation->Stream);
                    Y_ASSERT(hit.Position.Annotation->Stream->Type < TStream::StreamMax);
                    Y_ASSERT(hit.Position.Annotation->Value < 1.0f + FloatEpsilon);
                    Y_ASSERT((hit.Position.Annotation->Stream->Type != TStream::Url) || (hit.Position.Annotation->Value > 1.0f - FloatEpsilon));
                    Y_ASSERT((hit.Position.Annotation->Stream->Type != TStream::Title) || (hit.Position.Annotation->Value > 1.0f - FloatEpsilon));
                    Y_ASSERT((hit.Position.Annotation->Stream->Type != TStream::Body) || (hit.Position.Annotation->Value > 1.0f - FloatEpsilon));
                    Y_ASSERT((hit.Position.Annotation->Stream->Type != TStream::LinksInternal) || (hit.Position.Annotation->Value > 1.0f - FloatEpsilon));

                    Streams[hit.Position.Annotation->Stream->Type] = hit.Position.Annotation->Stream;

                    HitWeights[THitWeight::V1] = hit.Position.Annotation->Value;
                    HitWeights[THitWeight::V2] = HitWeights[THitWeight::V1] * HitWeights[THitWeight::V1];
                    HitWeights[THitWeight::V4] = HitWeights[THitWeight::V2] * HitWeights[THitWeight::V2];
                    HitWeights[THitWeight::AttenV1] = hit.Position.Annotation->Value * CalcAnnAttenuationMultiplier(hit.Position.LeftWordPos);
                    //TODO: костыли-костылики, считать затухание на каждый блокхит это какой-то ад
                    if (Y_UNLIKELY(hit.Position.Annotation->Stream->Type == TStream::FullText)) {
                        const auto& brkId = hit.Position.Annotation->BreakNumber;
                        HitWeights[THitWeight::OldTRAtten] = 100.0f / (brkId + 100.0f);
                        HitWeights[THitWeight::TxtHead] = (brkId == 1 ? 1.0 : 0);
                        HitWeights[THitWeight::TxtHiRel] = (hit.Position.Relevance >= 2 ? 1.0 : 0);
                    }
                }
            };
        };
    } // MACHINE_PARTS(Core)
} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>
