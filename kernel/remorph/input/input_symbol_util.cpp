#include "input_symbol_util.h"
#include "input_symbol.h"

#include "lemmas.h"

#include <kernel/inflectorlib/phrase/complexword.h>

#include <kernel/lemmer/core/lemmer.h>

#include <util/generic/string.h>
#include <util/string/vector.h>

namespace NSymbol {

namespace NPrivate {

static const TUtf16String WSPACE = u" ";

class TNominativeFormBuilder::TImpl {
private:
    NInfl::TSimpleAutoColloc Colloc;

public:
    void AddSymbol(TInputSymbol& symbol, const TDynBitMap& ctx) {
        const ILemmas& lemmas = symbol.GetLemmas(ctx);
        TYemmaIteratorPtr yemmaIteratorPtr = lemmas.GetYemmas();
        if (yemmaIteratorPtr.Get()) {
            TWLemmaArray yemmas;
            for (IYemmaIterator& yemmaIterator = *yemmaIteratorPtr; yemmaIterator.Ok(); ++yemmaIterator) {
                yemmas.push_back(*yemmaIterator);
            }

            Colloc.AddWord(NInfl::TComplexWord(symbol.GetNormalizedText(), yemmas));
        } else {
            Colloc.AddWord(NInfl::TComplexWord(symbol.GetLangMask(), symbol.GetNormalizedText()));
        }
    }

    TUtf16String BuildForm() {
        Colloc.GuessMainWord();
        Colloc.ReAgree();

        TVector<TUtf16String> wtroki;
        Colloc.Normalize(wtroki);

        return JoinStrings(wtroki, WSPACE);
    }
};

TNominativeFormBuilder::TNominativeFormBuilder()
    : Impl(new TNominativeFormBuilder::TImpl())
{
}

TNominativeFormBuilder::~TNominativeFormBuilder() {
}

void TNominativeFormBuilder::AddSymbol(TInputSymbol& s, const TDynBitMap& ctx) {
    Impl->AddSymbol(s, ctx);
}

TUtf16String TNominativeFormBuilder::BuildForm() {
    return Impl->BuildForm();
}

} // NPrivate

TUtf16String ToCamelCase(const TWtringBuf& str) {
    TUtf16String res;

    if (0 != str.size()) {
        bool makeTitle = true;
        for (TCharIterator it(str.begin(), str.end()); it != str.end(); ++it) {
            wchar32 c = *it;
            if (IsAlpha(c)) {
                c = makeTitle ? ToTitle(c) : ToLower(c);
                makeTitle = false;
            }
            if (!IsAlnum(c))
                makeTitle = true;
            WriteSymbol(c, res);
        }
    }
    return res;
}

}  // NSymbol
