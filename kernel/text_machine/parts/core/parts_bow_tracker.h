#pragma once

#include "parts_base.h"

#include <kernel/text_machine/parts/tracker/motor.h>
#include <kernel/text_machine/parts/common/bag_of_words.h>
#include <kernel/text_machine/parts/common/queries_helper.h>

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    MACHINE_PARTS(Core) {
        UNIT_FAMILY_INFO_BEGIN(TBagOfWordsTrackerFamily, EExpansionType expansion, EQueryModeType mode)
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
        struct TBagOfWordsTrackerGroup {
            UNIT_FAMILY(TBagOfWordsTrackerFamily)

            template <typename TrackerType, EQueryModeType Mode>
            UNIT(TBagOfWordsTrackerStub) {
                REQUIRE_MACHINE(Tracker, TrackerType);

                using TTracker = TrackerType;

                UNIT_FAMILY_STATE(TBagOfWordsTrackerFamily) {
                    TBagOfWords* BagOfWords = nullptr;
                    TTracker* BagTracker = nullptr;

                    TFloatsHolder Features;
                    TConstCoordsBuffer Coords;

                    void SaveToJson(NJson::TJsonValue& value) const {
                        if (BagOfWords) {
                            BagOfWords->SaveToJson(value["BagOfWords"]);
                        }
                        if (BagTracker) {
                            BagTracker->SaveToJson(value["BagTracker"]);
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
                                 ids.Emplace(trackerIds[i], ExpansionForGroup, TTrackerPrefix::BagOfWords);
                             }
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
                    static void ScatterStatic(TScatterMethod::CollectFeatures, const TStaticFeaturesInfo& info) {
                        auto guard = info.Features.Guard(UnitId());

                        auto& ids = info.Features.Next().Ids;
                        ids.resize(Static().GetNumFeatures());
                        auto idsBuf = TFFIdsBuffer::FromVector(ids, EStorageMode::Empty);
                        Static().SaveFeatureIds(idsBuf);
                    }
                    void Scatter(TScatterMethod::ConfigureFeatures, const TConfigureFeaturesInfo& info) {
                        auto guard = info.FeaturesConfig.Guard(UnitId());

                        Vars().Coords = info.FeaturesConfig.Next().Coords;

                        if (Vars().Coords.Count() == 0) {
                            SetUnitEnabled(false);
                            MACHINE_LOG("Core::TBagOfWordsTrackerStub", "DisableUnit", TStringBuilder{}
                                << "{Expansion: " << ExpansionForGroup
                                << ", UnitId: " << UnitId() << "}");

                            return;
                        }
                    }

                    void NewMultiQuery(TMemoryPool& pool, const TMultiQueryInfo& info) {
                        Vars().Features.Init(pool, Static().GetNumFeatures());

                        auto& queriesHelper = info.QueriesHelper;
                        Vars().BagOfWords = queriesHelper.CreateBagOfWords<TAnyQuery>(pool,
                            ExpansionForGroup,
                            TQueryMode::UseOriginal == Mode);

                        Vars().BagTracker = pool.New<TTracker>();
                        Vars().BagTracker->NewMultiQuery(pool, Proc<TCoreUnit>().GetSharedState(),
                            queriesHelper, Vars().BagOfWords);
                    }
                    void NewDoc(const TDocInfo&) {
                        Vars().BagTracker->NewDoc();
                    }
                    template <EExpansionType Expansion, EStreamType Stream>
                    Y_FORCE_INLINE void AddStreamHitBasic(const THitInfo& info) {
                        Y_ASSERT(ExpansionForGroup == Expansion
                            || (TExpansion::OriginalRequest == Expansion
                                && TQueryMode::UseOriginal == Mode));

                        Vars().BagTracker->template AddHit<Stream>(info.Hit);
                    }
                    void FinishDoc(const TDocInfo&) {
                        Vars().BagTracker->FinishDoc();
                    }
                    void SaveFeatures(const TSaveFeaturesInfo& info) {
                        Vars().Features.Clear();

                        Y_ASSERT(Vars().BagTracker);
                        Vars().BagTracker->CalcFeatures(Vars().Features);
                        Y_ASSERT(0 == Vars().Features.Avail());

                        for (auto coordsPtr : Vars().Coords) {
                            Y_ASSERT(coordsPtr);
                            info.Features.Cur().SetValue(Vars().Features[coordsPtr->FeatureIndex]);
                            info.Features.Append(1);
                        }
                    }
                };
            };
        };

        template <EExpansionType Expansion>
        using TBagOfWordsTrackerFamily = typename TBagOfWordsTrackerGroup<Expansion>
            ::TBagOfWordsTrackerFamily;
        template <EExpansionType Expansion, typename Tracker, EQueryModeType Mode>
        using TBagOfWordsTrackerStub = typename TBagOfWordsTrackerGroup<Expansion>
            ::template TBagOfWordsTrackerStub<Tracker, Mode>;
    } // MACHINE_PARTS(Core)
} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>
