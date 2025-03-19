#include "userid.h"

#include <library/cpp/testing/unittest/registar.h>

namespace NUgc {
    class TUserIdTest: public TTestBase {
    public:
        UNIT_TEST_SUITE(TUserIdTest)
        UNIT_TEST(StandardUserIdsAreValid)
        UNIT_TEST(NotStandardPrefixIsInvalid)
        UNIT_TEST(EmptyUserIsInvalid)
        UNIT_TEST(OnlyPrefixIsInvalid)
        UNIT_TEST(FirstCharIsNotSlashIsInvalid)
        UNIT_TEST(NumberIsInvalid)
        UNIT_TEST(OneSlashIsInvalid)
        UNIT_TEST(TrailingSlashIsInvalid)
        UNIT_TEST(ManySlashesIsOk)
        UNIT_TEST(TwoSlashesTogetherAreInvalid)
        UNIT_TEST(AllowedSymbols)
        UNIT_TEST(ValidatesInEveryConstructor)
        UNIT_TEST(GetsPassportUID)
        UNIT_TEST(GetsYandexUID)
        UNIT_TEST(ValidPrefixesAreWithSlashes)
        UNIT_TEST(CheckLoginStatus)
        UNIT_TEST(CreateUserIdFromValidPassportUid)
        UNIT_TEST(FailOnInvalidPassportUid)
        UNIT_TEST(CreateUserIdFromYandexUid)
        UNIT_TEST(CreateUserIdFromOrgId)
        UNIT_TEST(FailOnInvalidOrgId);
        UNIT_TEST_SUITE_END();

        void StandardUserIdsAreValid() {
            UNIT_ASSERT_NO_EXCEPTION(TUserId("/user/123"));
            UNIT_ASSERT_NO_EXCEPTION(TUserId("/visitor/234"));
            UNIT_ASSERT_NO_EXCEPTION(TUserId("/mobapp/345"));
        }

        void NotStandardPrefixIsInvalid() {
            UNIT_ASSERT_EXCEPTION(TUserId("/users/123"), TBadArgumentException);
        }

        void EmptyUserIsInvalid() {
            UNIT_ASSERT_EXCEPTION(TUserId(""), TBadArgumentException);
        }

        void OnlyPrefixIsInvalid() {
            UNIT_ASSERT_EXCEPTION(TUserId("/user/"), TBadArgumentException);
            UNIT_ASSERT_EXCEPTION(TUserId("/user"), TBadArgumentException);
            UNIT_ASSERT_EXCEPTION(TUserId("/visitor/"), TBadArgumentException);
            UNIT_ASSERT_EXCEPTION(TUserId("/visitor"), TBadArgumentException);
            UNIT_ASSERT_EXCEPTION(TUserId("/mobapp/"), TBadArgumentException);
            UNIT_ASSERT_EXCEPTION(TUserId("/mobapp"), TBadArgumentException);
        }

        void FirstCharIsNotSlashIsInvalid() {
            UNIT_ASSERT_EXCEPTION(TUserId("user"), TBadArgumentException);
            UNIT_ASSERT_EXCEPTION(TUserId("user/123"), TBadArgumentException);
            UNIT_ASSERT_EXCEPTION(TUserId("visitor/123"), TBadArgumentException);
            UNIT_ASSERT_EXCEPTION(TUserId("user/abc/12/dd"), TBadArgumentException);
        }

        void NumberIsInvalid() {
            UNIT_ASSERT_EXCEPTION(TUserId("42536475"), TBadArgumentException);
        }

        void OneSlashIsInvalid() {
            UNIT_ASSERT_EXCEPTION(TUserId("a/b"), TBadArgumentException);
            UNIT_ASSERT_EXCEPTION(TUserId("/a"), TBadArgumentException);
        }

        void TrailingSlashIsInvalid() {
            UNIT_ASSERT_EXCEPTION(TUserId("/user/123/"), TBadArgumentException);
            UNIT_ASSERT_EXCEPTION(TUserId("/mobapp/a/b/c/d/e/f/"), TBadArgumentException);
        }

        void ManySlashesIsOk() {
            UNIT_ASSERT_NO_EXCEPTION(TUserId("/mobapp/a/b/c/d/e/f"));
        }

