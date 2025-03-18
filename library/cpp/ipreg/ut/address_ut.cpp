#include <library/cpp/ipreg/address.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/int128/int128.h>
#include <util/string/vector.h>
#include <util/string/hex.h>

#include <string>
#include <vector>

using namespace NIPREG;

namespace {
    const auto& IP6_FIRST = TStringBuf("0000:0000:0000:0000:0000:0000:0000:0000");
    const auto& IP6_LAST  = TStringBuf("ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff");

    std::string GetHexString(const void* data, size_t amount) {
        return HexEncode(TStringBuf(static_cast<const char*>(data), amount));
    }

    TString GetNumStringByIp(const char* ip) {
        const auto& addr = TAddress::ParseAny(ip);
        Cerr << ip << " => " << addr.GetHexString() << "\n";
        return ToString(addr.AsUint128());
    }

    ui128 GetUint128ByIp(const char* ip) {
        const bool DEEP_VIEW = true;
        const auto& addr = TAddress::ParseAny(ip);
        Cerr << ip << " => " << addr.GetHexString(DEEP_VIEW) << "\n";
        return addr.AsUint128();
    }
} // anon-ns

Y_UNIT_TEST_SUITE(AddressTest) {
    Y_UNIT_TEST(AddressParseSimple) {
        UNIT_ASSERT_STRINGS_EQUAL(
            TAddress::FromIPv4Num(37486592).AsIPv6(),
            "0000:0000:0000:0000:0000:ffff:023c:0000"
        );

        UNIT_ASSERT_STRINGS_EQUAL(
            TAddress::ParseIPv4Num("37486592").AsIPv6(),
            "0000:0000:0000:0000:0000:ffff:023c:0000"
        );

        UNIT_ASSERT_STRINGS_EQUAL(
            TAddress::ParseIPv4("18.52.86.120").AsIPv6(),
            "0000:0000:0000:0000:0000:ffff:1234:5678"
        );

        UNIT_ASSERT_STRINGS_EQUAL(
            TAddress::ParseIPv6("::").AsIPv6(),
            IP6_FIRST
        );

        UNIT_ASSERT_STRINGS_EQUAL(
            TAddress::ParseIPv6("::1").AsIPv6(),
            "0000:0000:0000:0000:0000:0000:0000:0001"
        );

        UNIT_ASSERT_STRINGS_EQUAL(
            TAddress::ParseIPv6("1234:1234:1234:1234:1234:1234:1234:1234").AsIPv6(),
            "1234:1234:1234:1234:1234:1234:1234:1234"
        );

        UNIT_ASSERT_STRINGS_EQUAL(
            TAddress::ParseIPv6("1::").AsIPv6(),
            "0001:0000:0000:0000:0000:0000:0000:0000"
        );

        UNIT_ASSERT_STRINGS_EQUAL(
            TAddress::ParseIPv6("1000::1").AsIPv6(),
            "1000:0000:0000:0000:0000:0000:0000:0001"
        );
    }

    Y_UNIT_TEST(AddressIPv4) {
        UNIT_ASSERT(!TAddress::ParseAny("::1").IsIPv4());
        UNIT_ASSERT(TAddress::ParseAny("0.0.0.1").IsIPv4());
        UNIT_ASSERT(TAddress::ParseAny("127.0.0.1").IsIPv4());
    }

    Y_UNIT_TEST(AddressFormats) {
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("0.0.0.1").AsIPv6(), "0000:0000:0000:0000:0000:ffff:0000:0001");
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("::1").AsIPv6(), "0000:0000:0000:0000:0000:0000:0000:0001");
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("::ffff:0:1").AsIPv6(), "0000:0000:0000:0000:0000:ffff:0000:0001");

        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("0.0.0.1").AsLongIP(), "0.0.0.1");
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("::1").AsLongIP(), "0000:0000:0000:0000:0000:0000:0000:0001");
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("::ffff:0:1").AsLongIP(), "0.0.0.1");

        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("0.0.0.1").AsShortIP(), "0.0.0.1");
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("::1").AsShortIP(), "::1");
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("::ffff:0:1").AsShortIP(), "0.0.0.1");

        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("0.0.0.1").GetTextFromNetOrder(), "::ffff:0.0.0.1");
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("::1").GetTextFromNetOrder(), "::1");
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("::ffff:0:1").GetTextFromNetOrder(), "::ffff:0.0.0.1");

        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("0.0.0.1").AsShortIPv6(), "::ffff:0:1");
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("::1").AsShortIPv6(), "::1");
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("::ffff:0:1").AsShortIPv6(), "::ffff:0:1");

        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("0.0.0.1").AsIPv4Num(), "1");
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("::ffff:0:1").AsIPv4Num(), "1");
    }

    Y_UNIT_TEST(AddressCompare) {
        UNIT_ASSERT(TAddress::ParseIPv6("::1") == TAddress::ParseIPv6("::0001"));
        UNIT_ASSERT(TAddress::ParseIPv6("::1") != TAddress::ParseIPv6("::2"));
        UNIT_ASSERT(TAddress::ParseIPv6("::1") < TAddress::ParseIPv6("::2"));
        UNIT_ASSERT(TAddress::ParseIPv6("::1") <= TAddress::ParseIPv6("::2"));
        UNIT_ASSERT(TAddress::ParseIPv6("::1") <= TAddress::ParseIPv6("::1"));
        UNIT_ASSERT(TAddress::ParseIPv6("::2") > TAddress::ParseIPv6("::1"));
        UNIT_ASSERT(TAddress::ParseIPv6("::2") >= TAddress::ParseIPv6("::1"));
        UNIT_ASSERT(TAddress::ParseIPv6("::1") >= TAddress::ParseIPv6("::1"));
    }

    Y_UNIT_TEST(AddressLowest) {
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::Lowest().AsShortIPv6(), "::");
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::Lowest().AsIPv6(), IP6_FIRST);
        UNIT_ASSERT(TAddress::Lowest() == TAddress::ParseAny(IP6_FIRST));
    }

    Y_UNIT_TEST(AddressHighest) {
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::Highest().AsIPv6(), IP6_LAST);
        UNIT_ASSERT(TAddress::Highest() == TAddress::ParseAny(IP6_LAST));
    }

    Y_UNIT_TEST(AddressNext) {
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("::0").Next().AsIPv6(), "0000:0000:0000:0000:0000:0000:0000:0001");
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("::fffe:ffff:ffff").Next().AsShortIPv6(), "::ffff:0:0");
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("::ff").Next().AsIPv6(), "0000:0000:0000:0000:0000:0000:0000:0100");
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("::100").Next().AsIPv6(), "0000:0000:0000:0000:0000:0000:0000:0101");
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("::ffff").Next().AsIPv6(), "0000:0000:0000:0000:0000:0000:0001:0000");
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("::1:0000").Next().AsIPv6(), "0000:0000:0000:0000:0000:0000:0001:0001");
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("::1:0000").Next().AsIPv6(), "0000:0000:0000:0000:0000:0000:0001:0001");
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny(IP6_LAST).Next().AsIPv6(), IP6_LAST);
    }

    Y_UNIT_TEST(AddressPrev) {
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("::").Prev().AsIPv6(), IP6_FIRST);
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("::1").Prev().AsIPv6(), IP6_FIRST);
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("::100").Prev().AsIPv6(), "0000:0000:0000:0000:0000:0000:0000:00ff");
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("::101").Prev().AsIPv6(), "0000:0000:0000:0000:0000:0000:0000:0100");
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("::1:0").Prev().AsIPv6(), "0000:0000:0000:0000:0000:0000:0000:ffff");
        UNIT_ASSERT_STRINGS_EQUAL(TAddress::ParseAny("::1:1").Prev().AsIPv6(), "0000:0000:0000:0000:0000:0000:0001:0000");
    }

    Y_UNIT_TEST(AddressDistance) {
        UNIT_ASSERT_EQUAL(TAddress::Distance(TAddress::ParseAny("0.0.0.0"), TAddress::ParseAny("255.255.255.255")), std::numeric_limits<uint32_t>::max());

        UNIT_ASSERT_EQUAL(TAddress::Distance(TAddress::ParseAny("::"), TAddress::ParseAny("::")), 0);
        UNIT_ASSERT_EQUAL(TAddress::Distance(TAddress::ParseAny("::1"), TAddress::ParseAny("::2")), 1);
        UNIT_ASSERT_EQUAL(TAddress::Distance(TAddress::ParseAny("::2"), TAddress::ParseAny("::1")), 1);
        UNIT_ASSERT_EQUAL(TAddress::Distance(TAddress::ParseAny("::"), TAddress::ParseAny("::1:0")), 65536);
        UNIT_ASSERT_EQUAL(TAddress::Distance(TAddress::ParseAny("::1:0"), TAddress::ParseAny("::")), 65536);
        UNIT_ASSERT_EQUAL(TAddress::Distance(TAddress::ParseAny("::"), TAddress::ParseAny(IP6_LAST)), FromString<ui128>("340282366920938463463374607431768211455"));
        UNIT_ASSERT_EQUAL(TAddress::Distance(TAddress::ParseAny(IP6_LAST), TAddress::ParseAny("::")), FromString<ui128>("340282366920938463463374607431768211455"));

        UNIT_ASSERT_EQUAL(TAddress::Distance(TAddress::ParseAny("2a0b:a902::"), TAddress::ParseAny("2a0b:a905:ffff:ffff:ffff:ffff:ffff:ffff")), FromString<ui128>("316912650057057350374175801343"));
    }

    Y_UNIT_TEST(Check_Ui128) {
        UNIT_ASSERT_STRINGS_EQUAL("0", GetNumStringByIp("::"));

        const auto ALL_BITS_ON = ~ui128{};
        UNIT_ASSERT_EQUAL(ALL_BITS_ON, GetUint128ByIp(IP6_LAST.data()));

        const auto LEAST_SIGN_BIT_ON = ui128{1U};
        const auto LEAST_SIGN_BIT_ADDR = "::1";
        UNIT_ASSERT_EQUAL(LEAST_SIGN_BIT_ON, GetUint128ByIp(LEAST_SIGN_BIT_ADDR));
        UNIT_ASSERT_EQUAL(LEAST_SIGN_BIT_ADDR, TAddress::FromUint128(LEAST_SIGN_BIT_ON).AsShortIPv6());

        const auto BIT_31_ON = ui128{1} << 31;
        const auto BIT_31_ON_ADDR = "::8000:0";
        UNIT_ASSERT_EQUAL(BIT_31_ON, GetUint128ByIp(BIT_31_ON_ADDR));
        UNIT_ASSERT_EQUAL(TAddress::ParseAny(BIT_31_ON_ADDR), TAddress::FromUint128(BIT_31_ON));

        const auto MOST_SIGN_BIT_ON = (ui128{1U} << 127);
        const auto MOST_SIGN_BIT_ADDR = "8000::";
        UNIT_ASSERT_EQUAL(MOST_SIGN_BIT_ON, GetUint128ByIp(MOST_SIGN_BIT_ADDR));
        UNIT_ASSERT_EQUAL(MOST_SIGN_BIT_ADDR, TAddress::FromUint128(MOST_SIGN_BIT_ON).AsShortIPv6());

        const auto IPV4_PREFIX = ui128{0xffff} << 32;
        const auto IPV4_PREFIX_ADDR = "::ffff:0:0";
        UNIT_ASSERT_EQUAL(IPV4_PREFIX, GetUint128ByIp(IPV4_PREFIX_ADDR));
        UNIT_ASSERT_EQUAL(TAddress::ParseAny(IPV4_PREFIX_ADDR), TAddress::FromUint128(IPV4_PREFIX));
    }

    Y_UNIT_TEST(ParseBadNetwork) {
        const std::vector<std::string> badNets = {
            "0",
            "/0",
            "5/33",
            "1.0.0.0",
            "1.0.0.0/35",
            "bad-net",
            "aa.dd.bb.cc",
            "aa.dd.bb.cc/8",
            "::",
            "1:2:3::/131"
        };

        for (const auto& addr : badNets) {
            Cerr << "\t" << addr << "...\n";

            UNIT_ASSERT_EXCEPTION(TNetwork(addr.c_str()), std::exception);
        }
    }

    Y_UNIT_TEST(ParseNetworks) {
        struct NetTraits {
            std::string Net;
            std::string IpBegin;
            std::string IpEnd;
        };

        const std::vector<NetTraits> nets = {
            { "0.0.0.0/32", "0.0.0.0", "0.0.0.0" },
            { "1.0.0.0/26", "1.0.0.0", "1.0.0.63" },
            { "1.0.201.77/32", "1.0.201.77", "1.0.201.77" },
            { "1.0.201.100/31", "1.0.201.100", "1.0.201.101" },
            { "1.0.201.72/30", "1.0.201.72", "1.0.201.75" },
            { "223.255.247.128/25", "223.255.247.128", "223.255.247.255" },
            { "600:8801:9400:5a1:948b:ab15:dde3:61a3/128", "600:8801:9400:5a1:948b:ab15:dde3:61a3", "600:8801:9400:5a1:948b:ab15:dde3:61a3" },
            { "2001:200::/49", "2001:200::", "2001:200:0:7fff:ffff:ffff:ffff:ffff" },
            { "2c0f:ffd8:4000::/34", "2c0f:ffd8:4000::", "2c0f:ffd8:7fff:ffff:ffff:ffff:ffff:ffff" },
            { "::ffff:0:0/96", "0.0.0.0", "255.255.255.255" }
        };

        for (const auto& traits : nets) {
            const TNetwork range(traits.Net.c_str());

            Cerr << "\t" << traits.Net << " => " << GetHexString(range.begin.Data,  16) << " / "
                                                 << GetHexString(range.end.Data, 16) << "\n";

            UNIT_ASSERT_EQUAL(TAddress::ParseAny(traits.IpBegin), range.begin);
            UNIT_ASSERT_EQUAL(TAddress::ParseAny(traits.IpEnd), range.end);
        }
    }

    Y_UNIT_TEST(BinaryChecks) {
        unsigned char minIp6[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
        unsigned char maxIp6[] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
        unsigned char ip4in6[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0x01,0x02,0x03,0x04};
        unsigned char ip4[] = {0x4d,0x4b,0x9b,0x03};

        UNIT_ASSERT_EQUAL(TAddress::FromBinary(minIp6).AsShortIP(), "::");
        UNIT_ASSERT_EQUAL(TAddress::FromBinary(maxIp6).AsShortIP(), IP6_LAST);
        UNIT_ASSERT_EQUAL(TAddress::FromBinary(ip4in6).AsShortIP(), "1.2.3.4");
        UNIT_ASSERT_EQUAL(TAddress::FromBinaryIPv4(ip4).AsShortIP(), "77.75.155.3");
        UNIT_ASSERT_EQUAL(TAddress::FromBinaryIPv4(minIp6).AsShortIP(), "0.0.0.0");
        UNIT_ASSERT_EQUAL(TAddress::FromBinaryIPv4(maxIp6).AsShortIP(), "255.255.255.255");
    }
} // AddressTest
