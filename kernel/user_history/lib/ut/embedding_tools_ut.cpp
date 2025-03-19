#include "embedding_tools.h"

#include <kernel/user_history/user_history.h>
#include <kernel/user_history/proto/user_history.pb.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NPersonalization;

Y_UNIT_TEST_SUITE(Conversions) {
    Y_UNIT_TEST(WellDefinedConversions) {
        struct TCanonicalConversion {
            NNeuralNetApplier::EDssmModel DssmModel;
            NProto::EModels Model;
            NDssmApplier::EDssmModelType DssmModelType;
        };
        const TVector<TCanonicalConversion> canonicalConversions = {
            {
                NNeuralNetApplier::EDssmModel::LogDwellTimeBigrams,
                NProto::EModels::LogDwelltimeBigrams,
                NDssmApplier::EDssmModelType::LogDwellTimeBigrams
            },
            {
                NNeuralNetApplier::EDssmModel::FpsSpylogAggregatedQueryPart,
                NProto::EModels::FpsSpylogAggregatedQueryPart,
                NDssmApplier::EDssmModelType::FpsSpylogAggregatedQueryPart
            },
            {
                NNeuralNetApplier::EDssmModel::ReformulationsLongestClickLogDt,
                NProto::EModels::ReformulationsLongestClickLogDt,
                NDssmApplier::EDssmModelType::ReformulationsLongestClickLogDt
            }
        };
        for (const auto& conversion: canonicalConversions) {
            {
                const auto dssmModel = ToDssmModel(conversion.Model);
                UNIT_ASSERT_VALUES_EQUAL(dssmModel, conversion.DssmModel);
            }
            {
                const auto protoEnum = ToProtoEnum(conversion.DssmModelType);
                UNIT_ASSERT(protoEnum.Defined());
                UNIT_ASSERT_VALUES_EQUAL(static_cast<int>(*protoEnum), static_cast<int>(conversion.Model));
            }
            {
                const auto dssmModelType = ToDssmModelType(conversion.Model);
                UNIT_ASSERT(dssmModelType.Defined());
                UNIT_ASSERT_VALUES_EQUAL(*dssmModelType, conversion.DssmModelType);
            }
        }
    }

    Y_UNIT_TEST(TestCompareDescription) {
        NProto::TUserRecordsDescription descriptionNoSort;
        {
            descriptionNoSort.SetMaxRecords(10);
            auto opts = descriptionNoSort.MutableOptions();
            opts->SetModel(NProto::LogDwelltimeBigrams);
            opts->SetEmbeddingType(NProto::TitleEmbedding);
            opts->SetDwelltimeThreshold(120);
            opts->SetLessThanThreshold(false);
        }
        NProto::TUserRecordsDescription descriptionWithSort = descriptionNoSort;
        descriptionWithSort.SetSortOrderOnMirror(NProto::SOM_SORT_BY_DWELLTIME);
        UNIT_ASSERT(CompareUserRecordsDescriptions(descriptionNoSort, descriptionWithSort) == true);
        NProto::TUserRecordsDescription descriptionNotEqual = descriptionWithSort;
        descriptionNotEqual.SetMaxRecords(1000);
        UNIT_ASSERT(CompareUserRecordsDescriptions(descriptionNoSort, descriptionNotEqual) == false);
    }

    Y_UNIT_TEST(TestLoadWithNoSortField) {
        NProto::TUserRecordsDescription descriptionNoSort;
        {
            descriptionNoSort.SetMaxRecords(10);
            auto opts = descriptionNoSort.MutableOptions();
            opts->SetModel(NProto::LogDwelltimeBigrams);
            opts->SetEmbeddingType(NProto::TitleEmbedding);
            opts->SetDwelltimeThreshold(120);
            opts->SetLessThanThreshold(false);
        }
        TUserHistory histNoSort(descriptionNoSort);

        auto descriptionWithSort = descriptionNoSort;
        descriptionWithSort.SetSortOrderOnMirror(NProto::SOM_SORT_BY_TIMESTAMP);
        TUserHistory histWithSort(descriptionWithSort);

        TUserHistoryRecord rec (10, 200, {NProto::LogDwelltimeBigrams, "test string"});
        histNoSort.AddRecord(rec);

        NProto::TFilteredUserRecords protoHistoryNoSort;
        histNoSort.SaveToMirror(protoHistoryNoSort);

        UNIT_ASSERT(protoHistoryNoSort.RecordsSize() == 1);
        histWithSort.LoadFromPB(protoHistoryNoSort, NProto::LogDwelltimeBigrams);
        UNIT_ASSERT(histWithSort.GetDescription()->GetSortOrderOnMirror() == NProto::SOM_SORT_BY_TIMESTAMP);
        UNIT_ASSERT(histWithSort.Size() == 1);

        histWithSort.Clear();
        histWithSort.LoadFromPBWithFilter(protoHistoryNoSort, NProto::LogDwelltimeBigrams);
        UNIT_ASSERT(histWithSort.GetDescription()->HasSortOrderOnMirror() == false);
        UNIT_ASSERT(histWithSort.Size() == 1);
    }
}
