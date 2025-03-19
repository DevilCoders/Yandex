#include "key_decoder.h"

#include <kernel/search_types/search_types.h>
#include <library/cpp/charset/doccodes.h>
#include <library/cpp/charset/recyr.hh>
#include <kernel/keyinv/invkeypos/keychars.h>

namespace NDoom {

bool TKeyDecoder::Decode(const TStringBuf& src, TDecodedKey* key) const {
    Y_ASSERT(key);
    key->Clear();

    const char* formsPtr = src.data();
    const char* srcEnd = formsPtr + src.size();

    while (formsPtr < srcEnd && *formsPtr != '\x01') {
        ++formsPtr;
    }
    key->SetLemma(TStringBuf(src.data(), formsPtr));
    if (key->Lemma().size() > 0 && (Options_ & EKeyDecodingOption::IgnoreSpecialKeysDecodingOption)) {
        char prefix = key->Lemma()[0];
        if (prefix == OPEN_ZONE_PREFIX || prefix == CLOSE_ZONE_PREFIX || prefix == ATTR_PREFIX) {
            return false;
        }
    }

    if (formsPtr >= srcEnd) {
        if (!key->Lemma().StartsWith(ATTR_PREFIX)) {
            key->AddForm(LANG_UNK, 0, key->Lemma());
        }
        return true;
    }
    ++formsPtr;
    if (formsPtr >= srcEnd) {
        // non-empty forms expected
        return false;
    }

    TString formBuf;
    formBuf.reserve(key->Lemma().size() + 1);
    formBuf += key->Lemma();
    formBuf += '\x00';

    while (formsPtr < srcEnd) {
        size_t header = static_cast<unsigned char>(*(formsPtr++));
        size_t same = 0;
        size_t diff = 0;
        if (header & 128) {
            if (formsPtr >= srcEnd) {
                return false;
            }
            same = (header & 127);
            diff = static_cast<unsigned char>(*(formsPtr++));
        } else {
            diff = (header & 15);
            same = (header >> 4);
        }

        if (diff > size_t(srcEnd - formsPtr)) {
            return false;
        }
        if (same > formBuf.size()) {
            return false;
        }
        formBuf.resize(same);

        formBuf.append(formsPtr, diff);
        formsPtr += diff;

        if (formBuf.empty()) {
            return false;
        }

        size_t formTextLen = formBuf.size() - 1;
        EFormFlags flags = static_cast<EFormFlag>(static_cast<unsigned char>(formBuf.back()));
        ELanguage language = LANG_UNK;
        if (flags & FORM_HAS_LANG) {
            if (formBuf.size() < 2) {
                return false;
            }
            language = static_cast<ELanguage>(static_cast<unsigned char>(formBuf[--formTextLen]));
        }
        key->AddForm(language, flags, TStringBuf(formBuf.data(), formTextLen));
    }
    return true;
}

} // namespace NDoom
