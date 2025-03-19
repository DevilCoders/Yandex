#pragma once

#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/new_dict/common/new_language.h>

class TTurLanguage: public TLanguageTemplate<LANG_TUR> {
protected:
    bool CanBreak(const wchar16* word, size_t pos1, size_t len1, size_t pos2, size_t len2, bool isTranslit) const override;
    size_t PostRecognize(TWLemmaArray& lemmas, size_t begin, size_t end, const TConvertResults& cr, const NLemmer::TRecognizeOpt&) const override;

private:
    template <typename T>
    bool FirstPartIsAForm(T lemmasBegin, T lemmasEnd, const TWtringBuf& firstPart, const NLemmer::TRecognizeOpt& opt) const;
};
