#pragma once

#include <util/generic/strbuf.h>

struct TToken {
    TWtringBuf Word;        // Can be empty for leading punctuation
    TWtringBuf Punctuation; // All symbols after the current and up to the next token

    TToken() {
    }

    TToken(const TWtringBuf& word, const TWtringBuf& punct)
        : Word(word)
        , Punctuation(punct)
    {
    }
};
