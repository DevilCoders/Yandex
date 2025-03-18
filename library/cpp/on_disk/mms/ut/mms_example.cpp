#include <library/cpp/on_disk/mms/declare_fields.h>
#include <library/cpp/on_disk/mms/map.h>
#include <library/cpp/on_disk/mms/mapping.h>
#include <library/cpp/on_disk/mms/string.h>
#include <library/cpp/on_disk/mms/unordered_map.h>
#include <library/cpp/on_disk/mms/vector.h>
#include <library/cpp/on_disk/mms/writer.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/typetraits.h>
#include <util/folder/tempdir.h>
#include <util/folder/path.h>
#include <util/stream/file.h>
#include <utility>

namespace {
    template <class P>
    struct TMy {
        int Integer;
        NMms::TStringType<P> String;
        NMms::TMapType<P, NMms::TStringType<P>, int> Map;

        // Expose struct's fields to mms
        MMS_DECLARE_FIELDS(Integer, String, Map);
    };

    struct TSomeFeatures {
        float CTR;
        int Rating;
    };
    // No need to use MMS_DECLARE_FIELDS for POD type.
    // Make sure that the struct is well-aligned and does not have extra padding bytes:
    // they cause 'read from uninitialized memory' errors under msan and valgrind.

    template <class P>
    using TIndex = NMms::TUnorderedMap<P, std::pair<NMms::TStringType<P>, int>, NMms::TMapType<P, ui32, TSomeFeatures>>;
    // something like (query, region) -> (docId -> features)

    using TIndexStandalone = TIndex<NMms::TStandalone>;
}

Y_DECLARE_PODTYPE(TSomeFeatures);

Y_UNIT_TEST_SUITE(TMmsExample) {
    Y_UNIT_TEST(Basics1) {
        TTempDir tempDir;
        const TString fileName = JoinFsPaths(tempDir(), "file.mms");

        {
            // Populate the struct
            TMy<NMms::TStandalone> my;
            my.Integer = 22;
            my.String = "a string";
            my.Map.insert(std::make_pair("ten", 10));
            my.Map["eleven"] = 11;
            my.Map["twelve"] = 12;

            // Serialize it
            TFileOutput out(fileName);
            NMms::Write(out, my);
        }
        {
            // mmap() serialized data
            NMms::TMapping<TMy<NMms::TMmapped>> obj(fileName);

            // Use the data
            UNIT_ASSERT_EQUAL(obj->Integer, 22);
            UNIT_ASSERT_EQUAL(obj->String, "a string");
            UNIT_ASSERT_EQUAL(obj->Map.size(), 3);
            UNIT_ASSERT_EQUAL(obj->Map["twelve"], 12);
            UNIT_ASSERT_EQUAL(obj->Map[TString("ten")], 10);
            UNIT_ASSERT_EQUAL(obj->Map[TStringBuf("eleven")], 11);
            UNIT_ASSERT_EXCEPTION(obj->Map["unknown"], std::exception);
        }
    }

    Y_UNIT_TEST(Basics2) {
        TTempDir tempDir;
        const TString fileName = JoinFsPaths(tempDir(), "indexuser.mms");

        {
            TIndexStandalone indexSrc;
            auto& itemsFirst = indexSrc[std::make_pair("query", 213)];
            itemsFirst[1] = TSomeFeatures{0.1f, 10};
            itemsFirst[2] = TSomeFeatures{0.8f, 9};
            itemsFirst[100500] = TSomeFeatures{0.2f, 2};

            auto& itemsSecond = indexSrc[std::make_pair("query", 157)];
            itemsSecond[1] = TSomeFeatures{0.2f, 0};
            itemsSecond[0] = TSomeFeatures{0.0f, 8};

            TFileOutput out(fileName);
            NMms::Write(out, indexSrc);
        }
        {
            NMms::TMapping<TIndexStandalone> indexDst(fileName);
            // NMms::TMapping<TIndex<NMms::TMmapped>> also works

            const auto& p = std::make_pair("query", 213);
            UNIT_ASSERT_EQUAL(indexDst->count(p), 1);
            const auto& row = indexDst->at(p);

            const TSomeFeatures& doc2Features = row[2];
            const float eps = 0.000001f;
            UNIT_ASSERT_DOUBLES_EQUAL(doc2Features.CTR, 0.8f, eps);
            UNIT_ASSERT_VALUES_EQUAL(doc2Features.Rating, 9);

            UNIT_ASSERT_EQUAL(row.count(0), 0);
            UNIT_ASSERT_EQUAL(row.count(100500), 1);
            UNIT_ASSERT_EQUAL(row[100500].Rating, 2);
            UNIT_ASSERT_DOUBLES_EQUAL(row[1].CTR, 0.1f, eps);

            int numQueries = 0;
            int numDocs = 0;
            int sumRatings = 0;

            // use range-based for
            for (const auto& p2 : *indexDst) {
                numQueries++;
                for (const auto& q : p2.second) {
                    numDocs++;
                    sumRatings += q.second.Rating;
                }
            }

            UNIT_ASSERT_EQUAL(numQueries, 2);
            UNIT_ASSERT_EQUAL(numDocs, 3 + 2);
            UNIT_ASSERT_EQUAL(sumRatings, 10 + 9 + 2 + 0 + 8);
        }
    }
}
