#include <library/cpp/testing/unittest/registar.h>

#include "user_history.h"

#include <kernel/user_history/proto/user_history.pb.h>

#include <util/generic/algorithm.h>


namespace {
    void FillUserHistory(
        NPersonalization::TUserHistory& hist,
        const bool sorted,
        const int halfSize = 5,
        const TInstant firstTimestamp = TInstant::Seconds(0))
    {
        using namespace NPersonalization;

        for (time_t i = 0; i < halfSize; ++i) {
            hist.AddRecord(TUserHistoryRecord{
                static_cast<time_t>(firstTimestamp.Seconds()) + 2 * i + 1,
                i,
                {
                    TUserHistoryRecord::TDocEmbedding{
                        NPersonalization::NProto::EModels::LogDwelltimeBigrams,
                        ToString(i * 100)
                    }
                }
            });
            hist.AddRecord(TUserHistoryRecord{
                static_cast<time_t>(firstTimestamp.Seconds()) + 2 * i,
                i,
                {
                    TUserHistoryRecord::TDocEmbedding{
                        NPersonalization::NProto::EModels::LogDwelltimeBigrams,
                        ToString(i * 100)
                    }
                }
            });
        }
        if (sorted) {
            hist.SortByTimestamp();
        }
    }

    bool CheckMerge(
        const NPersonalization::TUserHistory& hist1,
        const NPersonalization::TUserHistory& hist2,
        const NPersonalization::TUserHistory& mergedHist)
    {
        bool ans = mergedHist.Size() == hist1.Size() + hist2.Size();
        auto iter1 = hist1.begin();
        auto iter2 = hist2.begin();
        for (const auto& rec : mergedHist) {
            if (iter2 != hist2.end()) {
                ans = ans && *iter2 == rec;
                ++iter2;
            } else if (iter1 != hist1.end()) {
                ans = ans && *iter1 == rec;
                ++iter1;
            }
        }
        return ans;
    }

    NPersonalization::NProto::TUserRecordsDescription GetUserRecordsDescription(
        const ui64 maxRecords,
        const ui64 dwelltimeThreshold,
        const bool lessThanThreshold)
    {
        NPersonalization::NProto::TUserRecordsDescription result;
        result.SetMaxRecords(maxRecords);
        auto options = result.MutableOptions();
        options->SetDwelltimeThreshold(dwelltimeThreshold);
        options->SetLessThanThreshold(lessThanThreshold);
        return result;
    }
}

Y_UNIT_TEST_SUITE(TRecordTestSuite) {
    using namespace NPersonalization;

    Y_UNIT_TEST(TestRecordSaveLoad) {
        const TUserHistoryRecord record{21, 300, {NPersonalization::NProto::LogDwelltimeBigrams, "Hi!"}};
        NProto::TUserHistoryRecord protoRec;
        record.SaveToPB(protoRec);
        // different default model is intentional
        auto loadedRecord = TUserHistoryRecord::ConstructFromPB(protoRec, NPersonalization::NProto::LogDtBigramsAMHardQueriesNoClicks);
        UNIT_ASSERT(loadedRecord == record);
    }
}

Y_UNIT_TEST_SUITE(THistoryTestSuite) {
    using namespace NPersonalization;

    Y_UNIT_TEST(TestHistoryAddRemove) {
        TUserHistory hist;
        const TUserHistoryRecord rec1{42, 600, {NPersonalization::NProto::LogDwelltimeBigrams, "Hello!"}};
        const TUserHistoryRecord rec2{21, 300, {NPersonalization::NProto::LogDwelltimeBigrams, "Hi!"}};
        hist.AddRecord(rec1);
        hist.AddRecord(rec2);
        UNIT_ASSERT(*hist.begin() == rec2);
        UNIT_ASSERT(*hist.rbegin() == rec1);

        UNIT_ASSERT(hist.Size() == 2);
        UNIT_ASSERT(hist.Empty() == false);
        hist.PopRecord();
        UNIT_ASSERT(hist.Size() == 1);

        hist.Clear();
        UNIT_ASSERT(hist.Size() == 0);
        UNIT_ASSERT(hist.Empty() == true);

        TUserHistory histWithFilter(GetUserRecordsDescription(2, 600, false));
        UNIT_ASSERT(histWithFilter.AddRecord(rec1) == true);
        UNIT_ASSERT(histWithFilter.AddRecord(rec2) == false);
        UNIT_ASSERT(*histWithFilter.begin() == rec1);
        UNIT_ASSERT(histWithFilter.Size() == 1);
        histWithFilter.PopRecord();
        UNIT_ASSERT(histWithFilter.Empty() == true);
    }

    Y_UNIT_TEST(TestHistorySort) {
        static const auto compByTimestamp = [] (const TUserHistoryRecord& rhs, const TUserHistoryRecord& lhs) {
            return lhs.Timestamp < rhs.Timestamp;
        };
        static const auto compByDwelltime = [] (const TUserHistoryRecord& lhs, const TUserHistoryRecord& rhs) {
            return lhs.Dwelltime < rhs.Dwelltime;
        };
        TUserHistory hist;
        FillUserHistory(hist, true);
        UNIT_ASSERT(IsSorted(hist.begin(), hist.end(), compByTimestamp));
        hist.SortByDwelltime();
        UNIT_ASSERT(IsSorted(hist.begin(), hist.end(), compByDwelltime));
    }

    Y_UNIT_TEST(TestHistoryMergeAppend) {
        TUserHistory hist1, hist2, appendHist, mergedHist;
        FillUserHistory(hist1, true);
        FillUserHistory(hist2, true, 5, TInstant::Seconds(hist1.begin()->Timestamp + 1));
        appendHist.Append(hist1);
        appendHist.Append(hist2);
        mergedHist = hist1.Merge(hist2);
        UNIT_ASSERT(CheckMerge(hist1, hist2, appendHist) == true);
        UNIT_ASSERT(CheckMerge(hist1, hist2, mergedHist) == true);
        appendHist.Clear();
        bool wrongAppendFailed = false;
        try {
            appendHist.Append(hist2);
            appendHist.Append(hist1);
        } catch (...) {
            wrongAppendFailed = true;
        }
        UNIT_ASSERT(wrongAppendFailed == true);
    }

    Y_UNIT_TEST(TestHistoryDropOld) {
        TUserHistory hist;
        FillUserHistory(hist, true);
        const TInstant lower = TInstant::Seconds(5);
        hist.DropOldRecords(lower);
        bool correctDrop = !hist.Empty();
        for (const auto& rec : hist) {
            correctDrop = correctDrop && rec.Timestamp >= static_cast<time_t>(lower.Seconds());
        }
        UNIT_ASSERT(correctDrop == true);
    }

    Y_UNIT_TEST(TestHistorySaveLoad) {
        TUserHistory hist, loadedHist;
        FillUserHistory(hist, false);
        NProto::TUserHistory protoHist;
        hist.SaveToPB(protoHist);
        loadedHist.LoadFromPB(protoHist, NPersonalization::NProto::EModels::LogDtBigramsAMHardQueriesNoClicks);
        bool correctSaveLoad = true;
        if (hist.Size() == loadedHist.Size()) {
            auto histIter = hist.begin();
            auto loadedHistIter = loadedHist.begin();
            while (histIter != hist.end()) {
                correctSaveLoad = correctSaveLoad &&
                    *histIter == *loadedHistIter;

                ++histIter;
                ++loadedHistIter;
            }
        } else {
            correctSaveLoad = false;
        }
        UNIT_ASSERT(correctSaveLoad == true);
    }
}

