#include <kernel/common_server/util/raw_text/datetime.h>

#include <library/cpp/testing/unittest/registar.h>

namespace {
    bool AreDaysEqual(const ui32 yearDay, TVector<std::tuple<TString, bool>> dates) {
        const auto predicate = [=](const std::tuple<TString, bool>& data) {
            TString date;
            bool forceLeapYear;
            std::tie(date, forceLeapYear) = data;
            return yearDay == NUtil::GetYearDay(TInstant::ParseIso8601(date), forceLeapYear);
        };
        return std::all_of(dates.cbegin(), dates.cend(), predicate);
    }
}

Y_UNIT_TEST_SUITE(RawTextParseSuite) {
    Y_UNIT_TEST(ParseDateTime) {
        TInstant parsedDateTime;

        parsedDateTime = NUtil::ParseFomattedLocalDatetime("22-08-2019 22:48:00", "%d-%m-%Y %H:%M:%S");
        UNIT_ASSERT_STRINGS_EQUAL(parsedDateTime.ToStringUpToSeconds(), "2019-08-22T22:48:00Z");

        parsedDateTime = NUtil::ParseFomattedLocalDatetime("22-08-2019 22:48:00", "%d-%m-%Y %H:%M:%S", "Europe/Moscow");
        UNIT_ASSERT_STRINGS_EQUAL(parsedDateTime.ToStringUpToSeconds(), "2019-08-22T19:48:00Z");
    }

    Y_UNIT_TEST(ParseDate) {
        TInstant parsedDate;
        parsedDate = NUtil::ParseFomattedLocalDatetime("22-08-2019", "%d-%m-%Y");
        UNIT_ASSERT_STRINGS_EQUAL(parsedDate.ToStringUpToSeconds(), "2019-08-22T00:00:00Z");
    }

    Y_UNIT_TEST(GetYearDay) {
        UNIT_ASSERT(AreDaysEqual(0, { std::make_tuple<TString, bool>("2000-01-01", true),
                                      std::make_tuple<TString, bool>("2000-01-01", false),
                                      std::make_tuple<TString, bool>("2019-01-01", true),
                                      std::make_tuple<TString, bool>("2019-01-01", false),
                                      std::make_tuple<TString, bool>("2020-01-01", true),
                                      std::make_tuple<TString, bool>("2020-01-01", false) } ));

        UNIT_ASSERT(AreDaysEqual(31, { std::make_tuple<TString, bool>("2000-02-01", true),
                                       std::make_tuple<TString, bool>("2000-02-01", false),
                                       std::make_tuple<TString, bool>("2019-02-01", true),
                                       std::make_tuple<TString, bool>("2019-02-01", false),
                                       std::make_tuple<TString, bool>("2020-02-01", true),
                                       std::make_tuple<TString, bool>("2020-02-01", false) } ));

        UNIT_ASSERT(AreDaysEqual(59, { std::make_tuple<TString, bool>("2000-02-29", true),
                                       std::make_tuple<TString, bool>("2000-02-29", false),
                                       std::make_tuple<TString, bool>("2019-03-01", false),
                                       std::make_tuple<TString, bool>("2020-02-29", true),
                                       std::make_tuple<TString, bool>("2020-02-29", false) } ));

        UNIT_ASSERT(AreDaysEqual(60, { std::make_tuple<TString, bool>("2000-03-01", true),
                                       std::make_tuple<TString, bool>("2000-03-01", false),
                                       std::make_tuple<TString, bool>("2019-03-01", true),
                                       std::make_tuple<TString, bool>("2020-03-01", true),
                                       std::make_tuple<TString, bool>("2020-03-01", false) } ));
    }
}
