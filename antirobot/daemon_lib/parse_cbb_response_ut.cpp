#include <library/cpp/testing/unittest/registar.h>

#include "unified_agent_log.h"
#include "config_global.h"
#include "parse_cbb_response.h"

#include <util/string/join.h>

namespace NAntiRobot {

    class TTestParseCbbResponseParams : public TTestBase {
    public:
        TString Name() const noexcept override {
            return "TTestParseCbbResponseParams";
        }
    };

    Y_UNIT_TEST_SUITE_IMPL(TTestParseCbbResponse, TTestParseCbbResponseParams) {
        Y_UNIT_TEST(OrdinaryResponse) {
            TAddr addr1;
            TAddr addr2;
            TInstant expire;
            NParseCbb::ParseAddrWithExpire("1.2.3.4; 1.2.3.5; 1234", addr1, addr2, expire);

            UNIT_ASSERT_EQUAL(addr1, TAddr("1.2.3.4"));
            UNIT_ASSERT_EQUAL(addr2, TAddr("1.2.3.5"));
            UNIT_ASSERT_EQUAL(expire, TInstant::Seconds(1234));
        }

        Y_UNIT_TEST(ZeroExpire) {
            TAddr addr1;
            TAddr addr2;
            TInstant expire;
            NParseCbb::ParseAddrWithExpire(" 1.2.3.4 ; 1.2.3.4; 0", addr1, addr2, expire);

            UNIT_ASSERT_EQUAL(addr1, TAddr("1.2.3.4"));
            UNIT_ASSERT_EQUAL(expire, TInstant::Max());
        }

        Y_UNIT_TEST(EmptyExpire) {
            TAddr addr1;
            TAddr addr2;
            TInstant expire;
            NParseCbb::ParseAddrWithExpire(" 1.2.3.4 ; 1.2.3.4; ", addr1, addr2, expire);

            UNIT_ASSERT_EQUAL(addr1, TAddr("1.2.3.4"));
            UNIT_ASSERT_EQUAL(expire, TInstant::Max());
        }

        Y_UNIT_TEST(InvalidResponse) {
            TAddr addr1;
            TAddr addr2;
            TInstant expire;
            UNIT_ASSERT_EXCEPTION(NParseCbb::ParseAddrWithExpire(" 1.2.3.4 12", addr1, addr2, expire), yexception);
            UNIT_ASSERT_EXCEPTION(NParseCbb::ParseAddrWithExpire(" 1.2.3.4 ; 1.2.3.4 12", addr1, addr2, expire), yexception);
        }

        Y_UNIT_TEST(ParseList) {
            TAddrSet set;
            NParseCbb::ParseAddrList(
                    set,
                    "1.2.3.1 ; 1.2.3.1 ; 1\n"
                    "1.2.3.2 ; 1.2.3.2 ; 2\n"
                    "1.2.3.3 ; 1.2.3.3 ; 3"
                    );

            UNIT_ASSERT_EQUAL(set.size(), 3);
            UNIT_ASSERT(set.ContainsActual(TAddr("1.2.3.1"), TInstant::Seconds(0)));
            UNIT_ASSERT(!set.ContainsActual(TAddr("1.2.3.1"), TInstant::Seconds(1)));
            UNIT_ASSERT(set.ContainsActual(TAddr("1.2.3.3"), TInstant::Seconds(2)));
            UNIT_ASSERT(!set.ContainsActual(TAddr("1.2.3.3"), TInstant::Seconds(3)));
        }

        Y_UNIT_TEST(ParseRange) {
            {
                TAddrSet set;
                NParseCbb::ParseAddrList(
                        set,
                        "1.2.3.2 ; 1.2.4.1; 2\n"
                        );

                UNIT_ASSERT_VALUES_EQUAL(set.size(), 1);
                UNIT_ASSERT(set.ContainsActual(TAddr("1.2.3.2"), TInstant::Seconds(1)));
                UNIT_ASSERT(set.ContainsActual(TAddr("1.2.3.3"), TInstant::Seconds(1)));
                UNIT_ASSERT(set.ContainsActual(TAddr("1.2.3.255"), TInstant::Seconds(1)));
                UNIT_ASSERT(set.ContainsActual(TAddr("1.2.4.1"), TInstant::Seconds(1)));

                UNIT_ASSERT(set.Find(TAddr("1.2.3.1")) == set.end());
                UNIT_ASSERT(set.Find(TAddr("1.2.4.2")) == set.end());
            }
            {
                TAddrSet set;
                NParseCbb::ParseAddrList(
                        set,
                        "255.255.255.255 ; 255.255.255.255; 2\n"
                        "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff ; ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff ; 2"
                        );
                UNIT_ASSERT_VALUES_EQUAL(set.size(), 2);
                UNIT_ASSERT(set.ContainsActual(TAddr("255.255.255.255"), TInstant::Seconds(1)));
                UNIT_ASSERT(set.ContainsActual(TAddr("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"), TInstant::Seconds(1)));
            }
        }

        Y_UNIT_TEST(ParseRangeReversed) {
            TAddrSet set;
            NParseCbb::ParseAddrList(
                    set,
                    "255.255.255.255 ; 0.0.0.1; 1\n"
                    );
            UNIT_ASSERT_EQUAL(set.size(), 0);
        }

        Y_UNIT_TEST(ParseTextList) {
            const TVector<TString> testStrings = {
                "abcde",
                R"(header['X-Host']=/abcde\/.+;doc=/\/text?a="test"/)",
                "another",
            };
            TVector<TString> list;
            NParseCbb::ParseTextList(list, JoinSeq("\n", testStrings));

            UNIT_ASSERT_VALUES_EQUAL(list.size(), 3);
            UNIT_ASSERT_VALUES_EQUAL(list[0], "abcde");
            UNIT_ASSERT_VALUES_EQUAL(list[1], testStrings[1]);
            UNIT_ASSERT_VALUES_EQUAL(list[2], "another");
        }
    }
}
