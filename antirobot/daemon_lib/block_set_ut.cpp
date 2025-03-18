#include <library/cpp/testing/unittest/registar.h>

#include "block_set.h"

#include <util/generic/yexception.h>

namespace NAntiRobot {

bool operator == (const TBlockRecord& first, const TBlockRecord& second) {
    return first.Addr == second.Addr
           && first.Category == second.Category
           && first.Description == second.Description
           && first.ExpireTime == second.ExpireTime
           && first.Status == second.Status
           && first.Uid == second.Uid
           && first.YandexUid == second.YandexUid
           ;
}

Y_UNIT_TEST_SUITE(BlockSet) {

Y_UNIT_TEST(Identity) {
    // Record with the same Uid and Category should be identical, other fields don't matter
    const TBlockRecord records[] = {
        {TUid(TUid::IP, 1), BC_SEARCH, TAddr(), "", EBlockStatus::Block, TInstant::Now(), ""},
        {TUid(TUid::IP, 1), BC_SEARCH, TAddr("1.2.3.4"), "b", EBlockStatus::Mark, TInstant::Now(), "a"},
        {TUid(TUid::IP, 1), BC_NON_SEARCH, TAddr(), "", EBlockStatus::Block, TInstant::Now(), ""},
        {TUid(TUid::IP, 2), BC_NON_SEARCH, TAddr(), "", EBlockStatus::Block, TInstant::Now(), ""},
    };

    {
        TBlockSet set = {records[0], records[1]};
        UNIT_ASSERT_VALUES_EQUAL(set.size(), 1);
    }
    {
        TBlockSet set = {records[0], records[2]};
        UNIT_ASSERT_VALUES_EQUAL(set.size(), 2);
    }
    {
        TBlockSet set = {records[3], records[2]};
        UNIT_ASSERT_VALUES_EQUAL(set.size(), 2);
    }
}

Y_UNIT_TEST(TestFindBlockRecord) {
    const TBlockRecord records[] = {
        {TUid(TUid::IP, 1), BC_SEARCH, TAddr(), "", EBlockStatus::Block, TInstant::Now(), ""},
        {TUid(TUid::IP, 1), BC_NON_SEARCH, TAddr(), "", EBlockStatus::Block, TInstant::Now(), ""},
        {TUid(TUid::IP, 2), BC_NON_SEARCH, TAddr(), "", EBlockStatus::Block, TInstant::Now(), ""},
    };
    const TBlockSet set(records, records + Y_ARRAY_SIZE(records));
    for (const auto& rec : records) {
        UNIT_ASSERT(FindBlockRecord(set, rec.Uid, rec.Category));
    }
    UNIT_ASSERT(!FindBlockRecord(set, TUid(TUid::IP, 1), BC_ANY_FROM_WHITELIST));
    UNIT_ASSERT(!FindBlockRecord(set, TUid(TUid::IP, 2), BC_SEARCH));
    UNIT_ASSERT(!FindBlockRecord(set, TUid(TUid::IP, 3), BC_NON_SEARCH));

    auto recPtr = FindBlockRecord(set, records[0].Uid, records[0].Category);
    UNIT_ASSERT_VALUES_EQUAL(recPtr->Addr, records[0].Addr);
    UNIT_ASSERT_VALUES_EQUAL(recPtr->Category, records[0].Category);
    UNIT_ASSERT_VALUES_EQUAL(recPtr->Description, records[0].Description);
    UNIT_ASSERT_VALUES_EQUAL(recPtr->ExpireTime, records[0].ExpireTime);
    UNIT_ASSERT_VALUES_EQUAL(recPtr->Status, records[0].Status);
    UNIT_ASSERT_VALUES_EQUAL(recPtr->Uid, records[0].Uid);
    UNIT_ASSERT_VALUES_EQUAL(recPtr->YandexUid, records[0].YandexUid);
}

Y_UNIT_TEST(TestGetAllItemsWithUid) {
    const TBlockRecord records[] = {
        {TUid(TUid::IP, 1), BC_SEARCH, TAddr(), "", EBlockStatus::Block, TInstant::Now(), ""},
        {TUid(TUid::IP, 1), BC_NON_SEARCH, TAddr(), "", EBlockStatus::Block, TInstant::Now(), ""},
        {TUid(TUid::IP, 2), BC_NON_SEARCH, TAddr(), "", EBlockStatus::Block, TInstant::Now(), ""},
        {TUid(TUid::IP, 3), BC_NON_SEARCH, TAddr(), "", EBlockStatus::Block, TInstant::Now(), ""},
        {TUid(TUid::IP, 3), BC_SEARCH, TAddr(), "", EBlockStatus::Block, TInstant::Now(), ""},
        {TUid(TUid::IP, 3), BC_SEARCH_WITH_SPRAVKA, TAddr(), "", EBlockStatus::Block, TInstant::Now(), ""},
        {TUid(TUid::IP, 3), BC_ANY_FROM_WHITELIST, TAddr(), "", EBlockStatus::Block, TInstant::Now(), ""},
    };
    const TBlockSet set(records, records + Y_ARRAY_SIZE(records));

    const std::pair<TUid, size_t> testData[] = {
        {TUid(TUid::IP, 1), 2},
        {TUid(TUid::IP, 2), 1},
        {TUid(TUid::IP, 3), 4},
        {TUid(TUid::IP, 4), 0},
    };
    for (const auto& testCase : testData) {
        auto range = GetAllItemsWithUid(set, testCase.first);
        UNIT_ASSERT_VALUES_EQUAL(range.size(), testCase.second);
    }
}

Y_UNIT_TEST(TestRemoveIf) {
    const TBlockRecord records[] = {
        {TUid(TUid::IP, 1), BC_SEARCH,     TAddr(), "", EBlockStatus::Block, TInstant::MicroSeconds(0), ""},
        {TUid(TUid::IP, 1), BC_NON_SEARCH, TAddr(), "", EBlockStatus::Block, TInstant::MicroSeconds(1), ""},
        {TUid(TUid::IP, 2), BC_NON_SEARCH, TAddr(), "", EBlockStatus::Block, TInstant::MicroSeconds(2), ""},
        {TUid(TUid::IP, 3), BC_NON_SEARCH, TAddr(), "", EBlockStatus::Block, TInstant::MicroSeconds(3), ""},
    };
    TBlockSet set(records, records + Y_ARRAY_SIZE(records));

    {
        auto removed = RemoveIf(set, [](const TBlockRecord& record) {
            return record.ExpireTime < TInstant::MicroSeconds(3);
        });
        UNIT_ASSERT_VALUES_EQUAL(set.size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(removed.size(), 3);
    }
    {
        auto removed = RemoveIf(set, [](const TBlockRecord&) { return false; });
        UNIT_ASSERT_VALUES_EQUAL(set.size(), 1);
        UNIT_ASSERT(removed.empty());
    }
    {
        auto removed = RemoveIf(set, [](const TBlockRecord&) { return true; });
        UNIT_ASSERT_VALUES_EQUAL(removed.size(), 1);
        UNIT_ASSERT(set.empty());
    }
}

namespace {

void AssertEqual(const TBlockSet& first, const TBlockSet& second) {
    UNIT_ASSERT_VALUES_EQUAL(first.size(), second.size());
    for (TBlockSet::const_iterator i = first.begin(), j = second.begin(); i != first.end(); ++i, ++j) {
        UNIT_ASSERT_VALUES_EQUAL(*i, *j);
    }
}

}

Y_UNIT_TEST(StreamSaveLoad) {
    const TBlockRecord records[] = {
        {TUid(TUid::IP, 1), BC_SEARCH, TAddr("1.2.3.4"), "", EBlockStatus::Block, TInstant::Now(), "abc"},
        {TUid(TUid::IP6, 2), BC_NON_SEARCH, TAddr("1.2.3.4"), "yandexuid", EBlockStatus::Mark, TInstant::MicroSeconds(1), "Description with spaces"},
        {TUid(TUid::SPRAVKA, 8), BC_ANY_FROM_WHITELIST, TAddr(), "12345", EBlockStatus::Block, TInstant::Now(), "Description"},
    };
    const TBlockSet blockSet(records, records + Y_ARRAY_SIZE(records));

    TStringStream ss;
    ss << blockSet;

    TBlockSet another;
    ss >> another;
    AssertEqual(blockSet, another);
}

Y_UNIT_TEST(StreamLoadCorrupted) {
    const TString testData[] = {
        // Wrong uid
        "88-1 BC_NON_SEARCH 1.2.3.4 yandexuid Mark 1428064502814576 description",
        "wrong_uid BC_NON_SEARCH 1.2.3.4 yandexuid Mark 1428064502814576 description",
        // Wrong block category
        "1-1 unknown_category 1.2.3.4 yandexuid Mark 1428064502814576 description",
        "1-1 55874654 1.2.3.4 yandexuid Mark 1428064502814576 description",
        "1-1 BC_NON SEARCH 1.2.3.4 yandexuid Mark 1428064502814576 description",
        // Wrong IP
        "1-1 BC_NON_SEARCH 1.2.3.4.5 yandexuid Mark 1428064502814576 description",
        "1-1 BC_NON_SEARCH 1.2.3 yandexuid Mark 1428064502814576 description",
        "1-1 BC_NON_SEARCH  yandexuid Mark 1428064502814576 description",
        "1-1 BC_NON_SEARCH 900.900.900.900 yandexuid Mark 1428064502814576 description",
        "1-1 BC_NON_SEARCH upyachka yandexuid Mark 1428064502814576 description",
        // No yandex uid
        "1-1 BC_NON_SEARCH 1.2.3.4 Mark 1428064502814576 description",
        // Wrong block status
        "1-1 BC_NON_SEARCH 1.2.3.4 yandexuid wrong_status 1428064502814576 description",
        // Wrong expire time
        "1-1 BC_NON_SEARCH 1.2.3.4 yandexuid Mark expire_time description",
        "1-1 BC_NON_SEARCH 1.2.3.4 yandexuid Mark -1 description",
    };

    for (const TString& test : testData) {
        TStringInput si(test);
        TBlockRecord record;
        UNIT_ASSERT_EXCEPTION(si >> record, yexception);
    }
}

Y_UNIT_TEST(LoadStrongExceptionGuarantee) {
    const TBlockRecord records[] = {
        {TUid(TUid::IP, 1), BC_SEARCH, TAddr(), "", EBlockStatus::Block, TInstant::Now(), ""},
    };
    TBlockSet blockSet(records, records + Y_ARRAY_SIZE(records));

    TBlockSet another = blockSet;
    TStringStream ss;
    ss << "upyachka";

    try {
        ss >> another;
    } catch (yexception&) {
    }

    AssertEqual(blockSet, another);
}

}

}
