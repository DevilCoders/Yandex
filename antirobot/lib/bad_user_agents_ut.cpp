#include <library/cpp/testing/unittest/registar.h>

#include "bad_user_agents.h"

namespace NAntiRobot {
    const TString userAgents(
"bot\n"
"^AppEngine-Google$\n"
"^mozilla/\n"
".*mail.*\n"
"YandexFavicons$\n"
"SimplePie\n"
);


    Y_UNIT_TEST_SUITE(TTestBaduserAgents) {

        Y_UNIT_TEST(TestTrivial) {
            {
                TStringInput str(userAgents);

                TBadUserAgents badAgents;
                badAgents.Load(str);

                UNIT_ASSERT(badAgents.IsUserAgentBad("bot"sv));
                UNIT_ASSERT(badAgents.IsUserAgentBad("boTanik"sv));
                UNIT_ASSERT(badAgents.IsUserAgentBad("megaBot"sv));
                UNIT_ASSERT(badAgents.IsUserAgentBad("megaBOTanik"sv));

                UNIT_ASSERT(badAgents.IsUserAgentBad("AppEngine-Google"sv));
                UNIT_ASSERT(!badAgents.IsUserAgentBad("AppEngine-Google_anything"sv));
                UNIT_ASSERT(!badAgents.IsUserAgentBad("anything_AppEngine-Google_anything"sv));
                UNIT_ASSERT(!badAgents.IsUserAgentBad("anything_AppEngine-Google"sv));

                UNIT_ASSERT(badAgents.IsUserAgentBad("MOZILLA/"sv));
                UNIT_ASSERT(badAgents.IsUserAgentBad("MOZILLA/Compatible"sv));
                UNIT_ASSERT(!badAgents.IsUserAgentBad("bebebeMOZILLA/Compatible"sv));

                UNIT_ASSERT(badAgents.IsUserAgentBad("yandex_mail_google"sv));
                UNIT_ASSERT(badAgents.IsUserAgentBad("maIl"sv));

                UNIT_ASSERT(badAgents.IsUserAgentBad("YandexFavicons"sv));
                UNIT_ASSERT(badAgents.IsUserAgentBad("YandexYandexFavicons"sv));
                UNIT_ASSERT(!badAgents.IsUserAgentBad("YandexFaviconsYandex"sv));
            }
            {
                TStringInput str(userAgents);

                TBadUserAgents badAgents;
                badAgents.Load(str, false);

                UNIT_ASSERT(!badAgents.IsUserAgentBad("APPEngine-GooglE"sv));

                UNIT_ASSERT(!badAgents.IsUserAgentBad("mozillA/"sv));
                UNIT_ASSERT(!badAgents.IsUserAgentBad("MoziLLA/Compatible"sv));

                UNIT_ASSERT(!badAgents.IsUserAgentBad("yandex_Mail_Google"sv));

                UNIT_ASSERT(!badAgents.IsUserAgentBad("YandeXFavicons"sv));
                UNIT_ASSERT(!badAgents.IsUserAgentBad("YandeXYandExFavicons"sv));

                UNIT_ASSERT(badAgents.IsUserAgentBad("SimplePie"sv));
                UNIT_ASSERT(!badAgents.IsUserAgentBad("SimplEPie"sv));
            }
        }
    }
}
