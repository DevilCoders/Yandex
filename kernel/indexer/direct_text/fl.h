#pragma once

#include <kernel/search_types/search_types.h>
#include <kernel/keyinv/invkeypos/keychars.h>
#include <library/cpp/langmask/langmask.h>
#include <library/cpp/wordpos/wordpos.h>

#include <library/cpp/charset/wide.h>
#include <util/digest/murmur.h>
#include <util/generic/string.h>
#include <util/system/defaults.h>

namespace NIndexerCore {

//! @param text     it must be null-terminated and length must not be greater than MAXKEY_BUF
//! @param buf      text in the buffer will be null-terminated
//! @note size of buffer must be enough to fit all converted text, actually the minimal size is MAXKEY_BUF
//!       because text of lemma or form can't be greater than this value
inline size_t ConvertKeyText(const char* text, wchar16* buf) {
    const size_t len = strlen(text);
    if (!len)
        return 0;
    if (*text == UTF8_FIRST_CHAR) {
        size_t written = 0;
        UTF8ToWide(text + 1, len - 1, buf, written);
        buf[written] = 0;
        return written;
    }
    CharToWide(text, len, buf, csYandex);
    buf[len] = 0;
    return len;
}

//! @attention to convert LemmaText or FormaText to wchar16 use NIndexerCore::ConvertKeyText()
struct TLemmatizedToken {  // defines one form of multitoken
    const char* LemmaText;
    const char* FormaText;
    ui8 Lang;
    ui8 Flags;             // title case, has joins, translit
    ui8 Joins;             // left/right, delimiters

    ui8 FormOffset;        // offset in multitoken

    i32 TermCount;                   // lemma's term frequence
    double Weight;                   // lemma's context independent weight

    const char        *StemGram;     // указатель на грамматику при основе (не зависящую от формы)
    const char* const *FlexGrams;    // грамматики формы
    ui32               GramCount:31; // размер массива грамматик формы
    ui32               IsBastard:1;

    ui8 Prefix;

    TLemmatizedToken()
        : LemmaText(nullptr)
        , FormaText(nullptr)
        , Lang((char)LANG_UNK)
        , Flags(0)
        , Joins(0)
        , FormOffset(0)
        , TermCount(0)
        , Weight(0)
        , StemGram(nullptr)
        , FlexGrams(nullptr)
        , GramCount(0)
        , IsBastard(0)
        , Prefix(0)
    {
    }
};

inline bool FormsEqual(const TLemmatizedToken* x, const TLemmatizedToken* y) {
    const ui8 maskOfFlags = (FORM_TITLECASE | FORM_HAS_JOINS);
    return strcmp(x->FormaText, y->FormaText) == 0
        && (x->Flags & maskOfFlags) == (y->Flags & maskOfFlags)
        && x->Lang == y->Lang
        && x->Joins == y->Joins
        && x->Prefix == y->Prefix;
}

struct TTokenLemmaLess {
    bool operator()(const TLemmatizedToken* x, const TLemmatizedToken* y) const {
        const int n = strcmp(x->LemmaText, y->LemmaText);
        return n < 0 || (n == 0 && x->Lang < y->Lang);
    }
};

struct TTokenLemmaEqualTo {
    bool operator()(const TLemmatizedToken* x, const TLemmatizedToken* y) const {
        return strcmp(x->LemmaText, y->LemmaText) == 0 && x->Lang == y->Lang;
    }
};

struct TTokenLemmaHash {
    size_t operator()(const TLemmatizedToken* x) const {
        const size_t n = strlen(x->LemmaText);
        return MurmurHash<size_t>(x->LemmaText, n) + static_cast<size_t>(x->Lang);
    }
};

} // NIIndexerCore
