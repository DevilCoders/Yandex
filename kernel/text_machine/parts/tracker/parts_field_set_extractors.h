#pragma once

#include "parts_base.h"
#include "positionless_units.h"
#include "annotation_units.h"

#include <kernel/text_machine/parts/accumulators/word_accumulator.h>

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    MACHINE_PARTS(Tracker) {

        UNIT_INFO_BEGIN(TFieldSetForDssmCountersForTextFieldSetWithSSHardsLongClicksPredictorVersFeb19Unit)
        UNIT_INFO_END()

        UNIT(TFieldSetForDssmCountersForTextFieldSetWithSSHardsLongClicksPredictorVersFeb19Unit)
        {
            using TStreamwiseState = NAnnotationAccumulatorParts::TNeuroTextCounterLevel1Unit::TState;
            UNIT_STATE {
                const TStreamwiseState* UrlState = nullptr;
                const TStreamwiseState* TitleState = nullptr;
                const TStreamwiseState* BodyState = nullptr;
                const TQuery* Query = nullptr;

                void SaveToJson(NJson::TJsonValue&) const {
                }

                TForDssmCountersForTextFieldSetWithSSHardsLongClicksPredictorVersFeb19 DoExtractData() const;
            };

            UNIT_PROCESSOR
                , public IDataExtractorImpl<EDataExtractorType::DssmCountersForTextFieldSetWithSSHardsLongClicksPredictorVersFeb19>
            {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void NewQuery(TMemoryPool&, const TQueryInfo& info) {
                    Query = &info.Query;
                    UrlState = &Vars<TUrlAnnotationFamily>().AnnotationAccumulator.GetNeuroTextCounterLevel1State();
                    TitleState = &Vars<TTitleAnnotationFamily>().AnnotationAccumulator.GetNeuroTextCounterLevel1State();
                    BodyState = &Vars<TBodyAnnotationFamily>().AnnotationAccumulator.GetNeuroTextCounterLevel1State();
                }
            public:
                void Scatter(TScatterMethod::RegisterDataExtractors, TSimpleExtractorsRegistry* registry) {
                    (*registry)[EDataExtractorType::DssmCountersForTextFieldSetWithSSHardsLongClicksPredictorVersFeb19]
                        .push_back(this);
                }

                TQueryStats GetQueryStats() final {
                    return TState::DoExtractData();
                }
            };
        };


        UNIT_INFO_BEGIN(TFieldSetForDssmSSHardWordWeightsFeaturesUnit)
        UNIT_INFO_END()

        UNIT(TFieldSetForDssmSSHardWordWeightsFeaturesUnit)
        {
            using TStreamwiseState = NPositionlessProxyParts::TWeightedUnit::TState;
            UNIT_STATE {
                const TStreamwiseState* TitleState = nullptr;
                const TStreamwiseState* QueryDwellTimeState = nullptr;
                const TQuery* Query = nullptr;

                void SaveToJson(NJson::TJsonValue&) const {
                }

                TCountersForDssmSSHardWordWeights DoExtractData() const;
            };

            UNIT_PROCESSOR
                , public IDataExtractorImpl<EDataExtractorType::CountersForDssmSSHardWordWeights>
            {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void NewQuery(TMemoryPool&, const TQueryInfo& info) {
                    Query = &info.Query;
                    TitleState = &Vars<TTitlePositionlessFamily>().PositionlessProxy;
                    QueryDwellTimeState = &Vars<TQueryDwellTimePositionlessFamily>().PositionlessProxy;
                }
            public:
                void Scatter(TScatterMethod::RegisterDataExtractors, TSimpleExtractorsRegistry* registry) {
                    (*registry)[EDataExtractorType::CountersForDssmSSHardWordWeights]
                        .push_back(this);
                }

                TQueryStats GetQueryWordsOccuranceStats() final {
                    return TState::DoExtractData();
                }
            };
        };

        UNIT_INFO_BEGIN(TFieldSetForFullSplitBertCountersUnit)
        UNIT_INFO_END()

        UNIT(TFieldSetForFullSplitBertCountersUnit)
        {
            UNIT_STATE {
                const TQuery* Query = nullptr;
                TAccumulatorsByFloatValue FieldsAcc;

                void SaveToJson(NJson::TJsonValue&) const {
                }

                TFullSplitBertCounters DoExtractData() const;
            };

            UNIT_PROCESSOR
                , public IDataExtractorImpl<EDataExtractorType::FullSplitBertCounters>
            {
                UNIT_PROCESSOR_METHODS

                Y_FORCE_INLINE void NewQuery(TMemoryPool& pool, const TQueryInfo& info) {
                    Query = &info.Query;
                    FieldsAcc.Init(pool, info.Query.Words.size());
                }

                Y_FORCE_INLINE void NewDoc(const TDocInfo&) {
                    FieldsAcc.MemSetZeroes();
                }

                Y_FORCE_INLINE void FinishDoc(const TDocInfo&) {
                    FieldsAcc.AddFrom(
                        Vars<TBodyPositionlessFamily>().PositionlessProxy.ExactAcc.AsSeq4f()
                    );
                }

            public:
                void Scatter(TScatterMethod::RegisterDataExtractors, TSimpleExtractorsRegistry* registry) {
                    (*registry)[EDataExtractorType::FullSplitBertCounters]
                        .push_back(this);
                }

                TFullSplitBertCounters GetCounters() final {
                    return TState::DoExtractData();
                }
            };
        };
    } // MACHINE_PARTS(Tracker)
} // NCore
} // NTextMachine

#include <kernel/text_machine/module/module_undef.inc>
