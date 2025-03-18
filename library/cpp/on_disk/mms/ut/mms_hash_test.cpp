#include "write_to_blob.h"

#include <library/cpp/on_disk/mms/cast.h>
#include <library/cpp/on_disk/mms/copy.h>
#include <library/cpp/on_disk/mms/string.h>
#include <library/cpp/on_disk/mms/unordered_map.h>
#include <library/cpp/on_disk/mms/unordered_set.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {
    template <typename THashMapType>
    bool AreHashMapsEqual(const THashMapType& first, const THashMapType& second) {
        if (first.size() != second.size()) {
            return false;
        }
        for (auto firstIter = first.begin(); firstIter != first.end(); ++firstIter) {
            const auto secondIter = second.find(firstIter->first);
            if (secondIter == second.end()) {
                return false;
            }
            if (firstIter->second != secondIter->second) {
                return false;
            }
        }
        return true;
    }

    template <typename THashSetType>
    bool AreHashSetsEqual(const THashSetType& first, const THashSetType& second) {
        if (first.size() != second.size()) {
            return false;
        }
        for (auto firstIter = first.begin(); firstIter != first.end(); ++firstIter) {
            const auto secondIter = second.find(*firstIter);
            if (secondIter == second.end()) {
                return false;
            }
        }
        return true;
    }
}

Y_UNIT_TEST_SUITE(TMmsHashTest) {
    Y_UNIT_TEST(TestHashDefaultDefault) {
        { //map
            typedef NMms::TUnorderedMap<NMms::TMmapped, int, int> THashMapType;

            THashMapType mm;
            UNIT_ASSERT_VALUES_EQUAL(mm.size(), 0);
            UNIT_ASSERT(mm.empty());
            for (unsigned i = 0; i < 100; ++i) {
                UNIT_ASSERT(mm.find(i) == mm.end());
                UNIT_ASSERT_EXCEPTION(mm[i], std::out_of_range);
            }
        }

        { //set
            typedef NMms::TUnorderedSet<NMms::TMmapped, int> THashSetType;

            THashSetType ms;
            UNIT_ASSERT_VALUES_EQUAL(ms.size(), 0);
            UNIT_ASSERT(ms.empty());
            for (unsigned i = 0; i < 100; ++i) {
                UNIT_ASSERT(ms.find(i) == ms.end());
                UNIT_ASSERT_VALUES_EQUAL(ms.count(i), 0);
            }
        }
    }

    Y_UNIT_TEST(TestHashMapSimple) {
        typedef NMms::TUnorderedMap<NMms::TStandalone, NMms::TStringType<NMms::TStandalone>, int> THashMapType;

        THashMapType st;
        st.insert(std::make_pair("0", 10));
        st.insert(std::make_pair("1", 1));

        UNIT_ASSERT_VALUES_EQUAL(st["0"], 10);
        UNIT_ASSERT_VALUES_EQUAL(st["1"], 1);
        for (unsigned i = 2; i < 100; ++i) {
            st.insert(std::make_pair(ToString(i), i));
        }

        const TBlob blob = WriteToBlob(st);
        const THashMapType::MmappedType& mm = Cast<THashMapType::MmappedType>(blob);

        UNIT_ASSERT_VALUES_EQUAL(mm.size(), 100);
        UNIT_ASSERT(mm.find("12") != mm.end());
        UNIT_ASSERT_VALUES_EQUAL(mm.find("12")->second, 12);
        UNIT_ASSERT_VALUES_EQUAL(mm["12"], 12);
        UNIT_ASSERT_VALUES_EQUAL(mm["34"], 34);

        UNIT_ASSERT(mm.find("nonexistent") == mm.end());
        UNIT_ASSERT_EXCEPTION(mm["nonexistent"], std::out_of_range);
    }

    Y_UNIT_TEST(TestHashSet) {
        typedef NMms::TUnorderedSet<NMms::TStandalone, NMms::TStringType<NMms::TStandalone>> THashSetType;

        THashSetType st;
        st.insert("0");
        st.insert("1");
        UNIT_ASSERT_VALUES_EQUAL(st.size(), 2);
        UNIT_ASSERT_VALUES_EQUAL(st.count("0"), 1);
        UNIT_ASSERT_VALUES_EQUAL(st.count("1"), 1);
        for (unsigned i = 2; i < 100; ++i)
            st.insert(ToString(i));

        const TBlob blob = WriteToBlob(st);
        const THashSetType::MmappedType& mm = Cast<THashSetType::MmappedType>(blob);

        UNIT_ASSERT_VALUES_EQUAL(mm.size(), 100);
        UNIT_ASSERT(mm.find("12") != mm.end());
        UNIT_ASSERT(mm.find("nonexistent") == mm.end());
    }

    Y_UNIT_TEST(TestHashMapCopyCompatibilty) {
        typedef NMms::TUnorderedMap<NMms::TStandalone, int, int> THashMapType;
        /* Compile time test */
        THashMapType a;
        THashMapType b = a;
        b = a;
        UNIT_ASSERT(true);
    }

    Y_UNIT_TEST(TestHashSetCopyCompatibilty) {
        typedef NMms::TUnorderedSet<NMms::TStandalone, int> THashSetType;
        /* Compile time test */
        THashSetType a;
        THashSetType b = a;
        b = a;
        UNIT_ASSERT(true);
    }

    Y_UNIT_TEST(TestHashMapMmsCopy) {
        typedef NMms::TUnorderedMap<NMms::TStandalone, NMms::TStringType<NMms::TStandalone>, int> THashMapType;

        THashMapType st;
        st.insert(std::make_pair("0", 10));
        st.insert(std::make_pair("1", 1));

        const TBlob blob = WriteToBlob(st);
        const THashMapType::MmappedType& mm = Cast<THashMapType::MmappedType>(blob);

        THashMapType copied;
        NMms::Copy(mm, copied);
        UNIT_ASSERT(AreHashMapsEqual(copied, st));
    }

    Y_UNIT_TEST(TestHashSetMmsCopy) {
        typedef NMms::TUnorderedSet<NMms::TStandalone, NMms::TStringType<NMms::TStandalone>> THashSetType;

        THashSetType st;
        st.insert("0");
        st.insert("1");

        const TBlob blob = WriteToBlob(st);
        const THashSetType::MmappedType& mm = Cast<THashSetType::MmappedType>(blob);

        THashSetType copied;
        NMms::Copy(mm, copied);
        UNIT_ASSERT(AreHashSetsEqual(copied, st));
    }
}

