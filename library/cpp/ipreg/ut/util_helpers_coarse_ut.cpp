#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/ipreg/reader.h>
#include <library/cpp/ipreg/util_helpers.h>
#include <library/cpp/ipreg/writer.h>

#include <util/stream/str.h>
#include <util/string/split.h>
#include <util/string/vector.h>

using namespace NIPREG;

namespace {
    TString ExtractCoarsedValue(const TString& checkedRange) {
        const TVector<TString> parts = StringSplitter(checkedRange).SplitBySet(":}").SkipEmpty();
        UNIT_ASSERT(5 == parts.size());

        return parts[parts.size() - 2];
    }

    TString GetCoarsedResult(const TString& input) {
        TStringInput inStream(input);
        TStringStream outStream;

        TReader reader(inStream);
        TWriter writer(outStream);

        DoCoarsening(reader, writer);
        return outStream.Str();
    }
} // anon-ns

Y_UNIT_TEST_SUITE(UtilCoarseTest)
{
    Y_UNIT_TEST(EmptyInput)
    {
        const TString empty;
        UNIT_ASSERT_STRINGS_EQUAL(empty, GetCoarsedResult(empty));
    }

    Y_UNIT_TEST(SimpleCheck)
    {
        using Row2CoeffPair = std::pair<TString, TString>;
        const TVector<Row2CoeffPair> testData = {
            {"20000.02",  "::0-::1\t{\"reliability\":20951.03,\"region_id\":1}\n"},
            {"0",         "::2-::3\t{\"reliability\":0.74,\"region_id\":2}\n"},
            {"150.03",    "::4-::5\t{\"reliability\":159.65,\"region_id\":3}\n"},
            {"500.05",    "::6-::7\t{\"reliability\":547.48,\"region_id\":4}\n"},
            {"80.04",     "::8-::9\t{\"reliability\":74.49,\"region_id\":5}\n"},
            {"12350012.35","::0-::9\t{\"reliability\":12345678.9,\"region_id\":225}\n"}
        };

        for (const auto& dataPair : testData) {
            const auto wantedCoeff = dataPair.first;
            const auto& rangeRow = dataPair.second;

            UNIT_ASSERT(wantedCoeff == ExtractCoarsedValue(GetCoarsedResult(rangeRow)));
        }
    }
} // UtilCoarseTest
