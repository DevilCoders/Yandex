#include <library/cpp/testing/unittest/registar.h>

#include "search_engine_recognizer.h"

namespace NAntiRobot {
        Y_UNIT_TEST_SUITE(TTestSearchEngineRecognized) {
            Y_UNIT_TEST(CandidatesToRanges) {
                TVector<TCrawlerCandidate> crawlers = {
                    {ECrawler::GoogleBot, TAddr{"66.249.75.200"}},
                    {ECrawler::GoogleBot, TAddr{"66.249.75.216"}},
                    {ECrawler::GoogleBot, TAddr{"66.249.75.217"}},
                    {ECrawler::GoogleBot, TAddr{"66.249.75.218"}},
                    {ECrawler::YahooBot, TAddr{"66.249.75.219"}},
                };

                TSearchEngineRecognizer::TCrawlerRange canon = {
                    {TIpInterval{TAddr{"66.249.75.200"}, TAddr{"66.249.75.200"}}, ECrawler::GoogleBot},
                    {TIpInterval{TAddr{"66.249.75.216"}, TAddr{"66.249.75.218"}}, ECrawler::GoogleBot},
                    {TIpInterval{TAddr{"66.249.75.219"}, TAddr{"66.249.75.219"}}, ECrawler::YahooBot},
                };

                auto result = TSearchEngineRecognizer::CandidatesToRanges(crawlers);
                UNIT_ASSERT_EQUAL(result, canon);
            }
        }
}
