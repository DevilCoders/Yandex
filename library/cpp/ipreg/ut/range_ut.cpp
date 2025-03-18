#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/ipreg/range.h>

using namespace NIPREG;

namespace {
    using RangesList = TVector<TRange>;

    struct TestData {
        TString orig;
        TVector<TString> results;
    };
    using TestDataList = TVector<TestData>;

    RangesList ParseVectorRanges(const TVector<TString>& rangesList) {
        RangesList result;
        for (const auto& rangeStr : rangesList) {
            result.push_back(TRange::BuildRange(rangeStr));
        }
        return result;
    }

    void CheckSplitResults(const RangesList& got, const RangesList& wanted, const bool showData = true) {
        for (size_t idx = 0; idx != wanted.size(); ++idx) {
            if (showData) {
                Cerr << wanted[idx] << " <==> " << got[idx] << "\n";
            }
            UNIT_ASSERT_EQUAL(wanted[idx], got[idx]);
        }
        UNIT_ASSERT_EQUAL(got.size(), wanted.size());
    }

    void CheckSplitData(const TestDataList& testData) {
        for (const auto& [ rangeStr, resultsList ] : testData) {
            const auto& netOrig = TRange::BuildRange(rangeStr);
            const auto& wantedList = ParseVectorRanges(resultsList);

            const auto& gotList = SplitRangeNets(netOrig);
            CheckSplitResults(gotList, wantedList);
        }
    }
} // anon-ns

