#include "utlib/writer_test_data_coding.h"

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/archive/yarchive.h>

namespace {
    const unsigned char data[] = {
        #include <kernel/sent_lens/ut/test_data.inc>
    };

    const TArchiveReader TestDataArchive(TBlob::NoCopy(data, sizeof(data)));
}

Y_UNIT_TEST_SUITE(TSentenceLengthsWriterTest) {
    Y_UNIT_TEST(TestOnSomeData) {
        TAutoPtr<IInputStream> testInput = TestDataArchive.ObjectByKey("/sent_lens_writer_test_input.inc");
        TAutoPtr<IInputStream> testOutput = TestDataArchive.ObjectByKey("/sent_lens_writer_test_output.inc");

        TString inputLine;
        TString outputLine;
        size_t testCaseIdx = 0;
        while (true) {
            TSentenceLengthsTestCase testCase;
            TString expectedCodedOutput;
            bool inputExhausted = ReadNextTestCase(testInput.Get(), &testCase);
            bool outputExhausted = testOutput->ReadLine(expectedCodedOutput);

            // we expect input and output to be exausted simultaneouslyu
            UNIT_ASSERT_VALUES_EQUAL_C(
                    inputExhausted, outputExhausted,
                    "test internal error: input and output exhausted not simultaneously");
            if (!inputExhausted) {
                break;
            }
            TString actualCodedOutput = BuildSentenceLengthsIndexCoded(testCase);
            UNIT_ASSERT_EQUAL_C(
                    expectedCodedOutput, actualCodedOutput,
                    (TStringBuilder() << "error in test case #" << testCaseIdx << " (counting from 0)" ).data()
                    );
            ++testCaseIdx;
        }
    }
}
