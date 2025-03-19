#pragma once

#include <kernel/lemmer/new_dict/rus/main_with_trans/rus.h>

class TBasicRusLanguage: public TRusLanguageWithTrans {
protected:
    size_t PostRecognize(TWLemmaArray& lemmas, size_t begin, size_t end, const TConvertResults&, const NLemmer::TRecognizeOpt&) const override;
    TBasicRusLanguage()
        : TRusLanguageWithTrans(LANG_BASIC_RUS)
    {
    }
};
