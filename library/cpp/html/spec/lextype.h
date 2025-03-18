#pragma once

#include <util/generic/fwd.h>

enum HTLEX_TYPE {
    HTLEX_EOF, // == YY_NULL
    HTLEX_START_TAG,
    HTLEX_EMPTY_TAG, // Never emitted by lexer
    HTLEX_END_TAG,
    HTLEX_MD,
    HTLEX_PI,
    HTLEX_TEXT,
    HTLEX_COMMENT,
    HTLEX_ASP,
};

const TString& ToString(HTLEX_TYPE);
bool FromString(const TString& name, HTLEX_TYPE& ret);
