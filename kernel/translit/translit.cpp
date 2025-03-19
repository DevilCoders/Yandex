/// author@ vvp@ Victor Ploshikhin
/// created: Nov 14, 2012 7:31:08 PM

#include "translit.h"

#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/untranslit/untranslit.h>
#include <library/cpp/token/charfilter.h>

#include <util/charset/unidata.h>

namespace {

bool Trans(TUntransliter& transliterator, const TWtringBuf& text, TUntransliter::WordPart& res)
{
    transliterator.Init(text);
    res = transliterator.GetNextAnswer();
    return !res.Empty();
}

}


TString TransliterateBySymbol(const TStringBuf& text, const ELanguage textLanguage) {
    TString res;
    TransliterateBySymbol(text, res, textLanguage);
    return res;
}

TString TransliterateBySymbol(const TStringBuf& text, TUntransliter* transliterator) {
    TString res;
    TransliterateBySymbol(text, res, transliterator);
    return res;
}

void TransliterateBySymbol(const TStringBuf& text, TString& translit, const ELanguage textLanguage) {
    const TLanguage* lang = NLemmer::GetLanguageById(textLanguage);
    if (!lang) {
        lang = NLemmer::GetLanguageById(LANG_RUS);
    }

    const auto transliterator = lang->GetTransliter();
    TransliterateBySymbol(text, translit, transliterator.Get());
}

void TransliterateBySymbol(const TStringBuf& text, TString& translit, TUntransliter* transliterator) {
    translit.clear();

    const TUtf16String wideText = UTF8ToWide(text);

    if (transliterator) {
        translit.reserve(text.size());

        for (const auto& ch : wideText) {
            bool transSuccess = false;
            TUntransliter::WordPart wp;

            const TWtringBuf charWbuf(&ch, 1);

            if (Trans(*transliterator, charWbuf, wp)) {
                transSuccess = true;
            } else {
                const TUtf16String norm = NormalizeUnicode(charWbuf, true, true); // try get rid of dyacritic
                if (Trans(*transliterator, norm, wp)) {
                    transSuccess = true;
                }
            }

            const auto preSize = translit.size();
            const TWtringBuf str = transSuccess ? wp.GetWord() : charWbuf;
            translit.resize(translit.size() + WideToUTF8BufferSize(str.size()));
            size_t written = 0;
            WideToUTF8(str.data(), str.size(), translit.begin() + preSize, written);
            translit.resize(preSize + written);
        }
    } else {
        translit = text;
    }
}
