#include "simple.h"

#include <kernel/inflectorlib/phrase/complexword.h>

#include <kernel/lemmer/dictlib/grambitset.h>

#include <util/charset/wide.h>
#include <util/generic/yexception.h>
#include <util/string/vector.h>

namespace NInfl {

namespace {

struct TWordHint {
    bool Main;
    TGramBitSet Grams;

    explicit TWordHint()
        : Main(false)
        , Grams()
    {
    }

    inline void Parse(const TStringBuf& str) {
        TStringBuf hints = str;
        TStringBuf hint;
        while (hints.NextTok(';', hint)) {
            if (hint.empty()) {
                continue;
            }
            size_t equalsPos = hint.find('=');
            bool supported = true;
            if (equalsPos == TStringBuf::npos) {
                if ((hint == "m") || (hint == "main")) {
                    Main = true;
                } else {
                    supported = false;
                }
            } else if ((equalsPos != 0) && (equalsPos != hint.size() - 1)) {
                TStringBuf name = hint.substr(0, equalsPos);
                TStringBuf value = hint.substr(equalsPos + 1);
                if ((name == "g") || (name == "grams")) {
                    Grams = TGramBitSet::FromString(value);
                } else {
                    supported = false;
                }
            } else {
                supported = false;
            }
            if (!supported) {
                throw yexception() << "Unsupported hint: \"" << hint << "\"";
            }
        }
    }
};

}

TSimpleInflector::TSimpleInflector(const TString& langs)
    : Langs(!langs.empty() ? NLanguageMasks::CreateFromList(langs) : NLanguageMasks::BasicLanguages())
    , OutputDelimiter(u" ")
{
}

TUtf16String TSimpleInflector::Inflect(const TUtf16String& text, const TString& grams, TSimpleResultInfo* resultInfo, bool pluralization) const {
    TGramBitSet gramBitSet = TGramBitSet::FromString(grams);

    TSimpleAutoColloc colloc;
    size_t collocSize = 0;
    bool mainWordSet = false;

    TWtringBuf phrase = text;
    TWtringBuf token;
    while (phrase.NextTok(' ', token)) {
        if (token.empty()) {
            continue;
        }

        TWordHint wordHint;
        size_t metaStart = token.find('{');
        if (metaStart != TWtringBuf::npos) {
            size_t metaEnd = token.find('}');
            if (metaEnd == token.size() - 1) {
                wordHint.Parse(::WideToUTF8(token.substr(metaStart + 1, metaEnd - metaStart - 1)));
                token.Trunc(metaStart);
            }
        }

        colloc.AddWord(TComplexWord(Langs, token, TWtringBuf(), wordHint.Grams, nullptr, true), wordHint.Main);
        ++collocSize;
        if (wordHint.Main) {
            if (mainWordSet) {
                throw yexception() << "Multiple main words specified";
            }
            mainWordSet = true;
        }
    }

    if (collocSize == 0) {
        return TUtf16String();
    }

    colloc.GuessOneLang();

    if (!mainWordSet) {
        colloc.GuessMainWord();
    }
    colloc.ReAgree();

    TVector<TUtf16String> inflected;

    TGramBitSet resultGrams;
    if (!colloc.Inflect(gramBitSet, inflected, resultInfo ? &resultGrams : nullptr, pluralization)) {
        return TUtf16String();
    }

    if (resultInfo) {
        resultInfo->Grams = resultGrams.ToString();
        resultInfo->Debug = colloc.DebugString();
    }

    return ::JoinStrings(inflected, OutputDelimiter);
}

} // NInfl
