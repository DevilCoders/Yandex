#pragma once

#include "def.h"
#include "result.h"
#include <util/generic/yexception.h>
#include <util/generic/array_ref.h>
#include <util/generic/noncopyable.h>

namespace NHtmlLexer {
    class TContext : TNonCopyable {
    public:
        TContext();

    public:
        // EOF Related flags
        bool BrokenASP;     ///< <%...EOF; ignore all next <%
        bool BrokenComment; ///< <!--...EOF; ignore all next <!--
        bool BrokenTag;     ///< <!,<?,</tag,<tag...EOF; ignore all next <tags
                            ///<                         unless ">" has been in attr-string
        bool QuotedGt;      ///< attr-string ">" when IN_TAG
        // Lexing states, required for eof conditions
        bool InComment;
        bool InMarkup;
        bool InASP;
        bool InCDATA;
        bool InReopenScript;

        bool Iws;
        const NHtml::TTag* HTag;
        unsigned AttCnt;
    };

    /// HTML lexer, actually implemented in ragel
    class TLexer : TNonCopyable {
    private:
        TContext Ctx;

        // token
        const unsigned char* p_tokstart;
        int toklen;
        const unsigned char* namestart;
        int namelen;
        // cdata
        const unsigned char* p_cdata_text_end;
        int start_cs;

        // Ragel context
        const unsigned char* p;
        const unsigned char* pe;
        int cs;

        const unsigned char* BufferEnd;
        size_t MaxChunkSize;
        // storage
        enum { // HTML4.decl
            ATTCNT = 60,
        };
        NHtml::TAttribute AttStore[ATTCNT];

    public:
        TLexer(const char* b, size_t len, size_t maxChunkSize = 0);

        bool More() const {
            return p < BufferEnd;
        }

        int Execute(TResult* lio) {
            int st = 0;
            if (p < BufferEnd) {
                st = ExecuteStep(lio);
                if (p == BufferEnd)
                    st = ExecuteEof(lio);
            }
            return st;
        }

    private:
        int ExecuteStep(TResult* lio);
        int ExecuteEof(TResult* lio);
        int MaybeExecuteAgain(TResult* lio);
        void OnToken(TResult* lio, HTLEX_TYPE type, unsigned len);

    private: // Debug
        void show_token_info(HTLEX_TYPE type, unsigned len);
        void show_lex_pos(const char* id = "lexing", const unsigned char* ptr = nullptr);
        void show_position(const char* id = "fsm");
        void show_exec_results(const char* id = "execute");
    };

    // we need use LexHtmlData() instead of
    class TObsoleteLexer {
    private:
        TArrayRef<const char> Input;
        TResult LexerResult;
        size_t CToken;

    public:
        TObsoleteLexer(const char* data, unsigned len)
            : Input(data, len)
            , CToken(0)
        {
        }

    public:
        const char* GetCurText() {
            return HaveToken() ? (const char*)Token().Text : nullptr;
        }

        int GetCurLength() {
            return HaveToken() ? Token().Leng : 0;
        }

        HTLEX_TYPE Next();

    private:
        const TToken& Token() const {
            return LexerResult.Tokens[CToken];
        }
        bool HaveToken() const {
            return (!LexerResult.Tokens.empty() && CToken != LexerResult.Tokens.size());
        }
        NHtml::TAttribute* getCurAttr(int idx) {
            return (idx >= 0 && idx < getCurNAtrs()) ? (&LexerResult.Attrs[Token().AttStart] + idx) : nullptr;
        }

    public:
        bool eof() {
            return Input.empty() || CToken > 0 && CToken == LexerResult.Tokens.size();
        }

        bool good() {
            return true;
        }

        //Actual for tag opening and closings
        const char* getCurTag();
        unsigned getCurTagLength();

        //Actual for tag opening
        int getCurNAtrs() {
            return (Token().Type == HTLEX_START_TAG) ? Token().NAtt : 0;
        }

        bool getCurAttr(int idx, const char*& nameStart, int& nameLength, const char*& valStart, int& valLength);
        bool getCurAttrName(int idx, const char*& start, int& len);
        bool getCurAttrValue(int idx, const char*& start, int& len);
        char getCurAttrQuote(int idx);
    };

    /// this function should be erased - it's called at syntax level and lexer supports zeroes
    inline void nul2space(char* buf, const int size) {
        for (char *p = buf, *end = buf + size; p != end; ++p)
            if (!*p)
                *p = ' ';
    }

}
