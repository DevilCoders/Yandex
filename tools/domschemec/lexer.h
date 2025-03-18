#pragma once

#include "lexer_context.h"
#include "pragma.h"

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>
#include <util/generic/maybe.h>

namespace NDomSchemeCompiler {

struct TParseError : public yexception {
    TPosition Position;
    TString Message;

    TParseError(TPosition pos, const TString& message)
        : Position(pos)
        , Message(message)
    {
    }
};

class TLexer {
    THolder<ILexerContext> Context;

public:
    TLexer(const TString& file);
    TLexer(THolder<ILexerContext>&& ctx);

    static bool IsSymbol(TStringBuf tok);
    static bool IsString(TStringBuf tok);

    // generic interface
    TPosition GetPosition() const;
    TStringBuf Peek() const;
    bool TryNext(TStringBuf tok);
    bool Next(TStringBuf expected);
    TStringBuf Next();
    void ThrowSomeError(const TString& msg, const TMaybe<TPosition>& = Nothing()) const;
    void ThrowSyntaxError(const TString& msg, const TMaybe<TPosition>& = Nothing()) const;

    // common required things
    TStringBuf NextSymbol();
    TStringBuf NextUnsignedNumber();
    TStringBuf NextString();
    bool NextBool();
    char NextChar(const char* allowedChars);

    // a dirty-dirty hack
    TStringBuf SlurpCppCode();

    bool ContextPop();
    void ContextPush(const TString& file);
    const TPragmas& GetPragmas() const;
    TPragmas& SetPragmas();

private:
    void Advance();
    void SkipWhitespace();
};

};
