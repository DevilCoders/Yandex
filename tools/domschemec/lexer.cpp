#include "lexer.h"
#include "ops.h"

#include <util/string/cast.h>
#include <util/generic/ptr.h>

namespace NDomSchemeCompiler {

TLexer::TLexer(const TString& file)
    : Context(MakeHolder<TDefaultLexerContext>(file))
{
    Advance();
}

TLexer::TLexer(THolder<ILexerContext>&& ctx)
    : Context(std::move(ctx))
{
    Advance();
}

TPosition TLexer::GetPosition() const {
    // TODO: calculate position upon lexing
    const char*& CurChar = Context->CurChar();
    const TStringBuf& NextToken = Context->NextToken();
    const TString& Code = Context->Code();

    size_t curLine = 1;
    const char* curLineStart = nullptr;
    for (const char* c = Code.data(); c <= CurChar - NextToken.size(); ++c) {
        if (*c == '\n') {
            ++curLine;
            curLineStart = c;
        }
    }
    return {Context->Position().FileName, curLine, CurChar - curLineStart - NextToken.size()};
}

TStringBuf TLexer::Peek() const {
    return Context->NextToken();
}

bool TLexer::Next(TStringBuf expected) {
    if (Context->NextToken() != expected) {
        ThrowSyntaxError(TString("'") + expected + "' expected");
    }
    Next();
    return true;
}

bool TLexer::TryNext(TStringBuf tok) {
    if (Context->NextToken() != tok) {
        return false;
    }
    Next();
    return true;
}

TStringBuf TLexer::Next() {
    TStringBuf res = Context->NextToken();
    Advance();
    return res;
}

void TLexer::ThrowSomeError(const TString& msg, const TMaybe<TPosition>& posParam) const {
    TPosition pos = posParam.GetOrElse(GetPosition());
    ythrow TParseError(pos, msg);
}

void TLexer::ThrowSyntaxError(const TString& msg, const TMaybe<TPosition>& posParam) const {
    TStringBuf& NextToken = Context->NextToken();
    ThrowSomeError(TString("Unexpected token '") + NextToken + "'. " + msg, posParam);
}

bool TLexer::IsSymbol(TStringBuf tok) {
    return isalpha(tok[0]) || tok[0] == '_' || tok[0] == '-';
}

bool TLexer::IsString(TStringBuf tok) {
    return tok.size() >= 2 && tok[0] == '"' && tok[tok.size() - 1] == '"';
}

TStringBuf TLexer::NextSymbol() {
    TStringBuf& NextToken = Context->NextToken();
    if (!NextToken || !IsSymbol(NextToken)) {
        ThrowSyntaxError("Expected some symbol");
    }
    return Next();
}

TStringBuf TLexer::NextUnsignedNumber() {
    TStringBuf& NextToken = Context->NextToken();
    size_t value;
    if (!NextToken || !TryFromString(NextToken, value)) {
        ThrowSyntaxError("Expected an unsigned number");
    }
    return Next();
}

TStringBuf TLexer::NextString() {
    TStringBuf& NextToken = Context->NextToken();
    if (!NextToken || !IsString(NextToken)) {
        ThrowSyntaxError("Expected string");
    }
    return Next();
}

bool TLexer::NextBool() {
    TStringBuf& NextToken = Context->NextToken();
    if (NextToken != "true" && NextToken != "false") {
        ThrowSyntaxError("Expected bool (\"true\" or \"false\"");
    }
    return Next() == "true";
}

char TLexer::NextChar(const char* allowedChars) {
    TStringBuf& NextToken = Context->NextToken();
    for (const char* c = allowedChars; *c; ++c) {
        if (NextToken == TStringBuf(c, 1)) {
            Next();
            return *c;
        }
    }
    ThrowSyntaxError(TString("Expected one char of: '") + allowedChars + "'");
    return '?';
}

TStringBuf TLexer::SlurpCppCode() {
    const char*& CurChar = Context->CurChar();
    TStringBuf& NextToken = Context->NextToken();

    if (Peek() != "{") {
        ThrowSyntaxError("Expected '{'");
    }

    const char* start = NextToken.data();
    size_t depth = 1;
    while (*CurChar && depth > 0) {
        // TODO: count all brackets - "{(["
        if (*CurChar == '{') {
            ++depth;
        } else if (*CurChar == '}') {
            --depth;
        } else if (*CurChar == '\'') {
            ++CurChar; // opening '
            if (*CurChar == '\\') {
                ++CurChar; // escape symbol
                if (!*CurChar) {
                    ThrowSomeError("Expected character");
                }
            }
            ++CurChar; // symbol itself
            if (*CurChar != '\'') {
                ThrowSomeError("Expected \"'\"");
            }
            ++CurChar; // closing '
        } else if (*CurChar == '"') {
            ++CurChar; // opening "
            while (*CurChar != '"') {
                if (*CurChar == '\'') {
                    ++CurChar; // escape symbol
                    if (!*CurChar) {
                        ThrowSomeError("Expected character");
                    }
                }
                ++CurChar; // symbol itslef
                if (!*CurChar) {
                    ThrowSomeError("Expected '\"'");
                }
            }
            ++CurChar; // closing "
        }
        ++CurChar;
    }

    TStringBuf res { start, CurChar };
    Advance();
    return res;
}

void TLexer::Advance() {
    const char*& CurChar = Context->CurChar();
    TStringBuf& NextToken = Context->NextToken();

    SkipWhitespace();

    if (!*CurChar) {
        // EOF
        NextToken = TStringBuf();
    } else if (isalpha(*CurChar) || *CurChar == '_') {
        // symbol (keyword / name)
        const char* start = CurChar;
        while (isalnum(*CurChar) || *CurChar == '_' || *CurChar == '-') {
            ++CurChar;
        }
        NextToken = TStringBuf(start, CurChar);
    } else if (*CurChar == '"') {
        const char* start = CurChar++;
        while (*CurChar && *CurChar != '"') {
            ++CurChar;
        }
        NextToken = TStringBuf(start, ++CurChar);
    } else if (*CurChar == '$' && GetPragmas().ProcessSvnKeywords) {
        const char* start = CurChar++;
        while (*CurChar && *CurChar != '$') {
            ++CurChar;
        }
        NextToken = TStringBuf(start, ++CurChar);
    } else if (isdigit(*CurChar) || (CurChar[0] == '-' && isdigit(CurChar[1]))) {
        const char* start = CurChar++;
        while (isdigit(*CurChar)) {
            ++CurChar;
        }
        if (*CurChar == '.') {
            ++CurChar;
            while (isdigit(*CurChar)) {
                ++CurChar;
            }
        }
        NextToken = TStringBuf(start, CurChar);
    } else {
        TStringBuf dtok = TStringBuf(CurChar, 2);
        if (dtok == "::" || dtok == "->" || IsValidationOp(dtok)) {
            NextToken = dtok;
            CurChar += 2;
        } else {
            NextToken = TStringBuf(CurChar, 1);
            ++CurChar;
        }
    }
}

void TLexer::SkipWhitespace() {
    const char*& CurChar = Context->CurChar();
    for (;;) {
        if (isspace(*CurChar)) {
            ++CurChar;
        } else if (CurChar[0] == '/' && CurChar[1] == '/') {
            CurChar += 2;
            while (*CurChar && *CurChar != '\n') {
                ++CurChar;
            }
            ++CurChar;
        } else if (CurChar[0] == '/' && CurChar[1] == '*') {
            CurChar += 2;
            while (CurChar[0] && CurChar[1] && CurChar[0] != '*' && CurChar[1] != '/') {
                ++CurChar;
            }
            ++CurChar;
            if (*CurChar) {
                ++CurChar;
            }
        } else {
            break;
        }
    }
}

bool TLexer::ContextPop() {
    return Context->Pop();
}

void TLexer::ContextPush(const TString& file) {
    Context->Push(file);
    Advance();
}

const TPragmas& TLexer::GetPragmas() const {
    return Context->GetPragmas();
}

TPragmas& TLexer::SetPragmas() {
    return Context->SetPragmas();
}

}
