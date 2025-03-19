#pragma once

#include <kernel/sent_lens/sent_lens.h>

namespace NDoom {

THolder<ISentenceLengthsReader> NewOffroadFastSentWadIndex(const TString& path, bool lockMemory);
THolder<ISentenceLengthsReader> NewOffroadFactorSentWadIndex(const TString& path, bool lockMemory);

} // NDoom
