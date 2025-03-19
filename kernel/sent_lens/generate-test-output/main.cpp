#include <kernel/sent_lens/utlib/writer_test_data_coding.h>

#include <util/stream/output.h>

int main() {
    TSentenceLengthsTestCase testCase;
    while (ReadNextTestCase(&Cin, &testCase)) {
        Cout << BuildSentenceLengthsIndexCoded(testCase) << Endl;
    }
    return 0;
}
