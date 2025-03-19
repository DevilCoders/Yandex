#pragma once

#include <util/generic/string.h>
#include <util/generic/strbuf.h>

#include <kernel/lemmer/dictlib/grambitset.h>
#include <kernel/lemmer/core/lemmer.h>
#include <library/cpp/langmask/langmask.h>


namespace NInfl {

class TNumFlexParadigm;

class TNumeralAbbr {
public:
    TNumeralAbbr(const TLangMask& langMask, const TWtringBuf& wordText);

    bool IsRecognized() const {
        return Paradigm != nullptr;
    }

    bool Inflect(const TGramBitSet& grammems, TUtf16String& restext, TGramBitSet* resgram = nullptr) const;

    void ConstructText(const TWtringBuf& newflex, TUtf16String& text) const;
private:

    TWtringBuf Prefix, Numeral, Delim, Flexion;
    const TNumFlexParadigm* Paradigm;   // without ownership
};

}   // namespace NInfl
