#pragma once

#include "parts_base.h"
#include "aggregator_parts.h"

#include <kernel/text_machine/module/module_def.inc>

#include <util/string/builder.h>

namespace NTextMachine {
namespace NCore {
    MACHINE_PARTS(Core) {
        UNIT_FAMILY_INFO_BEGIN(TAggregatorFamily)
        UNIT_FAMILY_INFO_END()

        template <typename TrackerUnitType,
            typename QuerySetType>
        struct TAggregatorGroup {
            UNIT_FAMILY(TAggregatorFamily)

            template <typename AggregatorType>
            UNIT(TAggregatorStub) {
                REQUIRE_MACHINE(Aggregator, AggregatorType);

                using TAggregator = AggregatorType;

                UNIT_FAMILY_STATE(TAggregatorFamily) {
                    TAggregator* Aggregator = nullptr;

                    TFloatsHolder Features;
                    TConstCoordsBuffer Coords;

                    void SaveToJson(NJson::TJsonValue& value) const {
                        if (!!Aggregator) {
                            Aggregator->SaveToJson(value["Aggregator"]);
                        }
                    }
                };

                UNIT_PROCESSOR {
                    UNIT_PROCESSOR_METHODS

                    REQUIRE_UNIT(
                        TCoreUnit,
                        TrackerUnitType
                    );

                    UNIT_PROCESSOR_STATIC {
                        size_t GetNumBaseFeatures() const {
                            return Static<TrackerUnitType>().GetNumFeatures();
                        }
                        void SaveBaseFeatureIds(TFFIdsBuffer& ids) const {
                            Static<TrackerUnitType>().SaveFeatureIds(ids);
                        }
                        size_t GetNumFeatures() const {
                             return TAggregator::GetNumFeatures(GetNumBaseFeatures());
                        }
                        void SaveFeatureIds(TFFIdsBuffer& ids) const {
                            TVector<TFFId> baseIds(GetNumBaseFeatures());
                            TFFIdsBuffer baseIdsBuf = TFFIdsBuffer::FromVector(baseIds, EStorageMode::Empty);
                            SaveBaseFeatureIds(baseIdsBuf);

                            const size_t countBefore = ids.Count();
                            TAggregator::SaveFeatureIds(TConstFFIdsBuffer::FromVector(baseIds), ids);
                            for (size_t i : xrange(countBefore, ids.Count())) {
                                ids[i].Set(Static<TrackerUnitType>().GetExpansion());
                                QuerySetType::UpdateFeatureId(ids[i]);
                            }
                        }
                    };

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
                            MACHINE_LOG("Core::TAggregatorStub", "DisableUnit", TStringBuilder{}
                                << "{Expansion: " << Static<TrackerUnitType>().GetExpansion()
                                << ", QuerySet: {" << QuerySetType::ToString() << "}"
                                << ", UnitId: " << UnitId() << "}");
                        }
                    }
                    void Scatter(TScatterMethod::AggregateFeatures<TrackerUnitType>, const TAggregateFeaturesInfo& info) {
                        if (!IsUnitEnabled()) {
                            return;
                        }

                        Y_ASSERT(!!Vars().Aggregator);
                        const auto& values = info.Query.Values;
                        const bool isOriginal = info.Query.IsOriginal();

                        float maxValue = -1.0f;
                        for (const auto& value : values) {
                            if (isOriginal || QuerySetType::Has(value.Type)) {
                                maxValue = Max(value.Value, maxValue);
                            }
                        }
                        if (maxValue >= 0.0f) {
                            MACHINE_LOG("Core::TAggregatorStub", "AddFeatures", TStringBuilder{}
                                << "{Expansion: " << Static<TrackerUnitType>().GetExpansion()
                                << ", QuerySet: {" << QuerySetType::ToString() << "}"
                                << ", UnitId: " << UnitId()
                                << ", SourceUnitId: " << Proc<TrackerUnitType>().UnitId() << "}");

                            Vars().Aggregator->AddFeatures(info.Features, maxValue);

                            Machine().Scatter(TScatterMethod::NotifyQueryFeatures{},
                                TNotifyQueryFeaturesInfo{
                                    info.Query.ExpansionType,
                                    info.Query.Index,
                                    Static<TrackerUnitType>().GetTrackerFeatureIds(),
                                    info.Features});
                        }
                    }

                    void NewMultiQuery(TMemoryPool& pool, const TMultiQueryInfo&) {
                        Vars().Features.Init(pool, Static().GetNumFeatures());
                        Vars().Aggregator = pool.New<TAggregator>();
                        Vars().Aggregator->Init(pool, Static().GetNumBaseFeatures());
                    }
                    void NewDoc(const TDocInfo&) {
                        Y_ASSERT(!!Vars().Aggregator);
                        Vars().Aggregator->Clear();
                    }
                    void SaveFeatures(const TSaveFeaturesInfo& info) {
                        Vars().Features.Clear();

                        Y_ASSERT(!!Vars().Aggregator);
                        Vars().Aggregator->SaveFeatures(Vars().Features);
                        Y_ASSERT(0  == Vars().Features.Avail());

                        for (auto coordsPtr : Vars().Coords) {
                            Y_ASSERT(coordsPtr);
                            info.Features.Cur().SetValue(Vars().Features[coordsPtr->FeatureIndex]);
                            info.Features.Append(1);
                        }
                    }
                };
            };
        };
    } // MACHINE_PARTS(Core)
} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>
