#include <library/cpp/testing/unittest/registar.h>

#include "addr_list.h"

#include <antirobot/lib/ar_utils.h>

#include <util/generic/algorithm.h>

#include <tuple>

namespace NAntiRobot {
    Y_UNIT_TEST_SUITE(TTestExpiringAddrSet) {
        Y_UNIT_TEST(ContainsActual) {
            {
                TAddrSet set;

                set.Add(TIpInterval(TAddr("1.1.1.1"), TAddr("1.1.1.2")), TInstant::Seconds(2));
                UNIT_ASSERT_VALUES_EQUAL(1, set.size());

                UNIT_ASSERT(set.ContainsActual(TAddr("1.1.1.1"), TInstant::Seconds(1)));
                UNIT_ASSERT(!set.ContainsActual(TAddr("1.1.1.1"), TInstant::Seconds(2)));

                UNIT_ASSERT(set.ContainsActual(TAddr("1.1.1.2"), TInstant::Seconds(1)));
                UNIT_ASSERT(!set.ContainsActual(TAddr("1.1.1.2"), TInstant::Seconds(2)));
                UNIT_ASSERT(!set.ContainsActual(TAddr("1.1.1.0"), TInstant::Seconds(1)));
                UNIT_ASSERT(!set.ContainsActual(TAddr("1.1.1.0"), TInstant::Seconds(2)));
                UNIT_ASSERT(!set.ContainsActual(TAddr("1.1.1.3"), TInstant::Seconds(1)));
                UNIT_ASSERT(!set.ContainsActual(TAddr("1.1.1.3"), TInstant::Seconds(2)));
            }
            {
                TAddrSet set;

                set.Add(TIpInterval(TAddr("1.1.1.10"), TAddr("1.1.1.20")), TInstant::Seconds(2));
                set.Add(TIpInterval(TAddr("1.1.2.10"), TAddr("1.1.2.20")), TInstant::Seconds(10));
                UNIT_ASSERT_VALUES_EQUAL(2, set.size());

                UNIT_ASSERT(set.ContainsActual(TAddr("1.1.1.10"), TInstant::Seconds(1)));
                UNIT_ASSERT(!set.ContainsActual(TAddr("1.1.1.10"), TInstant::Seconds(2)));

                UNIT_ASSERT(set.ContainsActual(TAddr("1.1.1.19"), TInstant::Seconds(1)));
                UNIT_ASSERT(set.ContainsActual(TAddr("1.1.1.20"), TInstant::Seconds(1)));
                UNIT_ASSERT(!set.ContainsActual(TAddr("1.1.1.20"), TInstant::Seconds(2)));

                UNIT_ASSERT(set.ContainsActual(TAddr("1.1.2.10"), TInstant::Seconds(9)));
                UNIT_ASSERT(!set.ContainsActual(TAddr("1.1.2.10"), TInstant::Seconds(10)));
                UNIT_ASSERT(!set.ContainsActual(TAddr("1.1.2.21"), TInstant::Seconds(1)));
            }
            {
                TAddrSet set;

                set.Add(TIpInterval(TAddr("1.1.1.1"), TAddr("1.1.1.2")), TInstant::Seconds(2));
                set.Add(TIpInterval(TAddr("1.1.1.10"), TAddr("1.1.1.15")), TInstant::Seconds(5));
                set.Add(TIpInterval(TAddr("1.1.1.16"), TAddr("1.1.1.20")), TInstant::Seconds(10));

                set.Add(TIpInterval(TAddr("1.1.2.10"), TAddr("1.1.2.20")), TInstant::Seconds(5));
                UNIT_ASSERT_EXCEPTION(set.Add(TIpInterval(TAddr("1.1.2.21"), TAddr("1.1.2.20")), TInstant::Seconds(10)), yexception); // bad range
                set.Add(TIpInterval(TAddr("1.1.2.21"), TAddr("1.1.2.30")), TInstant::Seconds(10));

                UNIT_ASSERT_VALUES_EQUAL(5, set.size());

                UNIT_ASSERT(set.ContainsActual(TAddr("1.1.1.10"), TInstant::Seconds(1)));
                UNIT_ASSERT(set.ContainsActual(TAddr("1.1.1.15"), TInstant::Seconds(3)));
                UNIT_ASSERT(!set.ContainsActual(TAddr("1.1.1.15"), TInstant::Seconds(9)));
                UNIT_ASSERT(set.ContainsActual(TAddr("1.1.1.16"), TInstant::Seconds(9)));

                UNIT_ASSERT(set.ContainsActual(TAddr("1.1.2.10"), TInstant::Seconds(3)));
                UNIT_ASSERT(!set.ContainsActual(TAddr("1.1.2.10"), TInstant::Seconds(6)));
                UNIT_ASSERT(set.ContainsActual(TAddr("1.1.2.15"), TInstant::Seconds(4)));
            }
            {
                TAddrSet set;

                set.Add(TIpInterval(TAddr("f0::1"), TAddr("f0::2")), TInstant::Seconds(2));

                UNIT_ASSERT_VALUES_EQUAL(1, set.size());
                UNIT_ASSERT(set.ContainsActual(TAddr("f0::1"), TInstant::Seconds(1)));
                UNIT_ASSERT(!set.ContainsActual(TAddr("f0::1"), TInstant::Seconds(2)));

                UNIT_ASSERT(set.ContainsActual(TAddr("f0::2"), TInstant::Seconds(1)));
                UNIT_ASSERT(!set.ContainsActual(TAddr("f0::2"), TInstant::Seconds(2)));

                UNIT_ASSERT(!set.ContainsActual(TAddr("f0::0"), TInstant::Seconds(1)));
                UNIT_ASSERT(!set.ContainsActual(TAddr("f0::3"), TInstant::Seconds(1)));
            }
        }

        Y_UNIT_TEST(IntersectedRanges) {
            TAddrSet set;

            UNIT_ASSERT(set.Add(TIpInterval(TAddr("1.1.1.5"), TAddr("1.1.1.10")), TInstant::Seconds(2)));
            UNIT_ASSERT(set.Add(TIpInterval(TAddr("1.1.2.5"), TAddr("1.1.2.5")), TInstant::Seconds(2)));
            UNIT_ASSERT(set.Add(TIpInterval(TAddr("1.1.2.10"), TAddr("1.1.2.20")), TInstant::Seconds(2)));

            // the following must not have an effect
            UNIT_ASSERT(!set.Add(TIpInterval(TAddr("1.1.1.6"), TAddr("1.1.1.7")), TInstant::Seconds(5)));
            UNIT_ASSERT(!set.Add(TIpInterval(TAddr("1.1.1.10"), TAddr("1.1.1.15")), TInstant::Seconds(5)));
            UNIT_ASSERT(!set.Add(TIpInterval(TAddr("1.1.1.5"), TAddr("1.1.1.15")), TInstant::Seconds(5)));
            UNIT_ASSERT(!set.Add(TIpInterval(TAddr("1.1.1.1"), TAddr("1.1.1.5")), TInstant::Seconds(5)));
            UNIT_ASSERT(!set.Add(TIpInterval(TAddr("1.1.1.1"), TAddr("1.1.1.15")), TInstant::Seconds(5)));

            UNIT_ASSERT(!set.Add(TIpInterval(TAddr("1.1.1.1"), TAddr("1.1.2.20")), TInstant::Seconds(5)));
            UNIT_ASSERT(!set.Add(TIpInterval(TAddr("1.1.1.1"), TAddr("1.1.2.21")), TInstant::Seconds(5)));
            UNIT_ASSERT(!set.Add(TIpInterval(TAddr("1.1.2.1"), TAddr("1.1.2.10")), TInstant::Seconds(5)));
            UNIT_ASSERT(!set.Add(TIpInterval(TAddr("1.1.2.1"), TAddr("1.1.2.11")), TInstant::Seconds(5)));
            UNIT_ASSERT(!set.Add(TIpInterval(TAddr("1.1.2.11"), TAddr("1.1.2.11")), TInstant::Seconds(5)));
            UNIT_ASSERT(!set.Add(TIpInterval(TAddr("1.1.2.10"), TAddr("1.1.2.10")), TInstant::Seconds(5)));
            UNIT_ASSERT(!set.Add(TIpInterval(TAddr("1.1.2.20"), TAddr("1.1.2.20")), TInstant::Seconds(5)));

            UNIT_ASSERT_VALUES_EQUAL(3, set.size());

            UNIT_ASSERT(set.ContainsActual(TAddr("1.1.1.6"), TInstant::Seconds(1)));
            UNIT_ASSERT(set.ContainsActual(TAddr("1.1.2.5"), TInstant::Seconds(1)));
            UNIT_ASSERT(!set.ContainsActual(TAddr("1.1.2.11"), TInstant::Seconds(3)));
        }

        Y_UNIT_TEST(Has) {
            TAddrSet set;

            set.Add(TIpInterval(TAddr("1.1.1.5"), TAddr("1.1.1.10")), TInstant::Seconds(12));
            set.Add(TIpInterval(TAddr("1.1.2.5"), TAddr("1.1.2.5")), TInstant::Seconds(13));
            set.Add(TIpInterval(TAddr("1.1.2.10"), TAddr("1.1.2.20")), TInstant::Seconds(14));

            {
                const auto r = set.Find(TAddr("1.1.1.5"));
                UNIT_ASSERT(r != set.end());
                UNIT_ASSERT_VALUES_EQUAL(r->second.Expire, TInstant::Seconds(12));
            }
            {
                const auto r = set.Find(TAddr("1.1.2.5"));
                UNIT_ASSERT(r != set.end());
                UNIT_ASSERT_VALUES_EQUAL(r->second.Expire, TInstant::Seconds(13));
            }
            {
                const auto r = set.Find(TAddr("1.1.2.20"));
                UNIT_ASSERT(r != set.end());
                UNIT_ASSERT_VALUES_EQUAL(r->second.Expire, TInstant::Seconds(14));
            }
        }

        Y_UNIT_TEST(MergeRefreshableAddrSets) {
            TVector<std::tuple<TStringBuf, TStringBuf, ui64>> intervals = {
                {"0.0.0.0", "1.2.3.4", 1},
                {"1.2.3.0", "6.6.6.6", 2},
                {"3.0.0.0", "6.6.6.6", 1},
                {"6.7.0.0", "6.8.0.0", 0},
                {"6.7.1.0", "6.7.2.0", 1},
                {"6.7.3.0", "6.7.3.0", 1},
                {"7.0.0.0", "8.0.0.0", 0},
                {"8.5.0.0", "255.255.255.255", 42},
                {"8.5.0.1", "255.255.255.255", 500},
                {"8.5.0.0", "255.255.255.255", 0}
            };

            TVector<TAddrSet> addrSets;

            for (size_t i = 0; i < intervals.size(); ++i) {
                TAddrSet addrSet;

                const auto [low, high, expire] = intervals[i];
                addrSet.Add(
                    TIpInterval(TAddr(low), TAddr(high)),
                    TInstant::Seconds(expire)
                );

                addrSets.push_back(addrSet);
            }

            const auto mergedSet = MergeAddrSets(addrSets);
            TVector<std::tuple<TString, TString, ui64>> result;

            for (const auto& [low, attr] : mergedSet) {
                result.push_back({
                    low.ToString(), attr.EndAddr.ToString(),
                    attr.Expire.Seconds()
                });
            }

            TVector<std::tuple<TString, TString, ui64>> expected = {
                {"0.0.0.0", "1.2.2.255", 1},
                {"1.2.3.0", "6.6.6.6", 2},
                {"6.7.0.0", "6.7.0.255", 0},
                {"6.7.1.0", "6.7.2.0", 1},
                {"6.7.2.1", "6.7.2.255", 0},
                {"6.7.3.0", "6.7.3.0", 1},
                {"6.7.3.1", "6.8.0.0", 0},
                {"7.0.0.0", "8.0.0.0", 0},
                {"8.5.0.0", "8.5.0.0", 42},
                {"8.5.0.1", "255.255.255.255", 500}
            };

            UNIT_ASSERT_EQUAL(result, expected);
        }
    }
}
