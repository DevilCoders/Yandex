#pragma once

#include "grambitset.h"

#include <util/generic/string.h>

#include <library/cpp/token/nlptypes.h>

namespace NSpike {
    struct TTerminal {
        const char* Name;
        NLP_TYPE Type;
        const char* Gram;
        const char* Lemma;
        int Code;
    };

    const TTerminal* TerminalByName(const TString& name);
    TString BitsetToString(const TVector<TGramBitSet>& bitset, const TString& delim = ", ", const TString& groupdelim = "; ");
    TGramBitSet StringToBitset(const TString& gram, const TString& delim = ",; ");

    bool Agree(const TGramBitSet& controller, const TGramBitSet& target, bool negative = false);
    bool Agree(const TGramBitSet& controller, const TVector<TGramBitSet>& target, bool negative = false);
    bool Agree(const TGramBitSet& controller, const TGramBitSet& target, const TVector<TGramBitSet>& categories /*, TGramBitSet& result*/);

}
