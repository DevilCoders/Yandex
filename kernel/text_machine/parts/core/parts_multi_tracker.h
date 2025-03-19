#pragma once

#include "parts_base.h"
#include "aggregator_parts.h"

#include <kernel/text_machine/parts/tracker/motor.h>

#include <kernel/text_machine/module/module_def.inc>

#include <util/string/builder.h>

namespace NTextMachine {
namespace NCore {
    MACHINE_PARTS(Core) {
        UNIT_FAMILY_INFO_BEGIN(TMultiTrackerFamily, EExpansionType expansion, EQueryModeType mode)
            UNIT_METHOD_INSTANTIATE_ARG(
                AddStreamHitBasic,
                ::NLingBoost::TExpansionTypeDescr,
                Expansion,
                (mode == TQueryMode::UseOriginal
                    ? ::NModule::TValuesList(
                        expansion,
                        TExpansion::OriginalRequest)
                    : expansion));
        UNIT_FAMILY_INFO_END()

        template <EExpansionType ExpansionForGroup>
        struct TMultiTrackerGroup {
            UNIT_FAMILY(TMultiTrackerFamily)

            template <typename TrackerType,
                EQueryModeType Mode>
            UNIT(TMultiTrackerStub) {
                REQUIRE_MACHINE(Tracker, TrackerType);

                using TTracker = TrackerType;

                UNIT_FAMILY_STATE(TMultiTrackerFamily) {
                    TPodBuffer<const TQuery*> Queries;
                    TPoolPodHolder<TTracker> Trackers;

                    TTracker* OriginalTracker = nullptr;
                    const TQuery* OriginalQuery = nullptr;

                    TFloatsHolder TrackerFeatures;

                    void SaveToJson(NJson::TJsonValue& value) const {
                        if (TQueryMode::UseOriginal == Mode && !!OriginalTracker) {
                            OriginalTracker->SaveToJson(value["OriginalTracker"]);
                        }
                        for (size_t i : xrange(Trackers.Count())) {
                            value["Trackers"][i]["LocalQueryId"] = i;
                            Trackers[i].SaveToJson(value["Trackers"][i]);
                        }
                    }
                };

                UNIT_PROCESSOR {
                    UNIT_PROCESSOR_METHODS

                    REQUIRE_UNIT(
                        TCoreUnit
                    );

                    UNIT_PROCESSOR_STATIC {
                        size_t GetNumFeatures() const {
                            return TTracker::GetNumFeatures();
                        }
                        void SaveFeatureIds(TFFIdsBuffer& buffer) const {
                            const TFFIds& trackerIds = TTracker::GetFeatureIds();
                            for (const TFFId& id : trackerIds) {
                                buffer.Add(id);
                                buffer.Back().Set(ExpansionForGroup);
                            }
                        }
                        const TFFIds& GetTrackerFeatureIds() const {
                            return TTracker::GetFeatureIds();
                        }
                        EExpansionType GetExpansion() const {
                            return ExpansionForGroup;
                        }
                    };

                    static void ScatterStatic(TScatterMethod::CollectExpansions, const TStaticExpansionsInfo& info) {
                        info.Expansions.insert(ExpansionForGroup);
                        if (TQueryMode::UseOriginal == Mode) {
                            info.Expansions.insert(TExpansion::OriginalRequest);
                        }
                    }
                    static void ScatterStatic(TScatterMethod::CollectStreams, const TStaticStreamsInfo& info) {
                        TTracker::ScatterStatic(TScatterMethod::CollectStreams(), info);
                    }
                    static void ScatterStatic(TScatterMethod::CollectValueRefs, const TStaticValueRefsInfo& info) {
                        auto guard = info.ValueRefs.Guard(UnitId());

                        MACHINE_LOG("Core::TMultiTrackerStub", "CollectValueRefs", TStringBuilder{}
                            << "{Expansion: " << ExpansionForGroup
                            << ", UnitId: " << UnitId() << "}");

                        TTracker::ScatterStatic(TScatterMethod::CollectValueRefs(), info);
                    }
                    void Scatter(TScatterMethod::BindValueRefs, const TBindValuesInfo& info) {
                        auto guard = info.ValueRefs.Guard(UnitId());

                        if (!IsUnitEnabled()) {
                            return;
                        }

                        MACHINE_LOG("Core::TMultiTrackerStub", "BindValues", TStringBuilder{}
                            << "{Expansion: " << ExpansionForGroup
                            << ", UnitId: " << UnitId() << "}");

                        if (TQueryMode::UseOriginal == Mode) {
                            Y_ASSERT(Vars().OriginalTracker);
                            Vars().OriginalTracker->Scatter(TScatterMethod::BindValueRefs(), info);
                        }

                        for (auto& tracker : Vars().Trackers) {
                            guard.VisitChild();
                            tracker.Scatter(TScatterMethod::BindValueRefs(), info);
                        }
                    }

                    void NewMultiQuery(TMemoryPool& pool, const TMultiQueryInfo& info) {
                        Vars().TrackerFeatures.Init(pool, TTracker::GetNumFeatures(), EStorageMode::Empty);

                        auto& queriesHelper = info.QueriesHelper;
                        Vars().Queries = queriesHelper.GetQueries(ExpansionForGroup);
                        Vars().Trackers.Init(pool, Vars().Queries.Count(), EStorageMode::Full);

                        // NOTE. Pre-initialize all trackers with default ctor
                        // May degrade performance somewhat, but
                        // helps with bug detection
                        Vars().Trackers.Construct();

                        for (size_t i : xrange(Vars().Queries.Count())) {
                            Vars().Trackers[i].NewQuery(pool, Proc<TCoreUnit>().GetSharedState(),
                                *Vars().Queries[i],
                                queriesHelper.GetMainWeights(ExpansionForGroup, i),
                                queriesHelper.GetExactWeights(ExpansionForGroup, i));
                        }

                        if (TQueryMode::UseOriginal == Mode) {
                            const size_t originalQueryIndex = queriesHelper.GetOriginalQueryIndex();
                            Vars().OriginalQuery = &queriesHelper.GetOriginalQuery();
                            Vars().OriginalTracker = pool.New<TTracker>();
                            Vars().OriginalTracker->NewQuery(pool, Proc<TCoreUnit>().GetSharedState(),
                                *Vars().OriginalQuery,
                                queriesHelper.GetMainWeights(originalQueryIndex),
                                queriesHelper.GetExactWeights(originalQueryIndex));
                        }
                    }
                    void NewDoc(const TDocInfo&) {
                        if (TQueryMode::UseOriginal == Mode) {
                            Y_ASSERT(Vars().OriginalTracker);
                            Vars().OriginalTracker->NewDoc();
                        }
                        for (auto& tracker : Vars().Trackers) {
                            tracker.NewDoc();
                        }
                    }
                    template <EExpansionType Expansion, EStreamType Stream>
                    Y_FORCE_INLINE void AddStreamHitBasic(const THitInfo& info) {
                        if (ExpansionForGroup == Expansion) {
                            Y_ASSERT(info.LocalQueryId < Vars().Trackers.Count());
                            Y_ASSERT(Vars().Queries[info.LocalQueryId]);
                            Y_ASSERT(ExpansionForGroup == Vars().Queries[info.LocalQueryId]->ExpansionType);
                            // NOTE. About 50% of time is spent here.
                            // Number of calls is proportional to number of expansions.
                            //
                            Vars().Trackers[info.LocalQueryId].template AddHit<Stream>(info.Hit);
                        } else {
                            Y_ASSERT(TQueryMode::UseOriginal == Mode);
                            Y_ASSERT(TExpansion::OriginalRequest == Expansion);
                            Y_ASSERT(Vars().OriginalTracker);
                            Vars().OriginalTracker->template AddHit<Stream>(info.Hit);
                        }
                    }
                    void FinishDoc(const TDocInfo&) {
                        if (TQueryMode::UseOriginal == Mode) {
                            Y_ASSERT(Vars().OriginalTracker);
                            Vars().OriginalTracker->FinishDoc();
                        }
                        for (auto& tracker : Vars().Trackers) {
                            tracker.FinishDoc();
                        }

                        for (size_t i = 0; i != Vars().Trackers.Count(); ++i) {
                            Y_ASSERT(Vars().Queries[i]);
                            Vars().TrackerFeatures.Clear();
                            Vars().Trackers[i].CalcFeatures(Vars().TrackerFeatures);
                            VerifyAndFixFeatures(Vars().TrackerFeatures);

                            TConstFloatsBuffer featuresBuf{Vars().TrackerFeatures};
                            Machine().Scatter(TScatterMethod::AggregateFeatures<TMultiTrackerFamily>{},
                                TAggregateFeaturesInfo{*Vars().Queries[i], featuresBuf});
                        }
                        if (TQueryMode::UseOriginal == Mode) {
                            Vars().TrackerFeatures.Clear();
                            Vars().OriginalTracker->CalcFeatures(Vars().TrackerFeatures);
                            VerifyAndFixFeatures(Vars().TrackerFeatures);

                            TConstFloatsBuffer featuresBuf{Vars().TrackerFeatures};
                            Machine().Scatter(TScatterMethod::AggregateFeatures<TMultiTrackerFamily>{},
                                TAggregateFeaturesInfo{*Vars().OriginalQuery, featuresBuf});
                        }
                    }
                };
            };
        };

        template <EExpansionType Expansion>
        using TMultiTrackerFamily = typename TMultiTrackerGroup<Expansion>
            ::TMultiTrackerFamily;
        template <EExpansionType Expansion,
            typename TrackerType,
            EQueryModeType Mode>
        using TMultiTrackerStub = typename TMultiTrackerGroup<Expansion>
            ::template TMultiTrackerStub<TrackerType, Mode>;
    } // MACHINE_PARTS(Core)
} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>
