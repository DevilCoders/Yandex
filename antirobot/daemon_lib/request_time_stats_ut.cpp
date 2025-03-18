#include <library/cpp/testing/unittest/registar.h>

#include "request_time_stats.h"
#include "stat.h"

#include <library/cpp/json/json_reader.h>

using namespace NAntiRobot;

namespace {
    const TTimeStatInfoVector timeStatVector = {{"", TDuration::Max()}};
    TTimeStats answerStats{timeStatVector, "answer"};
    TTimeStats waitStats{timeStatVector, "wait"};
    TTimeStats readStats{timeStatVector, "read"};
    TTimeStats captchaStats{timeStatVector, "captcha"};

    TStringBuf GetStatEntryName(const NJson::TJsonValue& value, TStringBuf suffix) {
        return TStringBuf(value.GetArraySafe()[0].GetStringSafe()).Chop(suffix.Size());
    }

    TString GetStatName(const TTimeStats& stats) {
        TStringBuilder statsString;
        {
            TStatsWriter statsWriter(&statsString.Out);
            stats.PrintStats(statsWriter);
        }

        NJson::TJsonValue statsJson;
        NJson::ReadJsonTree(statsString, &statsJson, true);

        auto& statEntries = statsJson.GetArraySafe();
        UNIT_ASSERT_VALUES_EQUAL(statEntries.size(), 2);

        const auto name = GetStatEntryName(statEntries[0], "time__deee");
        UNIT_ASSERT_VALUES_EQUAL(
            name, GetStatEntryName(statEntries[1], "time_upper__deee")
        );

        return TString(name);
    }
}

Y_UNIT_TEST_SUITE(RequestTimeStats) {
    Y_UNIT_TEST(Constructors) {
        {
            TRequestTimeStats requestTimeStats{answerStats, waitStats};

            UNIT_ASSERT_STRINGS_EQUAL(GetStatName(requestTimeStats.AnswerTimeStats), "answer");
            UNIT_ASSERT_STRINGS_EQUAL(GetStatName(requestTimeStats.WaitTimeStats), "wait");
            UNIT_ASSERT_VALUES_EQUAL(requestTimeStats.ReadTimeStats, nullptr);
            UNIT_ASSERT_VALUES_EQUAL(requestTimeStats.CaptchaAnswerTimeStats, nullptr);
        }

        {
            TRequestTimeStats requestTimeStats{answerStats, waitStats, readStats};

            UNIT_ASSERT_STRINGS_EQUAL(GetStatName(requestTimeStats.AnswerTimeStats), "answer");
            UNIT_ASSERT_STRINGS_EQUAL(GetStatName(requestTimeStats.WaitTimeStats), "wait");
            UNIT_ASSERT_STRINGS_EQUAL(GetStatName(*requestTimeStats.ReadTimeStats), "read");
            UNIT_ASSERT_VALUES_EQUAL(requestTimeStats.CaptchaAnswerTimeStats, nullptr);
        }

        {
            TRequestTimeStats requestTimeStats{answerStats, waitStats, readStats, captchaStats};

            UNIT_ASSERT_STRINGS_EQUAL(GetStatName(requestTimeStats.AnswerTimeStats), "answer");
            UNIT_ASSERT_STRINGS_EQUAL(GetStatName(requestTimeStats.WaitTimeStats), "wait");
            UNIT_ASSERT_STRINGS_EQUAL(GetStatName(*requestTimeStats.ReadTimeStats), "read");
            UNIT_ASSERT_STRINGS_EQUAL(GetStatName(*requestTimeStats.CaptchaAnswerTimeStats), "captcha");
        }
    }
}
