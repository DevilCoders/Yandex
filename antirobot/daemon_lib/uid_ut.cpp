#include "uid.h"

#include <antirobot/lib/addr.h>
#include <antirobot/lib/ar_utils.h>
#include <antirobot/lib/spravka.h>
#include <antirobot/lib/preview_uid.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAntiRobot;

Y_UNIT_TEST_SUITE(TUserIdentity) {
    Y_UNIT_TEST(Aggr) {
        UNIT_ASSERT_VALUES_EQUAL(TUid::FromIpAggregation(TAddr("1.2.3.4"), 1), TUid::FromIpAggregation(TAddr("1.2.3.5"), 1));
        UNIT_ASSERT_VALUES_EQUAL(TUid::FromIpAggregation(TAddr("1.2.3.4"), 1).Hash(), TUid::FromIpAggregation(TAddr("1.2.3.5"), 1).Hash());

        UNIT_ASSERT_VALUES_UNEQUAL(TUid::FromIpAggregation(TAddr("1.2.3.4"), 1), TUid::FromIpAggregation(TAddr("1.2.4.4"), 1));
        UNIT_ASSERT_VALUES_UNEQUAL(TUid::FromIpAggregation(TAddr("1.2.3.4"), 1).Hash(), TUid::FromIpAggregation(TAddr("1.2.4.4"), 1).Hash());

        UNIT_ASSERT_VALUES_EQUAL(TUid::FromIpAggregation(TAddr("1.2.3.4"), 2), TUid::FromIpAggregation(TAddr("1.2.4.5"), 2));
        UNIT_ASSERT_VALUES_EQUAL(TUid::FromIpAggregation(TAddr("1.2.3.4"), 2).Hash(), TUid::FromIpAggregation(TAddr("1.2.4.5"), 2).Hash());

        UNIT_ASSERT_VALUES_UNEQUAL(TUid::FromIpAggregation(TAddr("1.2.3.4"), 2), TUid::FromIpAggregation(TAddr("1.3.3.4"), 2));
        UNIT_ASSERT_VALUES_UNEQUAL(TUid::FromIpAggregation(TAddr("1.2.3.4"), 2).Hash(), TUid::FromIpAggregation(TAddr("1.3.3.4"), 2).Hash());

        UNIT_ASSERT_VALUES_UNEQUAL(TUid::FromIpAggregation(TAddr("1.2.3.4"), 1).Hash(), TUid::FromIpAggregation(TAddr("1.2.3.4"), 2).Hash());
        UNIT_ASSERT_VALUES_UNEQUAL(TUid::FromIpAggregation(TAddr("1.2.0.0"), 1).Hash(), TUid::FromIpAggregation(TAddr("1.2.0.0"), 2).Hash());

        UNIT_ASSERT_VALUES_EQUAL(TUid::FromIpAggregation(TAddr("1234:5678:9abc:def1:1fed:cba9:8765:4321"), 0),
                                 TUid::FromIpAggregation(TAddr("1234:5678:9abc:def1:2fed:cba9:8765:4321"), 0));
        UNIT_ASSERT_VALUES_EQUAL(TUid::FromIpAggregation(TAddr("1234:5678:9abc:def1:1fed:cba9:8765:4321"), 0).Hash(),
                                 TUid::FromIpAggregation(TAddr("1234:5678:9abc:def1:2fed:cba9:8765:4321"), 0).Hash());

        UNIT_ASSERT_VALUES_UNEQUAL(TUid::FromIpAggregation(TAddr("1234:5678:9abc:def1:1fed:cba9:8765:4321"), 0),
                                   TUid::FromIpAggregation(TAddr("1234:5678:9abc:def2:1fed:cba9:8765:4321"), 0));
        UNIT_ASSERT_VALUES_UNEQUAL(TUid::FromIpAggregation(TAddr("1234:5678:9abc:def1:1fed:cba9:8765:4321"), 0).Hash(),
                                   TUid::FromIpAggregation(TAddr("1234:5678:9abc:def2:1fed:cba9:8765:4321"), 0).Hash());

        UNIT_ASSERT_VALUES_EQUAL(TUid::FromIpAggregation(TAddr("1234:5678:9abc:def1:1fed:cba9:8765:4321"), 1),
                                 TUid::FromIpAggregation(TAddr("1234:5678:9abc:dec1:1fed:cba9:8765:4321"), 1));
        UNIT_ASSERT_VALUES_EQUAL(TUid::FromIpAggregation(TAddr("1234:5678:9abc:def1:1fed:cba9:8765:4321"), 1).Hash(),
                                 TUid::FromIpAggregation(TAddr("1234:5678:9abc:dec1:1fed:cba9:8765:4321"), 1).Hash());

        UNIT_ASSERT_VALUES_UNEQUAL(TUid::FromIpAggregation(TAddr("1234:5678:9abc:def1:1fed:cba9:8765:4321"), 1),
                                   TUid::FromIpAggregation(TAddr("1234:5678:9abc:dcf1:1fed:cba9:8765:4321"), 1));
        UNIT_ASSERT_VALUES_UNEQUAL(TUid::FromIpAggregation(TAddr("1234:5678:9abc:def1:1fed:cba9:8765:4321"), 1).Hash(),
                                   TUid::FromIpAggregation(TAddr("1234:5678:9abc:dcf1:1fed:cba9:8765:4321"), 1).Hash());

        UNIT_ASSERT_VALUES_EQUAL(TUid::FromIpAggregation(TAddr("1234:5678:9abc:def1:2fed:cba9:8765:4321"), 2),
                                 TUid::FromIpAggregation(TAddr("1234:5678:9abc:cef1:2fed:cba9:8765:4321"), 2));
        UNIT_ASSERT_VALUES_EQUAL(TUid::FromIpAggregation(TAddr("1234:5678:9abc:def1:2fed:cba9:8765:4321"), 2).Hash(),
                                 TUid::FromIpAggregation(TAddr("1234:5678:9abc:cef1:2fed:cba9:8765:4321"), 2).Hash());

        UNIT_ASSERT_VALUES_UNEQUAL(TUid::FromIpAggregation(TAddr("1234:5678:9abc:def1:1fed:cba9:8765:4321"), 2),
                                   TUid::FromIpAggregation(TAddr("1234:5678:9abd:def1:1fed:cba9:8765:4321"), 2));
        UNIT_ASSERT_VALUES_UNEQUAL(TUid::FromIpAggregation(TAddr("1234:5678:9abc:def1:1fed:cba9:8765:4321"), 2).Hash(),
                                   TUid::FromIpAggregation(TAddr("1234:5678:9abd:def1:1fed:cba9:8765:4321"), 2).Hash());

        UNIT_ASSERT_VALUES_UNEQUAL(TUid::FromIpAggregation(TAddr("1234:5678:9abc:def1:1fed:cba9:8765:4321"), 1).Hash(),
                                   TUid::FromIpAggregation(TAddr("1234:5678:9abc:def1:1fed:cba9:9765:4321"), 2).Hash());
        UNIT_ASSERT_VALUES_UNEQUAL(TUid::FromIpAggregation(TAddr("1234:5678:9abc:def1::"), 1).Hash(),
                                   TUid::FromIpAggregation(TAddr("1234:5678:9abc:def1::"), 2).Hash());
    }

    Y_UNIT_TEST(NameSpace) {
        UNIT_ASSERT(TUid::FromAddr(TAddr("1.2.3.4")) != TUid::FromIpAggregation(TAddr("1.2.3.4"), 0));
        UNIT_ASSERT(TUid::FromAddr(TAddr("1234::4321")) != TUid::FromIpAggregation(TAddr("1234::4321"), 0));
    }

    Y_UNIT_TEST(AggregationLevel) {
        UNIT_ASSERT_EQUAL(TUid::FromIpAggregation(TAddr("1.2.2.4"), 2).GetAggregationLevel(), 2);
        UNIT_ASSERT_EQUAL(TUid::FromAddr(TAddr("1.2.3.4")).GetAggregationLevel(), -1);
        UNIT_ASSERT_EQUAL(TUid::FromIpAggregation(TAddr("1234::4321"), 2).GetAggregationLevel(), 2);
        UNIT_ASSERT_EQUAL(TUid::FromAddr(TAddr("1234::4321")).GetAggregationLevel(), -1);
        UNIT_ASSERT_EQUAL(TUid::FromJa3Aggregation("a-b-c").GetAggregationLevel(), 3);
    }

    Y_UNIT_TEST(Preview) {
        UNIT_ASSERT(TUid::FromAddrOrSubnetPreview(TAddr("1234:5678:9abc:def1:2fed:cba9:8765:4321"), EPreviewAgentType::OTHER).Ns == TUid::PREVIEW);
        UNIT_ASSERT(TUid::FromAddrOrSubnetPreview(TAddr("23.45.123.1"), EPreviewAgentType::OTHER).Ns == TUid::PREVIEW);
        UNIT_ASSERT_VALUES_EQUAL(TUid::FromAddrOrSubnetPreview(TAddr("1234:5678:9abc:def1:2fed:cba9:8765:4321"), EPreviewAgentType::OTHER),
                                 TUid::FromAddrOrSubnetPreview(TAddr("1234:5678:9abc:def1:2fed:cba9:8765:1234"), EPreviewAgentType::OTHER));
        UNIT_ASSERT_VALUES_UNEQUAL(TUid::FromAddrOrSubnetPreview(TAddr("1234:5678:9abc:def1:2fed:cba9:8765:4321"), EPreviewAgentType::OTHER),
                                   TUid::FromAddrOrSubnetPreview(TAddr("1234:5678:9abc:cef1:2fed:cba9:8765:4321"), EPreviewAgentType::UNKNOWN));
        UNIT_ASSERT_VALUES_UNEQUAL(TUid::FromAddrOrSubnetPreview(TAddr("1234:5678:9abc:def1:2fed:cba9:8765:4321"), EPreviewAgentType::OTHER),
                                   TUid::FromAddrOrSubnetPreview(TAddr("1234:5678:9abc:def1:2fed:cba9:8765:4321"), EPreviewAgentType::UNKNOWN));
        UNIT_ASSERT_VALUES_UNEQUAL(TUid::FromAddrOrSubnetPreview(TAddr("0000:0000:0000:0000:0000:0000:0000:0000"), EPreviewAgentType::OTHER),
                                   TUid::FromAddrOrSubnetPreview(TAddr("0.0.0.0"), EPreviewAgentType::OTHER));
    }

    Y_UNIT_TEST(Aggr_str) {
        UNIT_ASSERT(TUid::FromJa3Aggregation("v-v-v").Ns == TUid::AGGR_STR);
        UNIT_ASSERT_VALUES_UNEQUAL(TUid::FromJa3Aggregation("a-b-c"), TUid::FromJa3Aggregation("d-e-f"));
        UNIT_ASSERT_VALUES_EQUAL(TUid::FromJa3Aggregation("a-b-c"), TUid::FromJa3Aggregation("a-b-c"));
    }

    Y_UNIT_TEST(Equal) {
        UNIT_ASSERT_EQUAL(TUid(TAddr("1.2.3.4")), TUid(TAddr("1.2.3.4")));
        UNIT_ASSERT_EQUAL(TUid(TAddr("1.2.3.4")).Hash(), TUid(TAddr("1.2.3.4")).Hash());

        UNIT_ASSERT_UNEQUAL(TUid(TAddr("1.2.3.4")), TUid(TAddr("1.2.3.5")));
        UNIT_ASSERT_UNEQUAL(TUid(TAddr("1.2.3.4")).Hash(), TUid(TAddr("1.2.3.5")).Hash());

        UNIT_ASSERT_EQUAL(TUid(TAddr("abcd:cdab::f10e")), TUid(TAddr("abcd:cdab::f10e")));
        UNIT_ASSERT_EQUAL(TUid(TAddr("abcd:cdab::f10e")).Hash(), TUid(TAddr("abcd:cdab::f10e")).Hash());
        UNIT_ASSERT_EQUAL(TUid(TAddr("::ffff:1.2.3.4")), TUid(TAddr("1.2.3.4")));
        UNIT_ASSERT_EQUAL(TUid(TAddr("::ffff:1.2.3.4")).Hash(), TUid(TAddr("1.2.3.4")).Hash());

        UNIT_ASSERT_UNEQUAL(TUid(TAddr("abcd:cdab::f10e")), TUid(TAddr("abcd:cdab::f10f")));
        UNIT_ASSERT_UNEQUAL(TUid(TAddr("abcd:cdab::f10e")).Hash(), TUid(TAddr("abcd:cdab::f10f")).Hash());

        UNIT_ASSERT_UNEQUAL(TUid(TAddr("abcd:cdab::f10e")), TUid(TAddr("bbcd:cdab::f10e")));
        UNIT_ASSERT_UNEQUAL(TUid(TAddr("abcd:cdab::f10e")).Hash(), TUid(TAddr("bbcd:cdab::f10e")).Hash());


        UNIT_ASSERT_EQUAL(TUid(TUid::AGGR, 0, 1), TUid(TUid::AGGR, 0, 1));
        UNIT_ASSERT_EQUAL(TUid(TUid::AGGR, 0, 1).Hash(), TUid(TUid::AGGR, 0, 1).Hash());

        UNIT_ASSERT_UNEQUAL(TUid(TUid::AGGR, 0, 1), TUid(TUid::AGGR, 0, 2));
        UNIT_ASSERT_UNEQUAL(TUid(TUid::AGGR, 0, 1).Hash(), TUid(TUid::AGGR, 0, 2).Hash());

        UNIT_ASSERT_UNEQUAL(TUid(TUid::AGGR, 0, 1), TUid(TUid::AGGR, 1, 1));
        UNIT_ASSERT_UNEQUAL(TUid(TUid::AGGR, 0, 1).Hash(), TUid(TUid::AGGR, 1, 1).Hash());

        UNIT_ASSERT_UNEQUAL(TUid(TUid::AGGR, 0, 1), TUid(TUid::IP, 0, 1));
        UNIT_ASSERT_UNEQUAL(TUid(TUid::AGGR, 0, 1).Hash(), TUid(TUid::IP, 0, 1).Hash());

        UNIT_ASSERT_EQUAL(TUid::FromSubnet(TAddr("1.2.3.4"), 32), TUid(TAddr("1.2.3.4")));
        UNIT_ASSERT_EQUAL(TUid::FromSubnet(TAddr("1.2.3.4"), 32).Hash(), TUid(TAddr("1.2.3.4")).Hash());

        UNIT_ASSERT_EQUAL(TUid::FromSubnet(TAddr("2a02:6b8:0:408:14e4:fc62:ea13:b8a1"), 128),
            TUid(TAddr("2a02:6b8:0:408:14e4:fc62:ea13:b8a1")));
        UNIT_ASSERT_EQUAL(TUid::FromSubnet(TAddr("2a02:6b8:0:408:14e4:fc62:ea13:b8a1"), 128).Hash(),
            TUid(TAddr("2a02:6b8:0:408:14e4:fc62:ea13:b8a1")).Hash());

        UNIT_ASSERT_UNEQUAL(TUid::FromSubnet(TAddr("1.2.3.4"), 24), TUid(TAddr("1.2.3.4")));
        UNIT_ASSERT_UNEQUAL(TUid::FromSubnet(TAddr("1.2.3.4"), 24).Hash(), TUid(TAddr("1.2.3.4")).Hash());

        UNIT_ASSERT_EQUAL(TUid::FromSubnet(TAddr("1.2.3.4"), 24), TUid::FromSubnet(TAddr("1.2.3.5"), 24));
        UNIT_ASSERT_EQUAL(TUid::FromSubnet(TAddr("1.2.3.4"), 24).Hash(), TUid::FromSubnet(TAddr("1.2.3.5"), 24).Hash());

        UNIT_ASSERT_EQUAL(TUid::FromSubnet(TAddr("2a02:6b8:0:408:14e4:fc62:ea13:b8a1"), 64),
            TUid::FromSubnet(TAddr("2a02:6b8:0:408:aaaa:bbbb:cccc:dddd"), 64));
        UNIT_ASSERT_EQUAL(TUid::FromSubnet(TAddr("2a02:6b8:0:408:14e4:fc62:ea13:b8a1"), 64).Hash(),
            TUid::FromSubnet(TAddr("2a02:6b8:0:408:aaaa:bbbb:cccc:dddd"), 64).Hash());

        UNIT_ASSERT_UNEQUAL(TUid::FromSubnet(TAddr("2a02:6b8:0:408:14e4:fc62:ea13:b8a1"), 64),
            TUid::FromSubnet(TAddr("2a02:6b8:0:409:aaaa:bbbb:cccc:dddd"), 64));
        UNIT_ASSERT_UNEQUAL(TUid::FromSubnet(TAddr("2a02:6b8:0:408:14e4:fc62:ea13:b8a1"), 64).Hash(),
            TUid::FromSubnet(TAddr("2a02:6b8:0:409:aaaa:bbbb:cccc:dddd"), 64).Hash());

        UNIT_ASSERT_EQUAL(TUid::FromAddrOrSubnet(TAddr("1.2.3.4"), 16), TUid(TAddr("1.2.3.4")));
        UNIT_ASSERT_EQUAL(TUid::FromAddrOrSubnet(TAddr("1.2.3.4"), 16).Hash(), TUid(TAddr("1.2.3.4")).Hash());

        UNIT_ASSERT_EQUAL(TUid::FromAddrOrSubnet(TAddr("2a02:6b8:0:408:14e4:fc62:ea13:b8a1"), 64),
            TUid::FromSubnet(TAddr("2a02:6b8:0:408:aaaa:bbbb:cccc:dddd"), 64));
        UNIT_ASSERT_EQUAL(TUid::FromAddrOrSubnet(TAddr("2a02:6b8:0:408:14e4:fc62:ea13:b8a1"), 64).Hash(),
            TUid::FromSubnet(TAddr("2a02:6b8:0:408:aaaa:bbbb:cccc:dddd"), 64).Hash());
    }

    Y_UNIT_TEST(OutputEqual) {
        TStringStream uid1;
        uid1 << TUid(TAddr("1.2.3.4"));
        TStringStream uid2;
        uid2 << TUid(TAddr("1.2.3.4"));
        TStringStream uid3;
        uid3 << TUid(TAddr("1.2.3.5"));
        TStringStream uid4;
        uid4 << TUid(TAddr("2.2.3.4"));
        UNIT_ASSERT_EQUAL(uid1.Str(), uid2.Str());
        UNIT_ASSERT_UNEQUAL(uid1.Str(), uid3.Str());
        UNIT_ASSERT_UNEQUAL(uid1.Str(), uid4.Str());
        UNIT_ASSERT_UNEQUAL(uid3.Str(), uid4.Str());

        TStringStream uid5;
        uid5 << TUid(TAddr("abcd:ef01:1234:5678::f10e"));
        TStringStream uid6;
        uid6 << TUid(TAddr("abcd:ef01:1234:5678::f10e"));
        TStringStream uid7;
        uid7 << TUid(TAddr("abcd:ef01:1234:5678::f10f"));
        TStringStream uid8;
        uid8 << TUid(TAddr("bbcd:ef01:1234:5678::f10e"));
        UNIT_ASSERT_EQUAL(uid5.Str(), uid6.Str());
        UNIT_ASSERT_UNEQUAL(uid5.Str(), uid7.Str());
        UNIT_ASSERT_UNEQUAL(uid5.Str(), uid8.Str());
        UNIT_ASSERT_UNEQUAL(uid7.Str(), uid8.Str());
    }

    Y_UNIT_TEST(TestCorrectInput) {
        static const char* EXTRA_STUFF[] = {
            "",
            "sflkgs;iho ;dsg",
        };
        TSpravka::TDegradation degradation;
        static const TUid TEST_UIDS[] = {
            TUid::FromAddr(TAddr("1.2.3.4")),
            TUid::FromAddr(TAddr("1234:5678:9abc:def1:1fed:cba9:8765:4321")),
            TUid::FromIpAggregation(TAddr("1234:5678:9abc:def1:1fed:cba9:8765:4321"), 1),
            TUid::FromIpAggregation(TAddr("1.2.3.4"), 1),
            TUid(),
            TUid(TUid::AGGR, TAddr("192.168.0.1").AsIp(), 1),
            TUid(TUid::IP, TAddr("192.168.0.1").AsIp()),
            TUid::FromSpravka(TSpravka::Generate(TAddr("192.168.0.1"), "yandex.ru", degradation)),
            TUid::FromAddrOrSubnetPreview(TAddr("1.2.3.4"), 64, EPreviewAgentType::OTHER),
            TUid::FromAddrOrSubnetPreview(TAddr("1234:5678:9abc:def1:1fed:cba9:8765:4321"), 64, EPreviewAgentType::OTHER),
            TUid::FromJa3Aggregation("v-v-v"),
        };
        for (size_t j = 0; j < Y_ARRAY_SIZE(EXTRA_STUFF); ++j) {
            for (size_t i = 0; i < Y_ARRAY_SIZE(TEST_UIDS); ++i) {
                TStringStream ss;
                // add some stuff to make sure that loading is OK from string with other data
                ss << TEST_UIDS[i] << ' ' << EXTRA_STUFF[j];

                TUid uid;
                ss >> uid;
//                Cerr << TEST_UIDS[i] << " " << uid << Endl;
                UNIT_ASSERT_EQUAL_C(TEST_UIDS[i], uid, i);
            }
        }
    }

    Y_UNIT_TEST(TestWrongInput) {
        /*
         *  Correct data:
         *  1-16909060
         *  8-123456789ABCDEF11FEDCBA987654321
         *  9-123456789ABCDEF11FEDCBA900000000-1
         *  5-16909056-1
         *  0-0
         *  5-3232235521-1
         *  1-3232235521
         *  3-3232235521
         *  2-1364295194856963148
         *  11-16909060-1
         */
        static const TString DATA[] = { // in comments it is said what wrong in the data
            "1", // no dash
            "1-16909060-2", // aggregation level for IP namespace
            "8-123456789ABCDEF11FEDCBA98765432112", // 2 extra digits at the end
            "9-123456789ABCDEF11FEDCBA900000000-1bc", // 2 extra letters at the end
            "9-123456789ABCQEF11FEDCBA900000000-1", // symbol 'Q' in hex
            "5-16909056-", // no digits after the second dash
            "0-", // no digits after the dash
            "5-3232235521-1-1", // extra dash and digits at the end
            "19-3232235521", // too large namespace number
            "11-16909060-1-0", // extra dash and digits at the end
            "11-ABABA-1", // wrong digits
            "11-16909060", // w.o. IdType
        };

        for (size_t i = 0; i < Y_ARRAY_SIZE(DATA); ++i) {
            TStringInput in(DATA[i]);
            TUid uid;

            UNIT_ASSERT_EXCEPTION(in >> uid, TFromStringException);
        }
    }

    Y_UNIT_TEST(UidToAddr) {
        TAddr ip = TAddr("1.2.3.4");

        UNIT_ASSERT(TUid::FromAddr(ip).IpBased());
        UNIT_ASSERT_EQUAL(ip, TUid::FromAddr(ip).ToAddr());

        TAddr ip6 = TAddr("1234:5678:9abc:def1:1fed:cba9:8765:4321");
        UNIT_ASSERT(TUid::FromAddr(ip6).IpBased());
        UNIT_ASSERT_EQUAL(ip6, TUid::FromAddr(ip6).ToAddr());

        UNIT_ASSERT(!TUid::FromAddrOrSubnetPreview(TAddr("1.2.3.4"), 1, EPreviewAgentType::OTHER).IpBased());
        UNIT_ASSERT(!TUid::FromAddrOrSubnetPreview(TAddr("1234:5678:9abc:def1:1fed:cba9:8765:4321"), 1, EPreviewAgentType::OTHER).IpBased());
    }

}
