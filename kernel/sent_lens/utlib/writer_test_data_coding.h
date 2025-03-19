#pragma once

#include <kernel/sent_lens/sent_lens.h>
#include <util/generic/list.h>

class IInputStream;

using TSentenceLengthsTestCase = TList<TSentenceLengths>;

bool ReadNextTestCase(IInputStream* input, TSentenceLengthsTestCase* result);
TString BuildSentenceLengthsIndexCoded(const TSentenceLengthsTestCase& testCase);
