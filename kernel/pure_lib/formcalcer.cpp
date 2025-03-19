#include <util/charset/wide.h>
#include <kernel/lemmer/alpha/abc.h>

#include "formcalcer.h"

const wchar16 FormPrefix = '!';
const wchar16 TitleCaseSuffix = '\x01'; //TODO a copy from ysite/yandex/pure

size_t TFormPureCalcer::Derenyx(TLangMask mask, const wchar16* src, size_t srclen, wchar16* buf, size_t buflen) {
    if (mask.Test(LANG_RUS)
        || mask.Test(LANG_UKR)
        || mask.Test(LANG_KAZ)) {
        return NLemmer::GetAlphaRulesUnsafe(LANG_RUS)->Derenyx(src, srclen, buf, buflen).Length;
    } else if (mask.Test(LANG_ENG)) {
        return NLemmer::GetAlphaRulesUnsafe(LANG_ENG)->Derenyx(src, srclen, buf, buflen).Length;
    }

    return 0;
}

TString TFormPureCalcer::ConvertToIndexFormat(const TUtf16String& form, bool isTitled) {
    size_t len = form.size();

    TCharTemp buffer(len + 2);
    wchar16* dst = buffer.Data();

    *dst++ = FormPrefix;

    memcpy(dst, form.data(), len * 2);
    dst += len;

    if (isTitled) {
        *dst++ = TitleCaseSuffix;
    }

    return WideToUTF8(buffer.Data(), dst - buffer.Data());
}

void TFormPureCalcer::Fill(const TUtf16String& text) {
    Forms.clear();
    Sources.clear();
    Lemmas.clear();
    Tokenizer.Tokenize(text.data(), text.size());
    TokenHandler.Flush();
    TokenHandler.Clear();
}