Y_UNIT_TEST_SUITE(RecordFitsTestSuite) {
    using namespace NPersonalization;

    Y_UNIT_TEST(TestRecordFitsWithoutThresholdFromOppositeSide) {
         const TUserHistoryRecord record{21, 700, {NPersonalization::NProto::LogDwelltimeBigrams, "Hi!"}};
         NPersonalization::NProto::TUserRecordsDescription userRecordsDescription;
         userRecordsDescription.SetMaxRecords(1);
         auto options = userRecordsDescription.MutableOptions();

         options->SetDwelltimeThreshold(100);
         options->SetLessThanThreshold(false);
         UNIT_ASSERT(NPersonalization::RecordFits(record, userRecordsDescription.GetOptions()));

         options->SetDwelltimeThreshold(800);
         options->SetLessThanThreshold(true);
         UNIT_ASSERT(NPersonalization::RecordFits(record, userRecordsDescription.GetOptions()));
    }

    Y_UNIT_TEST(TestRecordFitsWithThresholdFromOppositeSide) {
         const TUserHistoryRecord record{21, 700, {NPersonalization::NProto::LogDwelltimeBigrams, "Hi!"}};
         NPersonalization::NProto::TUserRecordsDescription userRecordsDescription;
         userRecordsDescription.SetMaxRecords(1);
         auto options = userRecordsDescription.MutableOptions();

         options->SetDwelltimeThreshold(100);
         options->SetDwelltimeThresholdFromOppositeSide(400);
         options->SetLessThanThreshold(false);
         UNIT_ASSERT(!NPersonalization::RecordFits(record, userRecordsDescription.GetOptions()));

         options->SetDwelltimeThresholdFromOppositeSide(800);
         UNIT_ASSERT(NPersonalization::RecordFits(record, userRecordsDescription.GetOptions()));
    }

    Y_UNIT_TEST(TestRecordFitsWithThresholdFromOppositeSideReverted) {
         const TUserHistoryRecord record{21, 700, {NPersonalization::NProto::LogDwelltimeBigrams, "Hi!"}};
         NPersonalization::NProto::TUserRecordsDescription userRecordsDescription;
         userRecordsDescription.SetMaxRecords(1);
         auto options = userRecordsDescription.MutableOptions();

         options->SetDwelltimeThreshold(800);
         options->SetDwelltimeThresholdFromOppositeSide(400);
         options->SetLessThanThreshold(true);
         UNIT_ASSERT(NPersonalization::RecordFits(record, userRecordsDescription.GetOptions()));
    }

    Y_UNIT_TEST(TestRecordFitsWithOnlyStateful) {
         TUserHistoryRecord record{21, 700, {NPersonalization::NProto::LogDwelltimeBigrams, "Hi!"}};
         NPersonalization::NProto::TUserRecordsDescription userRecordsDescription;
         userRecordsDescription.SetMaxRecords(1);
         auto options = userRecordsDescription.MutableOptions();
         options->SetOnlyStateful(true);

         UNIT_ASSERT(!NPersonalization::RecordFits(record, userRecordsDescription.GetOptions()));

         record.StatefulThemes = "pregnancy";
         UNIT_ASSERT(NPersonalization::RecordFits(record, userRecordsDescription.GetOptions()));
    }
}
