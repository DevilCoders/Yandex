#undef NDEBUG
#include <library/cpp/ipreg/address.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/deprecated/ipreg1/lookup.h>

#include <util/datetime/cputimer.h>
#include <util/generic/yexception.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/string/cast.h>
#include <util/string/split.h>

namespace {
    void PrintNet(const NIpreg::Net& net) {
        Cerr << "real-ip: " << net.RealIp
             << "; is_yandex: " << net.IsYandex
             << "; is_user: " << net.IsUser << "\n";
    }

    typedef TVector<TString> AddrListType;

    AddrListType LoadAddresses(const char* jsonFname) {
        TUnbufferedFileInput jsonFile(jsonFname);
        NJson::TJsonValue ranges;
        NJson::ReadJsonTree(&jsonFile, &ranges);

        if (!ranges.IsArray()) {
            ythrow yexception() << "failed to parse json";
        }

        AddrListType addrsList;

        const auto& ranges_arr = ranges.GetArray();
        const auto elems_amount = ranges_arr.size();
        for (size_t index = 0; index < elems_amount; ++index) {
            const auto& range = ranges_arr[index];

            addrsList.push_back(range["low"].GetString());
            addrsList.push_back(range["high"].GetString());
        }

        Y_ASSERT(addrsList.size());
        return addrsList;
    }

    TSimpleSharedPtr<NIpreg::TLookup> ipCheckerPtr;
}