Y_UNIT_TEST_SUITE(RangeTest)
{
    Y_UNIT_TEST(BadInitialLines)
    {
        const std::vector<const char*> lines = {
            "",
            "1",
            "1 ",
            "1 2",
            "1 2 ",
            "1-2- ",
            "1\t2\t ",
            "::3\t::1-{}"
        };

        for (const auto& line : lines) {
            UNIT_ASSERT_EXCEPTION(TRange::BuildRange(line), std::exception);
        }
    }

    Y_UNIT_TEST(InitialNoData)
    {
        const std::vector<const char*> lines = {
            "1 2\t",
            "1-3"
        };
        const bool NoData = true;

        try {
            for (const auto& line : lines) {
                TRange::BuildRange(line, NoData);
            }
        }
        catch (const std::exception& ex) {
            Cerr << ">>> " << ex.what() << "\n";
            UNIT_ASSERT(false);
        }
    }

    Y_UNIT_TEST(GoodInitialLines)
    {
        const std::vector<const char*> lines = {
         // "::1 ::2 {}",
         // "::3 ::4-{}",
            "::5 ::6\t{}",
         // "::7-::8-{}",
         // "::9-::10 {}",
            "::11-::12\t{}",
         // "::13\t::14\t{}",
         // "::15\t::16 {}",
         // "::17\t::18-{}"
        };

        for (const auto& line : lines) {
            try {
                TRange::BuildRange(line);
            }
            catch (const std::exception& ex) {
                Cerr << ">>> " << line << "; " << ex.what() << "\n";
                UNIT_ASSERT(false);
            }
        }
    }

    Y_UNIT_TEST(HostsQty) {
        UNIT_ASSERT_EQUAL(TRange::BuildRange("1.1.1.1/32\t{}").GetAddrsQty(), 1);
        UNIT_ASSERT_EQUAL(TRange::BuildRange("1.1.1.0/30\t{}").GetAddrsQty(), 4);
        UNIT_ASSERT_EQUAL(TRange::BuildRange("1.1.1.0/24\t{}").GetAddrsQty(), std::numeric_limits<uint8_t>::max() + 1);
        UNIT_ASSERT_EQUAL(TRange::BuildRange("1.1.0.0/16\t{}").GetAddrsQty(), std::numeric_limits<uint16_t>::max() + 1);
        UNIT_ASSERT_EQUAL(TRange::BuildRange("1.0.0.0/8\t{}").GetAddrsQty(), 65536 * 256);
    }

    Y_UNIT_TEST(Contains) {
        const auto& outerNet0 = TRange::BuildRange("1.2.3.0/24\t{}");
        const auto& outerNet1 = TRange::BuildRange("5.45.192.0/24\t{}");
        const auto& innerSubnet1Net1 = TRange::BuildRange("5.45.192.0/26\t{}");

        UNIT_ASSERT(!outerNet0.Contains(innerSubnet1Net1));
        UNIT_ASSERT(outerNet1.Contains(innerSubnet1Net1));
    }

    Y_UNIT_TEST(PrevNet64_1) {
        const auto& netOrig = TRange::BuildRange("1:2:3:4::-1:2:3:5:6::\t{}");
        const auto& prevNet = netOrig.BuildRangeByLast(netOrig);

        UNIT_ASSERT_EQUAL("1:2:3:4::", prevNet.First.AsShortIPv6());
        UNIT_ASSERT_EQUAL("1:2:3:4:ffff:ffff:ffff:ffff", prevNet.Last.AsShortIPv6());
        UNIT_ASSERT_EQUAL("{}", prevNet.Data);
    }

    Y_UNIT_TEST(PrevNet64_2) {
        const auto& netOrig = TRange::BuildRange("1:2:3:4:5::/72\t{}");
        const auto& newNet = netOrig.BuildRangeByFirst(netOrig);

        UNIT_ASSERT_EQUAL("1:2:3:4::", newNet.First.AsShortIPv6());
        UNIT_ASSERT_EQUAL("1:2:3:4:ffff:ffff:ffff:ffff", newNet.Last.AsShortIPv6());
        UNIT_ASSERT_EQUAL("{}", newNet.Data);
    }

    Y_UNIT_TEST(SplitRanges_simple) {
        const TestDataList dataList_simple = {
              {"0.0.0.0-255.255.255.255\t{123}", {"0.0.0.0-255.255.255.255\t{123}"}}
            , {"::-::fffe:1234:5678\t{456}", {"::-::fffe:1234:5678\t{456}"}}
            , {"1:2:3:4::-1:2:3:4:ffff:ffff:ffff:ffff\t{789}", {"1:2:3:4::-1:2:3:4:ffff:ffff:ffff:ffff\t{789}"}}
            , {"1999::-1999::5\t{789}", {"1999::-1999::5\t{789}"}}
        };
        CheckSplitData(dataList_simple);
    }

    Y_UNIT_TEST(SplitRanges_1to1) {
        const TestDataList dataList_1to1 = {
              {"2001:2:3:4::-2001:2:3:4::5:6\t{}", {"2001:2:3:4::-2001:2:3:4:ffff:ffff:ffff:ffff\t{}"}}
            , {"2002:3:4:5:6:7:1:0-2002:3:4:5:6:8:9:1\t{abc}", {"2002:3:4:5::-2002:3:4:5:ffff:ffff:ffff:ffff\t{abc}"}}
            , {"200a:b:c:d:e:f::-200a:b:c:d:e:f:0:1\t{123}", {"200a:b:c:d::-200a:b:c:d:ffff:ffff:ffff:ffff\t{123}"}}
            , {"20ab:cd::-20ab:cd::1:2:3:4\t{cdf}", {"20ab:cd::-20ab:cd:0:0:ffff:ffff:ffff:ffff\t{cdf}"}}
            , {"2001:420:c0c0:1002::-2001:420:c0c0:1002::\t{qqq}", {"2001:420:c0c0:1002::-2001:420:c0c0:1002:ffff:ffff:ffff:ffff\t{qqq}"}}
        };

        CheckSplitData(dataList_1to1);
    }

    Y_UNIT_TEST(SplitRanges_1to2) {
        const TestDataList dataList_1to2 = {
                {
                    "2001:2:3:4::-2005:6:7:8::1\t{cde}",
                    {
                          "2001:2:3:4::-2005:6:7:7:ffff:ffff:ffff:ffff\t{cde}"
                        , "2005:6:7:8::-2005:6:7:8:ffff:ffff:ffff:ffff\t{cde}"
                    }
                }
              , {
                    "200a:b:c:c::-200a:b:c:d:0:1:2:2\t{123}",
                    {
                          "200a:b:c:c::-200a:b:c:c:ffff:ffff:ffff:ffff\t{123}"
                        , "200a:b:c:d::-200a:b:c:d:ffff:ffff:ffff:ffff\t{123}"
                    }
                }
              , {
                    "2001:240:1400::-2001:240:28e7:d600:c5d0:9beb:97cd:d5e3\t{abc}",
                    {
                          "2001:240:1400::-2001:240:28e7:d5ff:ffff:ffff:ffff:ffff\t{abc}"
                        , "2001:240:28e7:d600::-2001:240:28e7:d600:ffff:ffff:ffff:ffff\t{abc}"
                    }
                }
              , {
                    "2001:67c:2ebc::-2001:67c:2ebc:24bf::\t{hehe}",
                    {
                          "2001:67c:2ebc::-2001:67c:2ebc:24be:ffff:ffff:ffff:ffff\t{hehe}"
                        , "2001:67c:2ebc:24bf::-2001:67c:2ebc:24bf:ffff:ffff:ffff:ffff\t{hehe}"
                    }
                }
        };

        CheckSplitData(dataList_1to2);
    }

    Y_UNIT_TEST(SplitRanges_1to3) {
        const TestDataList dataList_1to3 = {
                {
                    "200a:b:c:d:0:1:2:3-200d:e:f::1\t{123}",
                    {
                          "200a:b:c:d::-200a:b:c:d:ffff:ffff:ffff:ffff\t{123}"
                        , "200a:b:c:e::-200d:e:e:ffff:ffff:ffff:ffff:ffff\t{123}"
                        , "200d:e:f::-200d:e:f:0:ffff:ffff:ffff:ffff\t{123}"
                    }
                }
              , {
                    "2001:240:28e7:d600:c5d0:9beb:97cd:d5e4-2001:240:28e7:ffff:ffff:ffff:ffff:ffff\t{qwe}",
                    {
                          "2001:240:28e7:d600::-2001:240:28e7:d600:ffff:ffff:ffff:ffff\t{qwe}"
                        , "2001:240:28e7:d601::-2001:240:28e7:fffe:ffff:ffff:ffff:ffff\t{qwe}"
                        , "2001:240:28e7:ffff::-2001:240:28e7:ffff:ffff:ffff:ffff:ffff\t{qwe}"
                    }
                }
        };

        CheckSplitData(dataList_1to3);
    }
} // RangeTest
