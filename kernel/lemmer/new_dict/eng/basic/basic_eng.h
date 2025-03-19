#pragma once

#include <kernel/lemmer/new_dict/eng/main/eng.h>

class TBasicEngLanguage: public TEngLanguage {
protected:
    size_t PostRecognize(TWLemmaArray& lemmas, size_t begin, size_t end, const TConvertResults&, const NLemmer::TRecognizeOpt&) const override;
    size_t RecognizeInternal(const wchar16* word, size_t length, TWLemmaArray& out, const NLemmer::TRecognizeOpt& opt) const override;

public:
    TBasicEngLanguage()
        : TEngLanguage(LANG_BASIC_ENG)
    {
    }
};
