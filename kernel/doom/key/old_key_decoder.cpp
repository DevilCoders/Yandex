#include <kernel/search_types/search_types.h>
#include "old_key_decoder.h"

#include <library/cpp/charset/recyr.hh>

#include <kernel/keyinv/invkeypos/keycode.h>

namespace NDoom {

bool TOldKeyDecoder::Decode(const TStringBuf& src, TDecodedKey* dst) const {
    Y_ASSERT(dst);

    TKeyLemmaInfo lemma;
    char forms[N_MAX_FORMS_PER_KISHKA][MAXKEY_BUF];
    int formLengths[N_MAX_FORMS_PER_KISHKA];

    int formCount = DecodeKey(src.data(), &lemma, forms, formLengths);
    if (formCount == -1) {
        return false;
    }

    if (Options_ & IgnoreSpecialKeysDecodingOption) {
        if (lemma.szPrefix[0] != '\0') {
            return false;
        }
        if (lemma.szLemma[0] == OPEN_ZONE_PREFIX || lemma.szLemma[0] == CLOSE_ZONE_PREFIX) {
            return false;
        }
    }

    dst->Clear();

    TString decodedLemma;
    if (lemma.szPrefix[0] != '\0')
        decodedLemma += lemma.szPrefix;

    if (Options_ & RecodeToUtf8DecodingOption) {
        if (lemma.szLemma[0] == UTF8_FIRST_CHAR) {
            decodedLemma += TStringBuf(lemma.szLemma + 1);
        } else {
            char buffer[MAXKEY_BUF];
            size_t r = 0;
            size_t w = 0;
            RECODE_RESULT status = Recode(CODES_YANDEX, CODES_UTF8, lemma.szLemma, buffer, strlen(lemma.szLemma), sizeof(buffer), r, w);
            if (status != RECODE_OK) {
                return false;
            }
            decodedLemma += TStringBuf(buffer, w);
        }
    } else {
        decodedLemma += lemma.szLemma;
    }

    if (decodedLemma.empty()) {
        return false; /* This does happen (UTF8 first char and nothing else in lemma), and we don't want to generate invalid decoded keys. */
    }

    dst->SetLemma(decodedLemma);

    ui8 lemmaLanguage = lemma.Lang;

    for (int i = 0; i < formCount; i++) {
        char* form = forms[i];
        int formLength = formLengths[i];

        ui8 flags = 0, joins = 0, language = LANG_UNK;
        RemoveFormFlags(form, &formLength, &flags, &joins, &language);
        Y_ENSURE(formLength >= 0);

        if (language == LANG_UNK) {
            language = lemmaLanguage;
            if (language != LANG_UNK) {
                flags |= FORM_HAS_LANG;
            }
        }

        if (Options_ & RecodeToUtf8DecodingOption) {
            if (formLength > 0 && form[0] == UTF8_FIRST_CHAR) {
                dst->AddForm(static_cast<ELanguage>(language), static_cast<EFormFlag>(flags), TStringBuf(form + 1, formLength - 1));
            } else {
                char buffer[MAXKEY_BUF];
                size_t r = 0;
                size_t w = 0;
                RECODE_RESULT status = Recode(CODES_YANDEX, CODES_UTF8, form, buffer, formLength, sizeof(buffer), r, w);
                if (status != RECODE_OK) {
                    return false; /* Skip recoding failures. */
                }

                dst->AddForm(static_cast<ELanguage>(language), static_cast<EFormFlag>(flags), TStringBuf(buffer, w));
            }
        } else {
            dst->AddForm(static_cast<ELanguage>(language), static_cast<EFormFlag>(flags), TStringBuf(form, formLength));
        }
    }

    return true;
}

} // namespace NDoom