Y_UNIT_TEST_SUITE(LookupTest) {
    Y_UNIT_TEST(NoFile) {
        try {
            NIpreg::TLookup l("its-non-existed-file-name");
            UNIT_ASSERT(false);
        } catch (const std::exception& ex) {
            Cerr << "NoFile >>> catch [" << ex.what() << "]\n";
        }
    }

    Y_UNIT_TEST(EmptyData) {
        const TString tmpFile = "empty-file";
        {
            TFixedBufferFileOutput file(tmpFile);
        }

        try {
            NIpreg::TLookup l(tmpFile.data());
            UNIT_ASSERT(false);
        } catch (const std::exception& ex) {
            Cerr << "EmptyData >>> catch [" << ex.what() << "]\n";
        }
    }

    Y_UNIT_TEST(NoNetsData) {
        const TString tmpJson = "[{'attr1':'value1'},{'attr2':'value2'}]";
        const TString tmpFile = "no-nets.json";
        {
            TFixedBufferFileOutput file(tmpFile);
            file << tmpJson.data();
        }

        try {
            NIpreg::TLookup l(tmpFile.data());
            UNIT_ASSERT(false);
        } catch (const std::exception& ex) {
            Cerr << "NoNetsData >>> catch [" << ex.what() << "]\n";
        }
    }

    Y_UNIT_TEST(LoadData) {
        UNIT_ASSERT(nullptr == ipCheckerPtr.Get());
        ipCheckerPtr.Reset(new NIpreg::TLookup("./ipreg-layout.json"));
        UNIT_ASSERT(ipCheckerPtr.Get());
    }

    Y_UNIT_TEST(IsYa) {
        UNIT_ASSERT(ipCheckerPtr.Get());
        UNIT_ASSERT(ipCheckerPtr->IsYandex("77.88.7.8"));
    }

    Y_UNIT_TEST(NotYa) {
        UNIT_ASSERT(ipCheckerPtr.Get());
        UNIT_ASSERT(!ipCheckerPtr->IsYandex("8.8.8.8"));
    }

    Y_UNIT_TEST(NotYa_LIB920) {
        UNIT_ASSERT(ipCheckerPtr.Get());
        UNIT_ASSERT(!ipCheckerPtr->IsYandex("37.29.3.61"));
    }

    Y_UNIT_TEST(NotYa_LIB920_Full) {
        const struct { TString YaNetBegin; TString YaNetEnd; } YA_NETS[] = {
            // HOWTO BUILD: $ grep -v sub ipreg-layout.json | tr -d 'lowhigh' | sed 's/""://g'
              {"5.45.192.0","5.45.255.255"}
            , {"5.255.192.0","5.255.255.255"}
            , {"37.9.64.0","37.9.127.255"}
            , {"37.140.128.0","37.140.191.255"}
            , {"45.87.132.0","45.87.135.255"}
            , {"77.88.0.0","77.88.63.255"}
            , {"87.250.224.0","87.250.255.255"}
            , {"93.158.128.0","93.158.191.255"}
            , {"95.108.128.0","95.108.255.255"}
            , {"100.43.64.0","100.43.95.255"}
            , {"141.8.128.0","141.8.191.255"}
            , {"172.24.0.0","172.31.255.255"}
            , {"178.154.128.0","178.154.255.255"}
            , {"185.32.187.0","185.32.187.255"}
            , {"199.36.240.0","199.36.243.255"}
            , {"213.180.192.0","213.180.223.255"}
            , {"2620:10f:d000::","2620:10f:d00f:ffff:ffff:ffff:ffff:ffff"}
            , {"2a02:6b8::","2a02:6bf:ffff:ffff:ffff:ffff:ffff:ffff"}
            , {"2a0e:fd80::","2a0e:fd87:ffff:ffff:ffff:ffff:ffff:ffff"}
        };

        TVector<TString> nonYaAddressesList;
        for (const auto& [ yaRangeBegin, yaRangeEnd ] : YA_NETS) {
            const auto& nonYaIpBeforeRange = NIPREG::TAddress::ParseAny(yaRangeBegin).Prev().AsIPv6();
            nonYaAddressesList.push_back(nonYaIpBeforeRange);

            const auto& nonYaIpAfterRange = NIPREG::TAddress::ParseAny(yaRangeEnd).Next().AsIPv6();
            nonYaAddressesList.push_back(nonYaIpAfterRange);
        }

        UNIT_ASSERT(ipCheckerPtr.Get());
        for (const auto& nonYaIp : nonYaAddressesList) {
            UNIT_ASSERT(!ipCheckerPtr->IsYandex(nonYaIp));
        }
    }

    Y_UNIT_TEST(CheckNet1) {
        UNIT_ASSERT(ipCheckerPtr.Get());
        const auto& net = ipCheckerPtr->GetNet("5.45.254.129");
        PrintNet(net);
        UNIT_ASSERT(net.IsYandex);
    }

    Y_UNIT_TEST(CheckNet2) {
        UNIT_ASSERT(ipCheckerPtr.Get());

        NIpreg::TCppSetup::TDict headers;
        headers.insert(std::make_pair("X-Forwarded-For", "2a02:6b8:0:c40::"));

        const auto& net = ipCheckerPtr->GetNet("141.8.189.193", headers);
        PrintNet(net);
        UNIT_ASSERT(net.IsYandex);
    }

    Y_UNIT_TEST(SimplePerf) {
        UNIT_ASSERT(ipCheckerPtr.Get());

        AddrListType addrsList;
        {
            TTimer timer("testdata loading... ");
            addrsList = LoadAddresses("./ipreg-layout.json");
        }
        UNIT_ASSERT(addrsList.size());
        UNIT_ASSERT(!ipCheckerPtr->IsYandex("8.8.8.8")); // just in case
        {
            TSimpleTimer timer;
            size_t failCounter = 0;
            for (const auto& addr : addrsList) {
                if (!ipCheckerPtr->IsYandex(addr)) {
                    ++failCounter;
                }
            }
            Cerr << "SimplePerf: " << timer.Get().MilliSeconds() << " millisec. " << addrsList.size() << " addresses. " << failCounter << " fails.\n";

            UNIT_ASSERT(0 == failCounter);
            UNIT_ASSERT(timer.Get().MilliSeconds() < 300);
        }
    }
}
