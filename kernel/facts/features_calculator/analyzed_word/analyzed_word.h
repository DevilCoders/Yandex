#pragma once

#include <kernel/lemmer/core/lemmer.h>

#include <util/charset/wide.h>

namespace NUnstructuredFeatures {
    struct TAnalyzedWord {
        TUtf16String Word;
        TWLemmaArray Lemmas;
        float Frequency;
    };
}
