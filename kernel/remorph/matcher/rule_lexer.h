#pragma once

#include <kernel/remorph/core/core.h>

#include <util/stream/output.h>

/*inside scanner inclusion already done*/
#ifndef FLEX_SCANNER
    #undef yyFlexLexer
    #define yyFlexLexer rlFlexLexer
    #include <FlexLexer.h>
#endif

#define RULE_LEXER_H_TOKENS_LIST                \
    X(RLT_EOF)                                  \
    X(RLT_EOL)                                  \
    X(RLT_EOID)                                 \
    X(RLT_DEF)                                  \
    X(RLT_RULE)                                 \
    X(RLT_STRING)                               \
    X(RLT_STRING_VALUE)                         \
    X(RLT_INCLUDE)                              \
    X(RLT_LOGIC)                                \
    X(RLT_USE_GZT)                              \
    X(RLT_WORD)                                 \
    X(RLT_ID)                                   \
    X(RLT_PATH)                                 \
    X(RLT_REFERENCE)                            \
    X(RLT_PIPE)                                 \
    X(RLT_ASTERISK)                             \
    X(RLT_QUESTION)                             \
    X(RLT_DOT)                                  \
    X(RLT_PLUS)                                 \
    X(RLT_LPAREN)                               \
    X(RLT_LPAREN_NC)                            \
    X(RLT_LPAREN_NAMED)                         \
    X(RLT_RPAREN)                               \
    X(RLT_LBRACKET)                             \
    X(RLT_RBRACKET)                             \
    X(RLT_CARET)                                \
    X(RLT_DOLLAR)                               \
    X(RLT_MARK)                                 \
    X(RLT_SHARP)                                \
    X(RLT_COMMA)                                \
    X(RLT_FLOAT)

namespace NReMorph {

namespace NPrivate {

enum TRuleLexerToken {
#define X(A) A,
    RULE_LEXER_H_TOKENS_LIST
#undef X
};

const TString& ToString(TRuleLexerToken);

class TLexerError: public yexception {
};

class TRuleLexer: public rlFlexLexer {
private:
    IInputStream& Input;
    size_t LinePrev;
    size_t CharPos;
    size_t CharPosPrev;
    TString StringValue;
    bool HoldToken;
public:
    TRuleLexerToken Token;
    TString TokenValue;
    NRemorph::TSourcePos SourcePos;
public:
    TRuleLexer(IInputStream& in, const TString& fileName)
        : Input(in)
        , LinePrev(1)
        , CharPos(0)
        , CharPosPrev(0)
        , HoldToken(false)
        , Token(RLT_EOF)
        , SourcePos(fileName, 1, 1, 0, 0)
    {
    }
    ~TRuleLexer() override;
    void NextToken() {
        if (HoldToken) {
            HoldToken = false;
            return;
        }
        Token = GetToken(TokenValue);
    }
    void SetStartConditionRE();
    void SetStartConditionINITIAL();
    void SetStartConditionID();
    void SetStartConditionPATH();
    void SetStartConditionSTRING();
    void SetStartConditionLIST();
    void SetStartConditionLITERAL();

    inline void HoldCurrentToken() {
        HoldToken = true;
    }
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
    TRuleLexerToken GetToken(TString& tokenValue);
};

} // NPrivate

} // NReMorph
