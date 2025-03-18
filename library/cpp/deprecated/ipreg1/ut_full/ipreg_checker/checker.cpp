#include <ipreg/utils/common.h>

#include <library/cpp/getopt/opt.h>
#include <library/cpp/ipreg/address.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/deprecated/ipreg1/lookup.h>

#include <util/string/cast.h>
#include <util/stream/file.h>
#include <util/string/vector.h>
#include <util/string/split.h>
#include <util/stream/input.h>

using namespace NCommon;

namespace {
    class IpregChecker {
    public:
        IpregChecker(const TString& datafile)
            : Lookup(datafile.data())
        {
        }

        bool CheckAddr(NIPREG::TAddress addr, bool isUser) const {
            const auto answer = Lookup.GetNet(addr.AsIPv6());

            if (isUser && !answer.IsUser) {
                TStats::Inc("ya-user_not-detected");
            }
            if (!answer.IsYandex) {
                TStats::Inc("ya-net_not-detected");
            }

            return isUser ? answer.IsYandex && answer.IsUser : answer.IsYandex;
        }

        void MakeCheck(const TString& testFname, int pairsCheck, bool crashOnErr) {
            TUnbufferedFileInput jsonFile(testFname);
            NJson::TJsonValue allRanges;
            NJson::ReadJsonTree(&jsonFile, &allRanges);

            if (!allRanges.IsArray()) {
                ythrow yexception() << "failed to parse json '" << testFname << "'; wanted array with nets descriptions";
            }

            const auto& rangesArr = allRanges.GetArray();
            const auto rangesQty = rangesArr.size();
            for (int idx = 0; idx != rangesQty; ++idx) {
                const auto& range = rangesArr[idx];
                if (!range.Has("low") || !range.Has("high")) {
                    throw yexception() << "critical: low/high not found; #" << idx;
                }

                const auto& lo = range["low"].GetString();
                const auto& hi = range["high"].GetString();
                const auto& flags = range["flags"];

                TStats::Inc("ranges-in-qty");

                const bool isUser = flags["user"].GetBoolean();
                if (isUser) TStats::Inc("user");

                NIPREG::TAddress lowAddr  = NIPREG::TAddress::ParseAny(lo);
                NIPREG::TAddress highAddr = NIPREG::TAddress::ParseAny(hi);

                int addrsQty{};
                int checkedQty{};

                while ((addrsQty < pairsCheck * 2) && (lowAddr < highAddr)) {
                    TStats::Inc("iter_" + ToString(addrsQty / 2 + 1));
                    addrsQty += 2;
                    checkedQty += CheckAddr(lowAddr, isUser) + CheckAddr(highAddr, isUser);

                    lowAddr = lowAddr.Next();
                    highAddr = highAddr.Prev();
                }

                TStats::Inc("addrs_qty", addrsQty);
                TStats::Inc("checked_addrs_qty", checkedQty);

                if (addrsQty != checkedQty) {
                    TStats::Inc("failed_ranges");
                    TString msg = "non-consistent results were detected!";
                    if (crashOnErr) {
                        throw yexception() << msg;
                    }
                    Cerr << "FAIL\t" << lo << "-" << hi << "\n";
                    TStats::Inc("failed_addrs_qty", addrsQty - checkedQty);
                    Errors += addrsQty - checkedQty;
                }
            }
        }

        size_t GetErrorsAmount() const {
            return Errors;
        }

    private:
        NIpreg::TLookup Lookup;
        size_t Errors = 0;
    };
} // anon-ns

int main(int argc, const char* argv[]) {
    const StatsHelper stats("ipeg", true);

    NLastGetopt::TOpts opts;
    opts.AddHelpOption('h');
    opts.SetFreeArgsMin(0);
    opts.SetFreeArgsMax(1);

    TString layoutFname = "ipreg-layout.json";
    int pairsToCheck = 3;
    bool crashOnError = false;

    opts.AddLongOption('i', "ipreg-layout").RequiredArgument("path").DefaultValue(layoutFname).StoreResult(&layoutFname).Help("path to layout file");
    opts.AddLongOption('p', "pairs").RequiredArgument("qty").DefaultValue(pairsToCheck).StoreResult(&pairsToCheck).Help("qty of checked pairs in each row");
    opts.AddLongOption('c', "crash-on-error").Optional().NoArgument().DefaultValue("0").OptionalValue("1").StoreResult(&crashOnError).Help("just crash");
    NLastGetopt::TOptsParseResult optr(&opts, argc, argv);

    IpregChecker checker(layoutFname);
    checker.MakeCheck(layoutFname, pairsToCheck, crashOnError);

    TStats::PrintStats();
    return !!checker.GetErrorsAmount();
}
