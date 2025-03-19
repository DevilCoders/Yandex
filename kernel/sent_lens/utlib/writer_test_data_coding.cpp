#include "writer_test_data_coding.h"

#include <kernel/sent_lens/sent_lens_writer.h>

#include <util/string/hex.h>
#include <util/stream/input.h>
#include <util/string/split.h>

bool ReadNextTestCase(IInputStream* input, TSentenceLengthsTestCase* result) {
    TString line;
    if (!input->ReadLine(line)) {
        return false;
    }

    TVector<TString> sentLensCodedList;
    StringSplitter(line).Split('\t').SkipEmpty().Collect(&sentLensCodedList);
    result->clear();
    for (const auto& sentLensCoded : sentLensCodedList) {
        const TString sentLensDecoded = HexDecode(sentLensCoded);
        result->push_back(TSentenceLengths());
        TSentenceLengths& last = result->back();
        last.Reserve(sentLensDecoded.size());
        for (size_t i = 0; i < sentLensDecoded.size(); ++i) {
            last[i] = sentLensDecoded[i];
        }
        last.SetInitializedSize(sentLensDecoded.size());
    }
    return true;
}

TString BuildSentenceLengthsIndexCoded(const TSentenceLengthsTestCase& testCase) {
    TStringStream ss;
    {
        TSentenceLengthsWriter writer(&ss);
        ui32 i = 0;
        for (const auto& sentlens : testCase) {
            writer.Add(i++, sentlens);
        }
    }
    return HexEncode(ss.Str());
}
