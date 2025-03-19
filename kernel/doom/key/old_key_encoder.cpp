#include <kernel/search_types/search_types.h>
#include "old_key_encoder.h"

#include <kernel/keyinv/invkeypos/keycode.h>

namespace NDoom {


bool TOldKeyEncoder::Encode(const TDecodedKey& src, char* dst, size_t* dstLength) const {
    TKeyLemmaInfo lemma;
    char forms[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF];
    const char* formPtrs[N_MAX_FORMS_PER_KISHKA];

    size_t formCount = src.FormCount();

    if (formCount > static_cast<size_t>(N_MAX_FORMS_PER_KISHKA))
        return false;

    if (src.Lemma().size() + 1 > sizeof(lemma.szLemma))
        return false;

    if (src.Lemma().empty())
        return false; /* Note that one of them might be empty, but not both. */

    if (formCount == 0) {
        formCount = 1;
        formPtrs[0] = lemma.szLemma;
    } else {
        for (size_t i = 0; i < formCount; ++i) {
            const TDecodedFormRef& form = src.Form(i);

            if (form.Text().size() + 1 > MAXKEY_BUF)
                return false;

            memcpy(forms[i], form.Text().data(), form.Text().size());
            forms[i][form.Text().size()] = 0;

            int length = form.Text().size();
            if (!AppendFormFlags(forms[i], &length, MAXKEY_BUF, form.Flags(), 0, form.Language()))
                return false;

            formPtrs[i] = forms[i];
        }
    }

    memcpy(lemma.szLemma, src.Lemma().data(), src.Lemma().size());
    lemma.szLemma[src.Lemma().size()] = 0;

    int size = ConstructKeyWithForms(dst, MAXKEY_BUF, lemma, formCount, formPtrs);
    if (size <= 0)
        return false;

    *dstLength = size - 1; /* Size also counts trailing '\0'. */
    return true;
}

bool TOldKeyEncoder::Encode(const TDecodedKey& src, char* dst) const {
    size_t dstLength;
    return Encode(src, dst, &dstLength);
}

bool TOldKeyEncoder::Encode(const TDecodedKey& src, TString* dst) const {
    dst->resize(MAXKEY_LEN, ' ');

    size_t length = 0;
    if (!Encode(src, dst->begin(), &length))
        return false;

    dst->resize(length);
    return true;
}

TString TOldKeyEncoder::Encode(const TDecodedKey& src) const {
    TString result;
    return Encode(src, &result) ? result : TString();
}


} // namespace NDoom

