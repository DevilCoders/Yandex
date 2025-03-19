#include "pre_parser.h"

#include "errors.h"

#include <util/charset/wide.h>
#include <util/generic/algorithm.h>


namespace NReMorph {

namespace {

inline void UnexpectedTokenError(const TParseToken& token) {
    throw TParsingError(token.Location) << "unexpected token: " << token;
}

}

TPreParser::TPreParser(const TFileParseOptions& fileParseOptions, size_t lexerBaseBufferSize)
    : FileInput(new TIFStream(fileParseOptions.RulesPath))
    , Source(fileParseOptions.RulesPath)
    , IncludeDirPath(!fileParseOptions.BaseDirPath.empty() ? new TFsPath(fileParseOptions.BaseDirPath) : nullptr)
    , IncludesAllowed(true)
    , Lexer(*FileInput, Source, lexerBaseBufferSize)
    , LexerBaseBufferSize(lexerBaseBufferSize)
    , State(S_HEAD)
    , IncludeStack(new NPrivate::TPathStack(Source))
    , Statement()
{
}

TPreParser::TPreParser(const TStreamParseOptions& streamParseOptions, size_t lexerBaseBufferSize)
    : FileInput()
    , Source(streamParseOptions.RulesPath)
    , IncludeDirPath(!streamParseOptions.BaseDirPath.empty() ? new TFsPath(streamParseOptions.BaseDirPath) : nullptr)
    , IncludesAllowed(!streamParseOptions.Inline)
    , Lexer(streamParseOptions.Input, Source, lexerBaseBufferSize)
    , LexerBaseBufferSize(lexerBaseBufferSize)
    , State(S_HEAD)
    , IncludeStack(IncludesAllowed ? new NPrivate::TPathStack(Source) : nullptr)
    , Statement()
{
}

TPreParser::~TPreParser() {
    if (IncludeStack) {
        Y_ASSERT(!IncludeStack->empty());
        IncludeStack->pop_back();
    }
}

void TPreParser::Parse(const TStatementHandler& statementHandler) {
    NPrivate::TLexer::TTokenHandler tokenHandler = std::bind(&TPreParser::HandleToken, this, statementHandler, std::placeholders::_1);
    Lexer.Scan(tokenHandler);

    if (State != S_HEAD) {
        throw TParsingError(Statement.Location) << "statement is unfinished";
    }
}

void TPreParser::SetLexerBufferGrowthRate(size_t bufferGrowthRate) {
    Lexer.SetBufferGrowthRate(bufferGrowthRate);
}

void TPreParser::SetLexerBufferChoppingPart(size_t bufferChoppingPart) {
    Lexer.SetBufferChoppingPart(bufferChoppingPart);
}

void TPreParser::HandleToken(const TStatementHandler& statementHandler, const TParseToken& token) {
    if (token.Type == PTT_UNKNOWN) {
        UnexpectedTokenError(token);
    }

    switch (State) {
    case S_HEAD:
        if (token.Type == PTT_ID) {
            Statement.Head = token.RawData;
            Statement.Location = token.Location;
            State = S_BODY;
        } else {
            UnexpectedTokenError(token);
        }
        break;
    case S_BODY:
        Statement.Body.push_back(token);
        if (token.Type == PTT_SEMICOLON) {
            YieldStatement(statementHandler);
            Statement.Body.clear();
            State = S_HEAD;
        }
        break;
    }
}

size_t TPreParser::GetLexerBufferGrowthRate() const {
    return Lexer.GetBufferGrowthRate();
}

size_t TPreParser::GetLexerBufferChoppingPart() const {
    return Lexer.GetBufferChoppingPart();
}

TPreParser::TPreParser(const TFileParseOptions& fileParseOptions, size_t lexerBaseBufferSize,
                       TIntrusivePtr<NPrivate::TPathStack> includeStack)
    : FileInput(new TIFStream(fileParseOptions.RulesPath))
    , Source(fileParseOptions.RulesPath)
    , IncludeDirPath(new TFsPath(fileParseOptions.BaseDirPath))
    , IncludesAllowed(true)
    , Lexer(*FileInput, Source, lexerBaseBufferSize)
    , LexerBaseBufferSize(lexerBaseBufferSize)
    , State(S_HEAD)
    , IncludeStack(includeStack)
    , Statement()
{
    IncludeStack->push_back(Source);
}

void TPreParser::YieldStatement(const TStatementHandler& statementHandler) {
    if (Statement.Head == "include") {
        if ((Statement.Body.size() != 2) || (Statement.Body[0].Type != PTT_STRING)) {
            throw TParsingError(Statement.Location) << "incorrect 'include' statement";
        }
        if (!IncludesAllowed) {
            throw TParsingError(Statement.Location) << "includes are not allowed for inlined source";
        }
        if (Statement.Body[0].Data.empty()) {
            throw TParsingError(Statement.Body[0].Location) << "empty filename specified for include";
        }
        Include(statementHandler, Statement.Body[0].Data);
        return;
    }

    statementHandler(Statement);
}

void TPreParser::Include(const TStatementHandler& statementHandler, const TWtringBuf& path) {
    Y_ASSERT(IncludeDirPath);
    Y_ASSERT(IncludeStack.Get());
    Y_ASSERT(IncludeStack);

    TFsPath includePath = (*IncludeDirPath / ::WideToUTF8(path)).RealPath();

    if (::Find(IncludeStack->begin(), IncludeStack->end(), includePath.c_str()) != IncludeStack->end()) {
        throw TParsingError(TSourceLocation(Source)) << "included file forms a cycle: " << path;
    }

    if (!includePath.IsFile()) {
        throw TParsingError(TSourceLocation(Source)) << "included file doesn't exist or not a file: " << includePath;
    }

    TPreParser(TFileParseOptions(includePath.c_str()), LexerBaseBufferSize, IncludeStack).Parse(statementHandler);
}

} // NReMorph
