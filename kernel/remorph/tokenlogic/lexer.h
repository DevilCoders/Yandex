#pragma once

#include <kernel/remorph/core/core.h>

#include <util/stream/output.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>

/*inside scanner inclusion already done*/
#ifndef FLEX_SCANNER
    #undef yyFlexLexer
    #define yyFlexLexer tlFlexLexer
    #include <FlexLexer.h>
#endif

namespace NTokenLogic {

namespace NPrivate {

class TLexerError: public yexception {
};

struct TLexerToken: public NRemorph::TSourcePos {
    int Type;
    TString Text;
    TLexerToken(int type, const NRemorph::TSourcePos& src)
        : NRemorph::TSourcePos(src)
        , Type(type)
    {
    }
    ~TLexerToken() override {
    }
};

typedef TIntrusivePtr<TLexerToken> TLexerTokenPtr;

class TLexer: public tlFlexLexer {
private:
    IInputStream& Input;
    size_t LinePrev;
    size_t CharPos;
    size_t CharPosPrev;
    TString StringValue;
    NRemorph::TSourcePos SourcePos;

    TVector<TLexerTokenPtr> ParsedTokens;

public:
    TLexer(IInputStream& in, const TString& fileName)
        : Input(in)
        , LinePrev(1)
        , CharPos(0)
        , CharPosPrev(0)
        , SourcePos(fileName, 1, 1, 0, 0)
    {
    }

    ~TLexer() override;

    TLexerToken* NextToken() {
        TString tokenValue;
        ParsedTokens.push_back(new TLexerToken(GetToken(tokenValue), SourcePos));
        DoSwap(ParsedTokens.back()->Text, tokenValue);
        return ParsedTokens.back().Get();
    }

    void ClearTokenCache() {
        ParsedTokens.clear();
    }

    TString GetContext() const {
        return SourcePos.ToString();
    }

protected:
    int LexerInput(char* buf, int max_size) override {
        try {
            return (int)Input.Read(buf, (size_t)max_size);
        } catch (...) {
        }
        return -1;
    }

private:
    int GetToken(TString& tokenValue);
};

} // NPrivate

} // NReMorph