        void TwoSlashesTogetherAreInvalid() {
            UNIT_ASSERT_EXCEPTION(TUserId("/user//123"), TBadArgumentException);
        }

        void AllowedSymbols() {
            UNIT_ASSERT_NO_EXCEPTION(TUserId("/visitor/A/b/122323/zZ"));
            UNIT_ASSERT_NO_EXCEPTION(TUserId("/user/123~"));
            UNIT_ASSERT_NO_EXCEPTION(TUserId("/user/1-2-3"));
            UNIT_ASSERT_NO_EXCEPTION(TUserId("/user/\"123\""));
        }

        void ValidatesInEveryConstructor() {
            UNIT_ASSERT_EXCEPTION(TUserId(" "), TBadArgumentException);
            UNIT_ASSERT_EXCEPTION(TUserId(TStringBuf(" ")), TBadArgumentException);
            UNIT_ASSERT_EXCEPTION(TUserId(TString(" ")), TBadArgumentException);
        }

        void GetsPassportUID() {
            UNIT_ASSERT_EQUAL(TUserId("/user/123").GetPassportUID(), "123");     // valid
            UNIT_ASSERT_EQUAL(TUserId("/user/12a").GetPassportUID(), "");        // invalid
            UNIT_ASSERT_EQUAL(TUserId("/visitor/123").GetPassportUID(), "");        // invalid
        }

        void GetsYandexUID() {
            UNIT_ASSERT_EQUAL(TUserId("/user/123").GetYandexUID(), "");     // invalid
            UNIT_ASSERT_EQUAL(TUserId("/visitor/123").GetYandexUID(), "123");        // valid
            UNIT_ASSERT_EQUAL(TUserId("/visitor/123a").GetYandexUID(), "123a");        // still valid
        }

        void ValidPrefixesAreWithSlashes() {
            UNIT_ASSERT(!TUserId::ValidPrefixes.empty());
            for (const TString& prefix : TUserId::ValidPrefixes) {
                UNIT_ASSERT_C(!prefix.empty(), "Prefix: " << prefix);
                UNIT_ASSERT_VALUES_EQUAL_C(prefix[0], '/', "Prefix: " << prefix);
                UNIT_ASSERT_VALUES_EQUAL_C(prefix.back(), '/', "Prefix: " << prefix);
            }
        }

        void CheckLoginStatus() {
            UNIT_ASSERT(TUserId("/user/123").IsLoggedIn());

            UNIT_ASSERT(!TUserId("/orguser/123").IsLoggedIn());
            UNIT_ASSERT(!TUserId("/mobapp/123").IsLoggedIn());
            UNIT_ASSERT(!TUserId("/visitor/123").IsLoggedIn());
        }

        void CreateUserIdFromValidPassportUid() {
            TUserId uid = TUserId::FromPassportUID("123");
            UNIT_ASSERT(uid.IsLoggedIn());
            UNIT_ASSERT_STRINGS_EQUAL(uid.AsString(), "/user/123");

            TUserId uid1 = TUserId::FromPassportUID(123);
            UNIT_ASSERT(uid1.IsLoggedIn());
            UNIT_ASSERT_STRINGS_EQUAL(uid1.AsString(), "/user/123");
        }

        void FailOnInvalidPassportUid() {
            UNIT_ASSERT_EXCEPTION_CONTAINS(
                TUserId::FromPassportUID("abc"),
                TBadArgumentException,
                "invalid passport id");
        }

        void CreateUserIdFromYandexUid() {
            TUserId uid = TUserId::FromYandexUID("123");
            UNIT_ASSERT(!uid.IsLoggedIn());
            UNIT_ASSERT_STRINGS_EQUAL(uid.AsString(), "/visitor/123");
        }

        void CreateUserIdFromOrgId() {
            TUserId uid = TUserId::FromOrgId("123");
            UNIT_ASSERT(!uid.IsLoggedIn());
            UNIT_ASSERT_STRINGS_EQUAL(uid.AsString(), "/orguser/123");
        }

        void FailOnInvalidOrgId() {
            UNIT_ASSERT_EXCEPTION_CONTAINS(
                TUserId::FromOrgId("abc"),
                TBadArgumentException,
                "invalid org id");
        }
    };

    UNIT_TEST_SUITE_REGISTRATION(TUserIdTest);
} // namespace NUgc
