#include <library/cpp/testing/unittest/registar.h>

#include "addr.h"
#include "ar_utils.h"

#include <util/ysaveload.h>

#include <array>
#include <utility>

using namespace NAntiRobot;


Y_UNIT_TEST_SUITE(TAddrTest) {

    Y_UNIT_TEST(SaveLoadIp4) {
        TStringStream str;
        TAddr addr("1.2.3.4");
        UNIT_ASSERT_EQUAL(addr.ToString(), "1.2.3.4");
        ::Save(&str, addr);

        TAddr addr1;
        ::Load(&str, addr1);
        UNIT_ASSERT_EQUAL(addr1.ToString(), "1.2.3.4");
        UNIT_ASSERT_EQUAL(addr1.Len(), addr.Len());
        UNIT_ASSERT(!memcmp(addr1.Addr(), addr.Addr(), sizeof(sockaddr_storage)));
        UNIT_ASSERT(addr1 == addr);
        UNIT_ASSERT(!(addr1 != addr));
    }

    Y_UNIT_TEST(SaveLoadIp6) {
        TStringStream str;
        TAddr addr("abcd:cdab::f10e");
        UNIT_ASSERT(addr.Valid());
        UNIT_ASSERT_EQUAL(addr.ToString(), "abcd:cdab::f10e");
        ::Save(&str, addr);

        TAddr addr1;
        UNIT_ASSERT(!addr1.Valid());
        ::Load(&str, addr1);
        UNIT_ASSERT(addr1.Valid());
        UNIT_ASSERT_EQUAL(addr1.ToString(), "abcd:cdab::f10e");
        UNIT_ASSERT_EQUAL(addr1.Len(), addr.Len());
        UNIT_ASSERT(!memcmp(addr1.Addr(), addr.Addr(), sizeof(sockaddr_storage)));
        UNIT_ASSERT(addr1 == addr);
        UNIT_ASSERT(!(addr1 != addr));
    }

    Y_UNIT_TEST(AsIp) {
        TAddr addr1("1.2.3.4");
        UNIT_ASSERT_EQUAL(addr1.AsIp(), StrToIp("1.2.3.4"));
        UNIT_ASSERT_UNEQUAL(addr1.AsIp(), StrToIp("1.2.3.5"));
    }

    Y_UNIT_TEST(Compare) {
        TAddr addr1("abcd:cdab::f10e");
        TAddr addr2("abcd:cdab::f10e");
        TAddr addr3("abcd:cdab::f10f");
        UNIT_ASSERT(addr1.Valid());
        UNIT_ASSERT(addr1 == addr2);
        UNIT_ASSERT(addr1 != addr3);

        TAddr addr4("1.2.3.4");
        TAddr addr5("1.2.3.4");
        TAddr addr6("1.2.3.5");
        UNIT_ASSERT(addr4.Valid());
        UNIT_ASSERT(addr4 == addr5);
        UNIT_ASSERT(addr4 != addr6);

        UNIT_ASSERT(addr1 != addr4);
    }

    Y_UNIT_TEST(IsMax) {
        UNIT_ASSERT(TAddr("255.255.255.255").IsMax());

        const TVector<char> max6(16, 0xFF);
        const TStringBuf max6StrBuf(max6.data(), max6.size());
        UNIT_ASSERT(TAddr::FromIp6(max6StrBuf).IsMax());
    }

    Y_UNIT_TEST(IntervalForMask4) {
        TAddr addr("123.235.121.217");
        TAddr addr1;
        TAddr addr2;
        addr.GetIntervalForMask(33, addr1, addr2);
        UNIT_ASSERT_EQUAL(addr1, TAddr("123.235.121.217"));
        UNIT_ASSERT_EQUAL(addr2, TAddr("123.235.121.217"));
        addr.GetIntervalForMask(32, addr1, addr2);
        UNIT_ASSERT_EQUAL(addr1, TAddr("123.235.121.217"));
        UNIT_ASSERT_EQUAL(addr2, TAddr("123.235.121.217"));
        addr.GetIntervalForMask(31, addr1, addr2);
        UNIT_ASSERT_EQUAL(addr1, TAddr("123.235.121.216"));
        UNIT_ASSERT_EQUAL(addr2, TAddr("123.235.121.217"));
        addr.GetIntervalForMask(28, addr1, addr2);
        UNIT_ASSERT_EQUAL(addr1, TAddr("123.235.121.208"));
        UNIT_ASSERT_EQUAL(addr2, TAddr("123.235.121.223"));
        addr.GetIntervalForMask(24, addr1, addr2);
        UNIT_ASSERT_EQUAL(addr1, TAddr("123.235.121.0"));
        UNIT_ASSERT_EQUAL(addr2, TAddr("123.235.121.255"));
        addr.GetIntervalForMask(22, addr1, addr2);
        UNIT_ASSERT_EQUAL(addr1, TAddr("123.235.120.0"));
        UNIT_ASSERT_EQUAL(addr2, TAddr("123.235.123.255"));
        addr.GetIntervalForMask(16, addr1, addr2);
        UNIT_ASSERT_EQUAL(addr1, TAddr("123.235.0.0"));
        UNIT_ASSERT_EQUAL(addr2, TAddr("123.235.255.255"));
        addr.GetIntervalForMask(8, addr1, addr2);
        UNIT_ASSERT_EQUAL(addr1, TAddr("123.0.0.0"));
        UNIT_ASSERT_EQUAL(addr2, TAddr("123.255.255.255"));
        addr.GetIntervalForMask(1, addr1, addr2);
        UNIT_ASSERT_EQUAL(addr1, TAddr("0.0.0.0"));
        UNIT_ASSERT_EQUAL(addr2, TAddr("127.255.255.255"));
        addr.GetIntervalForMask(0, addr1, addr2);
        UNIT_ASSERT_EQUAL(addr1, TAddr("0.0.0.0"));
        UNIT_ASSERT_EQUAL(addr2, TAddr("255.255.255.255"));
    }

    Y_UNIT_TEST(IntervalForMask6) {
        TAddr addr("1234:5678:9abc:def1:1fed:cba9:8765:4321");
        TAddr addr1;
        TAddr addr2;
        addr.GetIntervalForMask(129, addr1, addr2);
        UNIT_ASSERT_EQUAL(addr1, TAddr("1234:5678:9abc:def1:1fed:cba9:8765:4321"));
        UNIT_ASSERT_EQUAL(addr2, TAddr("1234:5678:9abc:def1:1fed:cba9:8765:4321"));
        addr.GetIntervalForMask(128, addr1, addr2);
        UNIT_ASSERT_EQUAL(addr1, TAddr("1234:5678:9abc:def1:1fed:cba9:8765:4321"));
        UNIT_ASSERT_EQUAL(addr2, TAddr("1234:5678:9abc:def1:1fed:cba9:8765:4321"));
        addr.GetIntervalForMask(127, addr1, addr2);
        UNIT_ASSERT_EQUAL(addr1, TAddr("1234:5678:9abc:def1:1fed:cba9:8765:4320"));
        UNIT_ASSERT_EQUAL(addr2, TAddr("1234:5678:9abc:def1:1fed:cba9:8765:4321"));
        addr.GetIntervalForMask(124, addr1, addr2);
        UNIT_ASSERT_EQUAL(addr1, TAddr("1234:5678:9abc:def1:1fed:cba9:8765:4320"));
        UNIT_ASSERT_EQUAL(addr2, TAddr("1234:5678:9abc:def1:1fed:cba9:8765:432f"));
        addr.GetIntervalForMask(80, addr1, addr2);
        UNIT_ASSERT_EQUAL(addr1, TAddr("1234:5678:9abc:def1:1fed::"));
        UNIT_ASSERT_EQUAL(addr2, TAddr("1234:5678:9abc:def1:1fed:ffff:ffff:ffff"));
        addr.GetIntervalForMask(65, addr1, addr2);
        UNIT_ASSERT_EQUAL(addr1, TAddr("1234:5678:9abc:def1::"));
        UNIT_ASSERT_EQUAL(addr2, TAddr("1234:5678:9abc:def1:7fff:ffff:ffff:ffff"));
        addr.GetIntervalForMask(64, addr1, addr2);
        UNIT_ASSERT_EQUAL(addr1, TAddr("1234:5678:9abc:def1::"));
        UNIT_ASSERT_EQUAL(addr2, TAddr("1234:5678:9abc:def1:ffff:ffff:ffff:ffff"));
        addr.GetIntervalForMask(63, addr1, addr2);
        UNIT_ASSERT_EQUAL(addr1, TAddr("1234:5678:9abc:def0::"));
        UNIT_ASSERT_EQUAL(addr2, TAddr("1234:5678:9abc:def1:ffff:ffff:ffff:ffff"));
        addr.GetIntervalForMask(34, addr1, addr2);
        UNIT_ASSERT_EQUAL(addr1, TAddr("1234:5678:8000::"));
        UNIT_ASSERT_EQUAL(addr2, TAddr("1234:5678:bfff:ffff:ffff:ffff:ffff:ffff"));
        addr.GetIntervalForMask(32, addr1, addr2);
        UNIT_ASSERT_EQUAL(addr1, TAddr("1234:5678::"));
        UNIT_ASSERT_EQUAL(addr2, TAddr("1234:5678:ffff:ffff:ffff:ffff:ffff:ffff"));
        addr.GetIntervalForMask(16, addr1, addr2);
        UNIT_ASSERT_EQUAL(addr1, TAddr("1234::"));
        UNIT_ASSERT_EQUAL(addr2, TAddr("1234:ffff:ffff:ffff:ffff:ffff:ffff:ffff"));
        addr.GetIntervalForMask(4, addr1, addr2);
        UNIT_ASSERT_EQUAL(addr1, TAddr("1000::"));
        UNIT_ASSERT_EQUAL(addr2, TAddr("1fff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"));
        addr.GetIntervalForMask(1, addr1, addr2);
        UNIT_ASSERT_EQUAL(addr1, TAddr("::"));
        UNIT_ASSERT_EQUAL(addr2, TAddr("7fff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"));
        addr.GetIntervalForMask(0, addr1, addr2);
        UNIT_ASSERT_EQUAL(addr1, TAddr("::"));
        UNIT_ASSERT_EQUAL(addr2, TAddr("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff"));
    }

    static const std::array<std::pair<const char*, const char*>, 4> NEXT_PREV_TEST_CASES = {{
        {"1.2.3.4", "1.2.3.5"},
        {"abcd::1234:5678", "abcd::1234:5679"},
        {"abcd::", "abcd::1"},
        {"abcd:ffff:ffff:ffff:ffff:ffff:ffff:ffff", "abce::"}
    }};

    Y_UNIT_TEST(Next) {
        for (const auto& [a, b] : NEXT_PREV_TEST_CASES) {
            UNIT_ASSERT_EQUAL(TAddr(a).Next(), TAddr(b));
        }
    }

    Y_UNIT_TEST(Prev) {
        for (const auto& [a, b] : NEXT_PREV_TEST_CASES) {
            UNIT_ASSERT_EQUAL(TAddr(a), TAddr(b).Prev());
        }
    }

    Y_UNIT_TEST(AddrDistance) {
        UNIT_ASSERT_EQUAL(0, CalcIpDistance(TAddr(), TAddr()));

        UNIT_ASSERT_EQUAL(1, CalcIpDistance(TAddr("127.127.127.127"), TAddr("127.127.127.126")));

        UNIT_ASSERT_EQUAL(31, CalcIpDistance(TAddr("127.127.127.127"), TAddr()));
        UNIT_ASSERT_EQUAL(31, CalcIpDistance(TAddr(), TAddr("127.127.127.127")));

        UNIT_ASSERT_EQUAL(0, CalcIpDistance(TAddr("::FFFF:7F7F:7F7F"), TAddr("127.127.127.127")));
        UNIT_ASSERT_EQUAL(0, CalcIpDistance(TAddr("127.127.127.127"), TAddr("::FFFF:7F7F:7F7F")));

        UNIT_ASSERT_EQUAL(128, CalcIpDistance(TAddr("FFFF::FFFF:7F7F:7F7F"), TAddr("127.127.127.127")));

        UNIT_ASSERT_EQUAL(27, CalcIpDistance(TAddr("::7FF1:7F7F:7F7F"), TAddr("::7FF5:7F7F:7F7F")));
        UNIT_ASSERT_EQUAL(27, CalcIpDistance(TAddr("::7FF5:7F7F:7F7F"), TAddr("::7FF1:7F7F:7F7F")));

        UNIT_ASSERT_EQUAL(47, CalcIpDistance(TAddr("::7FF1:7F7F:7F7F"), TAddr()));
        UNIT_ASSERT_EQUAL(47, CalcIpDistance(TAddr(), TAddr("::7FF1:7F7F:7F7F")));

        UNIT_ASSERT_EQUAL(101, CalcIpDistance(TAddr("FFFF:1F00::FFFF:7F7F:7F7F"), TAddr("FFFF:1F10::FFFF:7F7F:7F7F")));
    }

    Y_UNIT_TEST(IPv4MappedToIPv6) {
        TAddr addr("::ffff:201.102.203.104");
        UNIT_ASSERT_VALUES_EQUAL(int(AF_INET), int(addr.GetFamily()));
        UNIT_ASSERT_VALUES_EQUAL(addr, TAddr("201.102.203.104"));

        UNIT_ASSERT_VALUES_EQUAL(TAddr("::ffff:1.2.3.4"), TAddr("1.2.3.4"));
        UNIT_ASSERT_VALUES_EQUAL(TAddr("::FffF:0102:0304"), TAddr("1.2.3.4"));
    }

    Y_UNIT_TEST(IpVersion) {
        UNIT_ASSERT(TAddr("1.2.3.4").IsIp4());
        UNIT_ASSERT(!TAddr("1234:5678:9abc:def1:1fed:cba9:8765:4321").IsIp4());
        UNIT_ASSERT(!TAddr("::1.2.3.4").IsIp4());

        UNIT_ASSERT(!TAddr("1.2.3.4").IsIp6());
        UNIT_ASSERT(TAddr("1234:5678:9abc:def1:1fed:cba9:8765:4321").IsIp6());
        UNIT_ASSERT(TAddr("::1.2.3.4").IsIp6());
    }

    Y_UNIT_TEST(Subnet4) {
        const TAddr testIp4Addr("255.127.63.31");
        const TAddr subnets[] = {
            {"0.0.0.0"},
            {"128.0.0.0"},
            {"192.0.0.0"},
            {"224.0.0.0"},
            {"240.0.0.0"},
            {"248.0.0.0"},
            {"252.0.0.0"},
            {"254.0.0.0"},
            {"255.0.0.0"},
            {"255.0.0.0"},
            {"255.64.0.0"},
            {"255.96.0.0"},
            {"255.112.0.0"},
            {"255.120.0.0"},
            {"255.124.0.0"},
            {"255.126.0.0"},
            {"255.127.0.0"},
            {"255.127.0.0"},
            {"255.127.0.0"},
            {"255.127.32.0"},
            {"255.127.48.0"},
            {"255.127.56.0"},
            {"255.127.60.0"},
            {"255.127.62.0"},
            {"255.127.63.0"},
            {"255.127.63.0"},
            {"255.127.63.0"},
            {"255.127.63.0"},
            {"255.127.63.16"},
            {"255.127.63.24"},
            {"255.127.63.28"},
            {"255.127.63.30"},
            {"255.127.63.31"},
        };
        for (size_t subnetBits = 0; subnetBits < Y_ARRAY_SIZE(subnets); ++subnetBits) {
            UNIT_ASSERT_VALUES_EQUAL(testIp4Addr.GetSubnet(subnetBits), subnets[subnetBits]);
        }
    }

    Y_UNIT_TEST(Subnet6) {
        const TAddr testIp6Addr("2a02:17d0:2c0:cb12:abcd:1234:dead:beef");
        const std::pair<size_t, TAddr> subnets[] = {
            {0, {"::"}},
            {3, {"2000::"}},
            {8, {"2a00::"}},
            {14, {"2a00::"}},
            {16, {"2a02::"}},
            {23, {"2a02:1600::"}},
            {24, {"2a02:1700::"}},
            {64, {"2a02:17d0:2c0:cb12::"}},
            {65, {"2a02:17d0:2c0:cb12:8000::"}},
            {112, {"2a02:17d0:2c0:cb12:abcd:1234:dead:0000"}},
            {128, {"2a02:17d0:2c0:cb12:abcd:1234:dead:beef"}},
        };
        for (auto subnet : subnets) {
            UNIT_ASSERT_VALUES_EQUAL(testIp6Addr.GetSubnet(subnet.first), subnet.second);
        }
    }

    Y_UNIT_TEST(CheckProjId) {
        const TAddr testIp6Addr("2a02:17d0:2c0:cb12:abcd:1234:dead:beef");
        ui32 validProjId = HostToInet(0xABCD1234);
        UNIT_ASSERT(testIp6Addr.GetProjId() == validProjId);
    }
}

