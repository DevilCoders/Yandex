#include "merge.h"

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/protobuf/util/pb_io.h>

using namespace NPersonalization;

Y_UNIT_TEST_SUITE(Merge) {
    Y_UNIT_TEST(Patch) {
        const TString desciptionText = "MaxRecords: 256\n\
Options {\n\
    Model: LogDwelltimeBigrams\n\
    DwelltimeThreshold: 120\n\
    LessThanThreshold: false\n\
    EmbeddingType: TitleEmbedding\n\
}\n\
SortOrderOnMirror: SOM_SORT_BY_REQUEST_TIMESTAMP\n\
StoreOptions {\n\
    StoreEmbedding: false\n\
    StoreUrl: true\n\
    StoreTitle: true\n\
    StoreOmniTitle: true\n\
    StorePosition: true\n\
    StoreRequestTimestamp: true\n\
}";

        NProto::TUserRecordsDescription userRecordsDescription;
        {
            TStringInput si(desciptionText);
            ParseFromTextFormat(si, userRecordsDescription);
        }

        NProto::TUserHistory base;
        {
            auto filteredRecords = base.AddFilteredRecords();
            filteredRecords->MutableDescription()->CopyFrom(userRecordsDescription);

            auto record1 = filteredRecords->AddRecords();
            record1->SetRequestTimestamp(30);

            auto record2 = filteredRecords->AddRecords();
            record2->SetRequestTimestamp(40);

            auto record3 = filteredRecords->AddRecords();
            record3->SetRequestTimestamp(50);
        }

        NProto::TUserHistory patch;
        {
            auto filteredRecords = patch.AddFilteredRecords();
            filteredRecords->MutableDescription()->CopyFrom(userRecordsDescription);

            auto record1 = filteredRecords->AddRecords();
            record1->SetRequestTimestamp(45);
        }

        {
            const auto patched = Patch(base, patch, NPersonalization::NProto::TUserHistoryPatch::PreferDataFromPatch);

            UNIT_ASSERT_EQUAL(patched.FilteredRecordsSize(), 1);
            const auto gotRecords = patched.GetFilteredRecords(0).RecordsSize();

            UNIT_ASSERT_EQUAL(gotRecords, 2);
        }
        {
            const auto patched = Patch(base, patch);

            UNIT_ASSERT_EQUAL(patched.FilteredRecordsSize(), 1);
            const auto gotRecords = patched.GetFilteredRecords(0).RecordsSize();

            UNIT_ASSERT_EQUAL(gotRecords, 3);
        }

    }
    Y_UNIT_TEST(EmptyBase) {
        const TString desciptionText = "MaxRecords: 256\n\
Options {\n\
    Model: LogDwelltimeBigrams\n\
    DwelltimeThreshold: 120\n\
    LessThanThreshold: false\n\
    EmbeddingType: TitleEmbedding\n\
}\n\
SortOrderOnMirror: SOM_SORT_BY_REQUEST_TIMESTAMP\n\
StoreOptions {\n\
    StoreEmbedding: false\n\
    StoreUrl: true\n\
    StoreTitle: true\n\
    StoreOmniTitle: true\n\
    StorePosition: true\n\
    StoreRequestTimestamp: true\n\
}";

        NProto::TUserRecordsDescription userRecordsDescription;
        {
            TStringInput si(desciptionText);
            ParseFromTextFormat(si, userRecordsDescription);
        }

        NProto::TUserHistory base;

        NProto::TUserHistory patch;
        {
            auto filteredRecords = patch.AddFilteredRecords();
            filteredRecords->MutableDescription()->CopyFrom(userRecordsDescription);

            auto record1 = filteredRecords->AddRecords();
            record1->SetRequestTimestamp(45);
        }

        {
            const auto patched = Patch(base, patch, NPersonalization::NProto::TUserHistoryPatch::PreferDataFromPatch);

            UNIT_ASSERT_EQUAL(patched.FilteredRecordsSize(), 1);
            const auto gotRecords = patched.GetFilteredRecords(0).RecordsSize();

            UNIT_ASSERT_EQUAL(gotRecords, 1);
        }
        {
            const auto patched = Patch(base, patch);

            UNIT_ASSERT_EQUAL(patched.FilteredRecordsSize(), 1);
            const auto gotRecords = patched.GetFilteredRecords(0).RecordsSize();

            UNIT_ASSERT_EQUAL(gotRecords, 1);
        }
    }

    Y_UNIT_TEST(BaseContainZeroFilteredRecords) {
        const TString desciptionText = "MaxRecords: 256\n\
Options {\n\
    Model: LogDwelltimeBigrams\n\
    DwelltimeThreshold: 120\n\
    LessThanThreshold: false\n\
    EmbeddingType: TitleEmbedding\n\
}\n\
SortOrderOnMirror: SOM_SORT_BY_REQUEST_TIMESTAMP\n\
StoreOptions {\n\
    StoreEmbedding: false\n\
    StoreUrl: true\n\
    StoreTitle: true\n\
    StoreOmniTitle: true\n\
    StorePosition: true\n\
    StoreRequestTimestamp: true\n\
}";

        NProto::TUserRecordsDescription userRecordsDescription;
        {
            TStringInput si(desciptionText);
            ParseFromTextFormat(si, userRecordsDescription);
        }

        NProto::TUserHistory base;
        {
            auto filteredRecords = base.AddFilteredRecords();
            filteredRecords->MutableDescription()->CopyFrom(userRecordsDescription);
        }

        NProto::TUserHistory patch;
        {
            auto filteredRecords = patch.AddFilteredRecords();
            filteredRecords->MutableDescription()->CopyFrom(userRecordsDescription);

            auto record1 = filteredRecords->AddRecords();
            record1->SetRequestTimestamp(45);
        }

        {
            const auto patched = Patch(base, patch, NPersonalization::NProto::TUserHistoryPatch::PreferDataFromPatch);

            UNIT_ASSERT_EQUAL(patched.FilteredRecordsSize(), 1);
            const auto gotRecords = patched.GetFilteredRecords(0).RecordsSize();

            UNIT_ASSERT_EQUAL(gotRecords, 1);
        }
        {
            const auto patched = Patch(base, patch);

            UNIT_ASSERT_EQUAL(patched.FilteredRecordsSize(), 1);
            const auto gotRecords = patched.GetFilteredRecords(0).RecordsSize();

            UNIT_ASSERT_EQUAL(gotRecords, 1);
        }
    }
    Y_UNIT_TEST(PatchLogicFromToString) {
        const auto patchLogic = NPersonalization::NProto::TUserHistoryPatch::PreferDataFromPatch;
        const TString patchLogicAsString = ToString(patchLogic);
        UNIT_ASSERT_EQUAL(patchLogicAsString, "TUserHistoryPatch_EUserHistoryPatchLogic_PreferDataFromPatch");
        UNIT_ASSERT_EQUAL(FromString<NPersonalization::NProto::TUserHistoryPatch::EUserHistoryPatchLogic>(patchLogicAsString), patchLogic);
    }
}