Y_UNIT_TEST_SUITE(TMmsFastHashTest) {
    Y_UNIT_TEST(TestHashDefaultDefault) {
        { //map
            typedef NMms::TFastUnorderedMap<NMms::TMmapped, int, int> THashMapType;

            THashMapType mm;
            UNIT_ASSERT_VALUES_EQUAL(mm.size(), 0);
            UNIT_ASSERT(mm.empty());
            for (unsigned i = 0; i < 100; ++i) {
                UNIT_ASSERT(mm.find(i) == mm.end());
                UNIT_ASSERT_EXCEPTION(mm[i], std::out_of_range);
            }
        }

        { //set
            typedef NMms::TUnorderedSet<NMms::TMmapped, int> THashSetType;

            THashSetType ms;
            UNIT_ASSERT_VALUES_EQUAL(ms.size(), 0);
            UNIT_ASSERT(ms.empty());
            for (unsigned i = 0; i < 100; ++i) {
                UNIT_ASSERT(ms.find(i) == ms.end());
                UNIT_ASSERT_VALUES_EQUAL(ms.count(i), 0);
            }
        }
    }

    Y_UNIT_TEST(TestHashMapSimple) {
        typedef NMms::TFastUnorderedMap<NMms::TStandalone, NMms::TStringType<NMms::TStandalone>, int> THashMapType;

        THashMapType st;
        st.insert(std::make_pair("0", 10));
        st.insert(std::make_pair("1", 1));

        UNIT_ASSERT_VALUES_EQUAL(st["0"], 10);
        UNIT_ASSERT_VALUES_EQUAL(st["1"], 1);
        for (unsigned i = 2; i < 100; ++i) {
            st.insert(std::make_pair(ToString(i), i));
        }

        const TBlob blob = WriteToBlob(st);
        const THashMapType::MmappedType& mm = Cast<THashMapType::MmappedType>(blob);

        UNIT_ASSERT_VALUES_EQUAL(mm.size(), 100);
        UNIT_ASSERT(mm.find("12") != mm.end());
        UNIT_ASSERT_VALUES_EQUAL(mm.find("12")->second, 12);
        UNIT_ASSERT_VALUES_EQUAL(mm["12"], 12);
        UNIT_ASSERT_VALUES_EQUAL(mm["34"], 34);

        UNIT_ASSERT(mm.find("nonexistent") == mm.end());
        UNIT_ASSERT_EXCEPTION(mm["nonexistent"], std::out_of_range);
    }

    Y_UNIT_TEST(TestHashSet) {
        typedef NMms::TUnorderedSet<NMms::TStandalone, NMms::TStringType<NMms::TStandalone>> THashSetType;

        THashSetType st;
        st.insert("0");
        st.insert("1");
        UNIT_ASSERT_VALUES_EQUAL(st.size(), 2);
        UNIT_ASSERT_VALUES_EQUAL(st.count("0"), 1);
        UNIT_ASSERT_VALUES_EQUAL(st.count("1"), 1);
        for (unsigned i = 2; i < 100; ++i)
            st.insert(ToString(i));

        const TBlob blob = WriteToBlob(st);
        const THashSetType::MmappedType& mm = Cast<THashSetType::MmappedType>(blob);

        UNIT_ASSERT_VALUES_EQUAL(mm.size(), 100);
        UNIT_ASSERT(mm.find("12") != mm.end());
        UNIT_ASSERT(mm.find("nonexistent") == mm.end());
    }

    Y_UNIT_TEST(TestHashMapCopyCompatibilty) {
        typedef NMms::TFastUnorderedMap<NMms::TStandalone, int, int> THashMapType;
        /* Compile time test */
        THashMapType a;
        THashMapType b = a;
        b = a;
        UNIT_ASSERT(true);
    }

    Y_UNIT_TEST(TestHashSetCopyCompatibilty) {
        typedef NMms::TUnorderedSet<NMms::TStandalone, int> THashSetType;
        /* Compile time test */
        THashSetType a;
        THashSetType b = a;
        b = a;
        UNIT_ASSERT(true);
    }

    Y_UNIT_TEST(TestHashMapMmsCopy) {
        typedef NMms::TFastUnorderedMap<NMms::TStandalone, NMms::TStringType<NMms::TStandalone>, int> THashMapType;

        THashMapType st;
        st.insert(std::make_pair("0", 10));
        st.insert(std::make_pair("1", 1));

        const TBlob blob = WriteToBlob(st);
        const THashMapType::MmappedType& mm = Cast<THashMapType::MmappedType>(blob);

        THashMapType copied;
        NMms::Copy(mm, copied);
        UNIT_ASSERT(AreHashMapsEqual(copied, st));
    }

    Y_UNIT_TEST(TestHashSetMmsCopy) {
        typedef NMms::TFastUnorderedSet<NMms::TStandalone, NMms::TStringType<NMms::TStandalone>> THashSetType;

        THashSetType st;
        st.insert("0");
        st.insert("1");

        const TBlob blob = WriteToBlob(st);
        const THashSetType::MmappedType& mm = Cast<THashSetType::MmappedType>(blob);

        THashSetType copied;
        NMms::Copy(mm, copied);
        UNIT_ASSERT(AreHashSetsEqual(copied, st));
    }
}
