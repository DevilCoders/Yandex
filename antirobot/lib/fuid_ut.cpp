#include <library/cpp/testing/unittest/registar.h>

#include "fuid.h"

#include <library/cpp/http/cookies/cookies.h>

#include <util/stream/str.h>

#include <cstring> // memcmp()


using namespace NAntiRobot;

bool operator==(const TFlashCookie& fuid1, const TFlashCookie& fuid2) {
    return memcmp(&fuid1, &fuid2, sizeof(TFlashCookie)) == 0;
}


void CheckCorrectDate(const TFlashCookie& fuid) {
    const ui64 time = fuid.Time().TimeT();
    UNIT_ASSERT(1200000000 <= time); // 2008-01-11
    UNIT_ASSERT(time <= 1600000000); // 2020-09-13
}

Y_UNIT_TEST_SUITE(TTestFuid) {
    Y_UNIT_TEST(Parse) {
        struct TFuidDescr {
            TString FuidStr;
            ui64 Id;
            ui32 Time;
        };
        const TFuidDescr fuidDescrs[] = {
              {
                  "5497ba437e46d244.tq8tZ1djzKrnE3622EzdoFAfb5WK2e9fnIGA06aUvq7D_8A2Hzlm8FpmHprQqzOduP6Ze7QYcZ9FBqiaDxQrsU1-3jsDlkFKf5lbby7VahmbFqZEhxJov8YwhmOgSSbp"
                , 9099191288067504707ULL
                , 1419229763
              }
            , {
                  "547614604dc95e20.sjDR4wAozBrQkidbcEqQeqWMFuxMV-EF7avqSrrl1jqjaS3j8UPwpy_JlP87xUGSe_21dQVLjySFJjIKPcIXGiQ84Gge8iW9RGinLOQ8e1MdyqwRATow8B_FWbrn4Zt9"
                , 5605114704188281952ULL
                , 1417024608
              }
        };
        for (const auto& fuidDescr: fuidDescrs) {
            const TString& fuidStr = fuidDescr.FuidStr;
            const TString fuidCookieValue = "fuid01=" + fuidStr;
            const THttpCookies cookies(fuidCookieValue);

            TFlashCookie fuid1;
            TFlashCookie fuid2;

            UNIT_ASSERT(fuid1.Parse(fuidStr) != nullptr);
            UNIT_ASSERT(fuid2.ParseCookies(cookies) != nullptr);
            UNIT_ASSERT_EQUAL(fuid1, fuid2);
            UNIT_ASSERT_EQUAL(fuid1.Time(), TInstant::Seconds(fuidDescr.Time));
            UNIT_ASSERT_VALUES_EQUAL(fuid1.Id(), fuidDescr.Id);
            CheckCorrectDate(fuid1);
        }
    }

    Y_UNIT_TEST(StreamIO) {
        TString s;
        const TFlashCookie fuid1 = TFlashCookie::FromId(5605114704188281952ULL);
        {
            TStringOutput out(s);
            out << fuid1;
        }
        TFlashCookie fuid2;
        {
            TStringInput in(s);
            in >> fuid2;
        }
        UNIT_ASSERT_VALUES_EQUAL(fuid1.Id(), fuid2.Id());
        UNIT_ASSERT_EQUAL(fuid1, fuid2);
        CheckCorrectDate(fuid1);
    }

    Y_UNIT_TEST(StreamInputValid) {
        const TString in[] = {
              "12-1312345678"
            , "1123123123-1449854954"
        };
        for (const auto& s: in) {
            TStringInput fin(s);
            TFlashCookie fuid;
            UNIT_ASSERT_NO_EXCEPTION(fin >> fuid);
            CheckCorrectDate(fuid);
        }
    }

    Y_UNIT_TEST(StreamInputInvalid) {
        const TString in[] = {
            // integer overflaw
              "1312345678-12111111111"
            , "1123123123-4449854954"
            , "11123123123-1449854954"
            // old format
            , "17002981961416813987"
            , "2744775527702702875"
            // bad format
            , ""
            , "170"
            , "123-123-123"
        };
        for (const auto& s: in) {
            TStringInput fin(s);
            TFlashCookie fuid;
            UNIT_ASSERT_EXCEPTION(fin >> fuid, yexception);
        }
    }

    Y_UNIT_TEST(IdConvert) {
        const TFlashCookie fuid1 = TFlashCookie::FromId(9099191288067504707ULL);
        const TFlashCookie fuid2 = TFlashCookie::FromId(fuid1.Id());
        UNIT_ASSERT_VALUES_EQUAL(fuid1.Id(), fuid2.Id());
        UNIT_ASSERT_EQUAL(fuid1, fuid2);
        CheckCorrectDate(fuid1);
    }
}
