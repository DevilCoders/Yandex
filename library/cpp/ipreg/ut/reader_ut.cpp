#include <library/cpp/testing/unittest/env.h> // ArcadiaSourceRoot()
#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/ipreg/reader.h>

#include <util/stream/file.h>
#include <util/stream/str.h>
#include <util/string/split.h>
#include <util/string/vector.h>

using namespace NIPREG;

namespace {
    void CheckAddr(const TAddress& addr, const TString& wantedIpStr) {
        if (addr.IsIPv4()) {
            const bool isIpv6Prefix = wantedIpStr.StartsWith("::ffff:");
            UNIT_ASSERT_STRINGS_EQUAL(wantedIpStr, isIpv6Prefix ? addr.AsShortIPv6() : addr.AsIPv4());
            return;
        }

        const bool isFullIPv6 = wantedIpStr.size() == 39;
        UNIT_ASSERT_STRINGS_EQUAL(wantedIpStr, isFullIPv6 ? addr.AsIPv6() : addr.AsShortIPv6());
    }

    void CheckRange(const TRange& range, const TString& line, bool isEmptyData) {
        const TVector<TString> parts = StringSplitter(line).SplitBySet(" -\t").SkipEmpty();
        UNIT_ASSERT((isEmptyData ? 2 : 3) == parts.size());
        UNIT_ASSERT(parts[0].size() && parts[1].size());
        if (!isEmptyData) {
            UNIT_ASSERT(parts[2].size());
        }

        CheckAddr(range.First, parts[0]);
        CheckAddr(range.Last,  parts[1]);

        if (!isEmptyData) {
            UNIT_ASSERT_STRINGS_EQUAL(range.Data, parts[2]);
        }
    }

    void CheckReader(TReader&& reader, const TString& controlTestData, bool isEmptyData = false) {
        const TVector<TString> lines = StringSplitter(controlTestData).Split('\n').SkipEmpty();
        for (const auto& line : lines) {
            UNIT_ASSERT(reader.Next());
            CheckRange(reader.Get(), line, isEmptyData);
        }
        UNIT_ASSERT(!reader.Next());
    }
} // anon-ns

Y_UNIT_TEST_SUITE(ReaderTest) {
    Y_UNIT_TEST(FromFile) {
        const TString& testDataFname = ArcadiaSourceRoot() + "/library/cpp/ipreg/ut/testIPREG.parse";
        const TString& testData(TFileInput(testDataFname).ReadAll());

       CheckReader(TReader(testDataFname, false, " "), testData);
    }

    Y_UNIT_TEST(FromStream) {
        const TString testData =
            "::-::1:2:3:4\trow1\n"
            "::1:2:3:5-::2:3:4:6\trow2\n"
            "::2:3:4:7-::3:4:5:8\trow3\n"
            "::3:4:5:9-::4:5:6:0\trow4\n"
            "::4:5:6:1-::5:6:7:2\trow5\n";
        TStringInput ss(testData);

        CheckReader(TReader(ss), testData);
    }

    Y_UNIT_TEST(FromStreamNoData) {
        const TString testData =
            "::-::1:2:3:4\t\n"
            "::1:2:3:5-::3:4:5:6\n";
        TStringInput ss(testData);

        bool NoData = true;
        CheckReader(TReader(ss, NoData), testData, NoData);
    }
} // ReaderTest
