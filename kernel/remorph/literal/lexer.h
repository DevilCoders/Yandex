#pragma once

#include <kernel/remorph/core/core.h>

#include <util/stream/output.h>
#include <util/stream/str.h>
#include <util/generic/string.h>
#include <utility>

/*inside scanner inclusion already done*/
#ifndef FLEX_SCANNER
    #undef yyFlexLexer
    #define yyFlexLexer litFlexLexer
    #include <FlexLexer.h>
#endif

#define LITERAL_LEXER_H_TOKENS_LIST \
    X(RLT_REFERENCE)                \
    X(RLT_LPAREN)                   \
    X(RLT_RPAREN)                   \
    X(RLT_WORD)                     \
    X(RLT_ID)                       \
    X(RLT_EQUAL)                    \
    X(RLT_NOT_EQUAL)                \
    X(RLT_LESS)                     \
    X(RLT_GREATER)                  \
    X(RLT_AND)                      \
    X(RLT_OR)                       \
    X(RLT_NOT)                      \
    X(RLT_EOF)                      \
    X(RLT_TYPE_FILE)                \
    X(RLT_TYPE_LOGIC)               \
    X(RLT_TYPE_LEMMA)               \
    X(RLT_TYPE_LEMMA_FILE)          \
    X(RLT_RE)

namespace NLiteral {

enum TLiteralLexerToken {
#define X(A) A,
    LITERAL_LEXER_H_TOKENS_LIST
#undef X
};

const TString& ToString(TLiteralLexerToken);

class TLiteralLexer: public litFlexLexer {
private:
    TStringInput Input;
    size_t CharPos;
    size_t CharPosPrev;
public:
    TLiteralLexerToken Token;
    TString TokenValue;
    NRemorph::TSourcePos SourcePos;
    TString Suffix;
public:
    TLiteralLexer(const TString& str)
        : Input(str)
        , CharPos(0)
        , CharPosPrev(0)
        , Token(RLT_EOF)
    {
        SourcePos.BegLine = SourcePos.EndLine = 1;
        SetStartConditionINITIAL();
    }
    ~TLiteralLexer() override;
    void NextToken() {
        Token = GetToken(TokenValue);
    }

    void SetStartConditionINITIAL();
    void SetStartConditionSTRING();
    void SetStartConditionLOGIC();
protected:
    int LexerInput(char* buf, int max_size) override
    {
        try {
            return (int)Input.Read(buf, (size_t)max_size);
        } catch (...) {
        }

        return -1;
    }
private:
    TLiteralLexerToken GetToken(TString& tokenValue);
};

} // NLiteral
