#pragma once

#include "lexer.h"
#include "parse_options.h"
#include "parse_token.h"

#include <kernel/remorph/common/source.h>

#include <util/folder/path.h>
#include <util/generic/buffer.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <functional>
#include <util/stream/file.h>
#include <util/string/vector.h>

namespace NReMorph {

namespace NPrivate {

struct TPathStack: public TVector<TString>, public TSimpleRefCount<TPathStack> {
    explicit TPathStack(const TString& root)
        : TVector<TString>(1u, root)
    {
    }
};

} // NPrivate

class TPreParser {
public:
    typedef std::function<void(const TParseStatement&)> TStatementHandler;

private:
    enum EState {
        S_HEAD,
        S_BODY,
    };

private:
    THolder<TIFStream> FileInput;
    TString Source;
    THolder<TFsPath> IncludeDirPath;
    bool IncludesAllowed;

    NPrivate::TLexer Lexer;
    size_t LexerBaseBufferSize;
    EState State;
    TIntrusivePtr<NPrivate::TPathStack> IncludeStack;

    TParseStatement Statement;

public:
    explicit TPreParser(const TFileParseOptions& fileParseOptions, size_t lexerBaseBufferSize = 0);
    explicit TPreParser(const TStreamParseOptions& streamParseOptions, size_t lexerBaseBufferSize = 0);
    ~TPreParser();

    void Parse(const TStatementHandler& statementHandler);
    void SetLexerBufferGrowthRate(size_t bufferGrowthRate);
    void SetLexerBufferChoppingPart(size_t bufferChoppingPart);

    void HandleToken(const TStatementHandler& statementHandler, const TParseToken& token);

    size_t GetLexerBufferGrowthRate() const;
    size_t GetLexerBufferChoppingPart() const;

private:
    explicit TPreParser(const TFileParseOptions& fileParseOptions, size_t lexerBaseBufferSize,
                        TIntrusivePtr<NPrivate::TPathStack> includeStack);

    void YieldStatement(const TStatementHandler& statementHandler);
    void Include(const TStatementHandler& statementHandler, const TWtringBuf& path);
};

} // NReMorph
