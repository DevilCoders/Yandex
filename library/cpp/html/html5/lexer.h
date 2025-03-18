#pragma once

#include "ascii.h"
#include "tokenizer.h"
#include <library/cpp/html/lexer/result.h>

namespace NHtml5 {
    class TLexer {
    public:
        TLexer(const char* b, size_t len);

        bool More() const;

        int Execute(NHtmlLexer::TResult* lio);

    private:
        void EmitCommentToken(const TToken& token, NHtmlLexer::TResult* lio);

        void EmitDoctypeTag(const TToken& token, NHtmlLexer::TResult* lio);

        void EmitEndTag(const TToken& token, NHtmlLexer::TResult* lio);

        void EmitStartTag(const TToken& token, NHtmlLexer::TResult* lio);

        void FinishTag(const TToken& token, NHtmlLexer::TToken* res);

        void InitializeText();

        void MaybeChangeTokenizerState(const TToken& token);

        void MaybeEmitTextToken(NHtmlLexer::TResult* lio);

    private:
        struct TText {
            ETokenType Type;
            const char* Start;
            size_t Length;
        };

        TTokenizer<TByteIterator> Tokenizer_;
        TText Text_;
        bool More_;
    };

    // bool TJob::operator () (const TToken* lexTokens, size_t tokenCount, const TAttribute* attrs);
    // false for break
    template <class TJob>
    void LexHtmlData(const char* data, size_t len, TJob&& job) {
        struct TImpl {
            inline static void nul2space(char* buf, int size) {
                for (char* p = buf; size--; p++)
                    if (!*p) {
                        *p = ' ';
                    }
            }
        };

        if (!data || !len)
            return;

        /// @warning breaks constness, should be removed when nlp is ready
        TImpl::nul2space((char*)data, len);

        NHtmlLexer::TResult lexResult;
        NHtml5::TLexer lexer(data, len);

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
