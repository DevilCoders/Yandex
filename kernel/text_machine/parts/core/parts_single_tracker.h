#pragma once

#include "parts_base.h"
#include "aggregator_parts.h"

#include <kernel/text_machine/parts/tracker/motor.h>

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    MACHINE_PARTS(Core) {
        UNIT_FAMILY_INFO_BEGIN(TBasicExpansionSingleTrackerFamily, EExpansionType expansion, EQueryModeType mode)
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
        struct TBasicExpansionSingleTrackerGroup {
            UNIT_FAMILY(TBasicExpansionSingleTrackerFamily)

            template <typename TrackerType,
                EQueryModeType Mode = TQueryMode::DontUseOriginal>
            UNIT(TBasicExpansionSingleTrackerStub) {
                REQUIRE_MACHINE(Tracker, TrackerType);

                using TTracker = TrackerType;

                UNIT_FAMILY_STATE(TBasicExpansionSingleTrackerFamily) {
                    TTracker* Tracker = nullptr;
                    const TQuery* Query = nullptr;

                    TConstCoordsBuffer Coords;
                    TFloatsHolder Features;

                    void SaveToJson(NJson::TJsonValue& value) const {
                        if (Tracker) {
                            Tracker->SaveToJson(value["Tracker"]);
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
                        void SaveFeatureIds(TFFIdsBuffer& ids) const {
                             const auto& trackerIds = TTracker::GetFeatureIds();
                             for (size_t i : xrange(trackerIds.size())) {
                                 ids.Emplace(trackerIds[i], ExpansionForGroup);
                             }
                        }
                    };

                    static void ScatterStatic(TScatterMethod::CollectExpansions, const TStaticExpansionsInfo& info) {
                        info.Expansions.insert(ExpansionForGroup);
                    }
                    static void ScatterStatic(TScatterMethod::CollectStreams, const TStaticStreamsInfo& info) {
                        TTracker::ScatterStatic(TScatterMethod::CollectStreams(), info);
                    }
                    static void ScatterStatic(TScatterMethod::CollectFeatures, const TStaticFeaturesInfo& info) {
                        auto guard = info.Features.Guard(UnitId());

                        auto& ids = info.Features.Next().Ids;
                        ids.resize(Static().GetNumFeatures());
                        auto idsBuf = TFFIdsBuffer::FromVector(ids, EStorageMode::Empty);
                        Static().SaveFeatureIds(idsBuf);
                    }
                    static void ScatterStatic(TScatterMethod::CollectValueRefs, const TStaticValueRefsInfo& info) {
                        auto guard = info.ValueRefs.Guard(UnitId());

                        MACHINE_LOG("Core::TBasicExpansionSingleTrackerStub", "CollectValueRefs", TStringBuilder{}
                            << "{Expansion: " << ExpansionForGroup
                            << ", UnitId: " << UnitId() << "}");

                        TTracker::ScatterStatic(TScatterMethod::CollectValueRefs(), info);
                    }
                    void Scatter(TScatterMethod::BindValueRefs, const TBindValuesInfo& info) {
                        auto guard = info.ValueRefs.Guard(UnitId());

                        if (!IsUnitEnabled() || !Vars().Query) {
                            return;
                        }
                        Y_ASSERT(Vars().Tracker);

                        MACHINE_LOG("Core::TBasicExpansionSingleTrackerStub", "BindValues", TStringBuilder{}
                            << "{Expansion: " << ExpansionForGroup
                            << ", UnitId: " << UnitId() << "}");

                        Vars().Tracker->Scatter(TScatterMethod::BindValueRefs(), info);
                    }
                    void Scatter(TScatterMethod::RegisterDataExtractors, TSimpleExtractorsRegistry* registry) {
                        if (!IsUnitEnabled() || !Vars().Query) {
                            return;
                        }
                        Y_ASSERT(Vars().Tracker);

                        MACHINE_LOG("Core::TBasicExpansionSingleTrackerStub", "RegisterDataExtractors", TStringBuilder{}
                            << "{Expansion: " << ExpansionForGroup
                            << ", UnitId: " << UnitId() << "}");

                        Vars().Tracker->Scatter(TScatterMethod::RegisterDataExtractors(), registry);
                    }
                    void Scatter(TScatterMethod::ConfigureFeatures, const TConfigureFeaturesInfo& info) {
                        auto guard = info.FeaturesConfig.Guard(UnitId());

                        Vars().Coords = info.FeaturesConfig.Next().Coords;

                        if (Vars().Coords.Count() == 0) {
                            SetUnitEnabled(false);

                            MACHINE_LOG("Core::TBasicExpansionSingleTrackerStub", "DisableUnit", TStringBuilder{}
                                << "{Expansion: " << ExpansionForGroup
                                << ", UnitId: " << UnitId() << "}");
                        }
                    }

                    void NewMultiQuery(TMemoryPool& pool, const TMultiQueryInfo& info) {
                        Vars().Features.Init(pool, Static().GetNumFeatures());

                        auto& queriesHelper = info.QueriesHelper;
                        Y_ASSERT(queriesHelper.GetNumQueries(ExpansionForGroup) <= 1);

                        EExpansionType workingType = ExpansionForGroup;
                        if (queriesHelper.GetNumQueries(ExpansionForGroup) > 0) {
                            Vars().Query = &queriesHelper.GetQuery(ExpansionForGroup, 0);
                        } else if (TQueryMode::UseOriginal == Mode) {
                            Vars().Query = &queriesHelper.GetOriginalQuery();
                            workingType = TExpansion::OriginalRequest;
                        } else {
                            Vars().Query = nullptr;
                        }

                        if (!!Vars().Query) {
                            Vars().Tracker = pool.New<TTracker>();
                            Vars().Tracker->NewQuery(pool, Proc<TCoreUnit>().GetSharedState(),
                                *Vars().Query,
                                queriesHelper.GetMainWeights(workingType, 0),
                                queriesHelper.GetExactWeights(workingType, 0));
                        }
                    }
                    void NewDoc(const TDocInfo&) {
                        if (!Vars().Query) {
                            return;
                        }
                        Y_ASSERT(Vars().Tracker);
                        Vars().Tracker->NewDoc();
                    }
                    template <EExpansionType Expansion, EStreamType Stream>
                    Y_FORCE_INLINE void AddStreamHitBasic(const THitInfo& info) {
                        if (!Vars().Query) {
                            return;
                        }
                        if (TQueryMode::UseOriginal == Mode
                            && Expansion != Vars().Query->ExpansionType)
                        {
                            return; // skip original request hits, when expansions is available
                        }
                        if (info.LocalQueryId > 0) {
                            Y_ASSERT(false);
                            return; // skip hits from queries after the 1st
                        }

                        Y_ASSERT(Vars().Tracker);
                        Y_ASSERT(TQueryMode::UseOriginal == Mode || ExpansionForGroup == Vars().Query->ExpansionType);
                        Y_ASSERT(TExpansion::OriginalRequest == Vars().Query->ExpansionType || ExpansionForGroup == Vars().Query->ExpansionType);
                        Vars().Tracker->template AddHit<Stream>(info.Hit);
                    }
                    void FinishDoc(const TDocInfo&) {
                        if (!Vars().Query) {
                            return;
                        }
                        Y_ASSERT(Vars().Tracker);
                        Vars().Tracker->FinishDoc();
                    }
                    void SaveFeatures(const TSaveFeaturesInfo& info) {
                        Vars().Features.Clear();

                        if (Vars().Query) {
                            Y_ASSERT(Vars().Tracker);
                            Vars().Tracker->CalcFeatures(Vars().Features);
                            Y_ASSERT(0 == Vars().Features.Avail());

                            for (auto coordsPtr : Vars().Coords) {
                                Y_ASSERT(coordsPtr);
                                info.Features.Cur().SetValue(Vars().Features[coordsPtr->FeatureIndex]);
                                info.Features.Append(1);
                            }
                        } else {
                            for (auto& feature : info.Features.Append(Vars().Coords.Count())) {
                                feature.SetValue(0.0f);
                            }
                        }
                    }
                };
            };
        };

        template <EExpansionType Expansion>
        using TBasicExpansionSingleTrackerFamily = typename TBasicExpansionSingleTrackerGroup<Expansion>
            ::TBasicExpansionSingleTrackerFamily;
        template <EExpansionType Expansion,
            typename TrackerType>
        using TBasicExpansionSingleTrackerStub = typename TBasicExpansionSingleTrackerGroup<Expansion>
            ::template TBasicExpansionSingleTrackerStub<TrackerType>;
    } // MACHINE_PARTS(Core)
} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>
