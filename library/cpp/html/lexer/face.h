#pragma once

#include "def.h"
#include "lex.h"

#include <util/generic/array_ref.h>

namespace NHtmlLexer {
    // bool TJob::operator () (const TToken* lexTokens, size_t tokenCount, const TAttribute* attrs);
    // false for break
    template <class TJob>
    void LexHtmlData(const char* data, size_t len, TJob&& job) {
        if (!data || !len)
            return;

        /// @warning breaks constness, should be removed when nlp is ready
        nul2space((char*)data, len);

        const size_t blockSize = 1 * 1024 * 1024;

        TResult lexResult;
        TLexer lexer(data, len, blockSize);

        while (lexer.More()) {
            lexer.Execute(&lexResult);
            if (lexResult.Tokens.empty())
                continue;
            if (!job(lexResult.Tokens.data(), lexResult.Tokens.size(), lexResult.Attrs.data()))
                break;
            lexResult.Clear();
        }
    }

}
