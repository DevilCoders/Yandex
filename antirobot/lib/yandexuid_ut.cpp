#include <library/cpp/testing/unittest/registar.h>

#include "yandexuid.h"

#include <util/generic/algorithm.h>

namespace NAntiRobot {

Y_UNIT_TEST_SUITE(GetYandexUid) {

template<size_t Length>
TString MakeCookieString(const TString (&uids)[Length]) {
    TString cookieString;
    for (const TString& uid : uids) {
        cookieString += "yandexuid=" + uid + ";";
    }
    return cookieString;
}

#define SOME_TIMESTAMP "1400000000"

Y_UNIT_TEST(Empty) {
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexUid(THttpCookies("yandexuid=;")), "");
}

Y_UNIT_TEST(NoYandexUid) {
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexUid(THttpCookies("L=foo; fuid01=bar;")), "");
}

Y_UNIT_TEST(Single) {
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexUid(THttpCookies("yandexuid=12345" SOME_TIMESTAMP)), "12345" SOME_TIMESTAMP);
}

Y_UNIT_TEST(SingleValid) {
    const TString UIDS[] = {
        "1" SOME_TIMESTAMP,
        "12345" SOME_TIMESTAMP,
        "1234567890" SOME_TIMESTAMP,
    };

    for (const auto& uid: UIDS) {
        UNIT_ASSERT(IsValidYandexUid(uid));
        UNIT_ASSERT_STRINGS_EQUAL(GetYandexUid(THttpCookies("yandexuid=" + uid)), uid);
    }
}

Y_UNIT_TEST(SingleInvalid) {
    const TString UIDS[] = {
        SOME_TIMESTAMP,
        "123456789012" SOME_TIMESTAMP,
        "85.236.29.242.1369294263197912",
        "7148479301406877688,Session_id=noauth:1406877688,Cookie_check=1,ys=wprid.1406877689545856-1424238832023566647626543-ws39-190-NEWS_STORY",
    };

    for (const auto& uid: UIDS) {
        UNIT_ASSERT(!IsValidYandexUid(uid));
        UNIT_ASSERT_STRINGS_EQUAL(GetYandexUid(THttpCookies("yandexuid=" + uid)), "");
    }
}

Y_UNIT_TEST(MultipleNonEmpty) {
    const TString UIDS[] = {"123" SOME_TIMESTAMP, "456" SOME_TIMESTAMP, "789" SOME_TIMESTAMP};

    TString coockieStr = MakeCookieString(UIDS);
    TStringBuf yandexuid = GetYandexUid(THttpCookies(coockieStr));
    // if there are multiple yandexuids the function should return any of them
    const auto end = UIDS + Y_ARRAY_SIZE(UIDS);
    UNIT_ASSERT(Find(UIDS, end, yandexuid) != end);
}

Y_UNIT_TEST(MultipleEmptyAndNonEmpty) {
    const TString UIDS[] = {"123" SOME_TIMESTAMP, "456" SOME_TIMESTAMP, ""};

    TString coockieStr = MakeCookieString(UIDS);
    TStringBuf yandexuid = GetYandexUid(THttpCookies(coockieStr));
    // if there are multiple yandexuids the function should return any nonempty one
    const auto end = UIDS + Y_ARRAY_SIZE(UIDS) - 1; // exclude the empty one
    UNIT_ASSERT(Find(UIDS, end, yandexuid) != end);
}

Y_UNIT_TEST(EmptyAndNonEmpty) {
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexUid(THttpCookies("yandexuid=; yandexuid=123" SOME_TIMESTAMP ";")), "123" SOME_TIMESTAMP);
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexUid(THttpCookies("yandexuid=123" SOME_TIMESTAMP "; yandexuid=;")), "123" SOME_TIMESTAMP);
    UNIT_ASSERT_STRINGS_EQUAL(GetYandexUid(THttpCookies("yandexuid=; yandexuid=123456789012" SOME_TIMESTAMP)), "");
}

}

}

