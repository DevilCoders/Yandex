#pragma once

#include "def.h"

#include <util/generic/noncopyable.h>
#include <util/generic/vector.h>

namespace NHtmlLexer {
    /// result of lexing a portion of HTML - lex tokens and their attributes
    struct TResult: private TNonCopyable {
        TVector<TToken> Tokens;
        TVector<NHtml::TAttribute> Attrs;

        inline TResult() {
            Tokens.reserve(2048);
            Attrs.reserve(512);
        }

        inline ~TResult() {
        }

        inline void Clear() {
            Tokens.clear();
            Attrs.clear();
        }

        inline void AddToken(const TToken& t) {
            Tokens.push_back(t);
        }
    };

}
