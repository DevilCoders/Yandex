#pragma once

#include <util/random/mersenne.h>
#include <util/generic/string.h>

namespace NSnippets {
    class TConfig;
    class TWordShuffler {
    private:
        TMersenne<ui64> Rnd;
    public:
        TWordShuffler(ui64 seed)
          : Rnd(seed)
        {
        }

        void ShuffleWords(TUtf16String& s);
        void ShuffleWords(const TConfig& cfg, TUtf16String& s);
    };
}
