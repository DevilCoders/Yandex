#pragma once

#include "source_pos.h"
#include "tokens.h"
#include "ast.h"
#include "types.h"
#include "debug.h"

#include <util/generic/ptr.h>
#include <util/generic/stack.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

#define DBGO GetDebugOutPARSER()

namespace NRemorph {

class TParserError: public yexception {
};

class TUnexpectedToken: public TParserError {
private:
    const NRemorph::TToken* Token;
    TSourcePosPtr SourcePos;
    TString ExpandedToken;
public:
    TUnexpectedToken(size_t, const NRemorph::TToken* token)
        : Token(token)
        , SourcePos(token->SourcePos)
    {
    }
    template <class TLiteralTable>
    void ExpandToken(const TLiteralTable& lt) {
        ExpandedToken = NRemorph::ToString(lt, *Token);
        *this << Token->SourcePos << ": unexpected token " << ExpandedToken;
    }
    inline TSourcePosPtr GetSourcePos() const {
        return SourcePos;
    }
    inline const TString& GetExpandedToken() const {
        return ExpandedToken;
    }
};

class TUnexpectedEND: public TParserError {
public:
    TUnexpectedEND() {
        *this << "unexpected END";
    }
};

class TMismatchedParens: public TParserError {
public:
    TMismatchedParens() {
        *this << "mismatched parentheses";
    }
};

namespace NPrivate {

struct TParserState;

struct TParseFunction: public TSimpleRefCount<TParseFunction> {
    virtual ~TParseFunction() {}
    virtual const char* GetTypeString() const = 0;
    virtual void Invoke(TParserState& s) = 0;
};

typedef TIntrusivePtr<TParseFunction> TParseFunctionPtr;

struct TParserStack: public TStack<TParseFunctionPtr, TVector<TParseFunctionPtr>> {
    void DbgPrint() const;
};

struct TParserState {
    const TVectorTokens& Tokens;
    size_t Cur;
    TParserStack Stack;
    TAstNodePtr Result;
    size_t Depth;
    const TTokenRParen* LastRParen;

    TParserState(const TVectorTokens& tokens);

    const TToken* NextToken() const {
        return Tokens.empty() ?
            nullptr
            : (Cur < Tokens.size() ?
                Tokens[Cur].Get()
                : nullptr);
    }

    template <class TLiteralTable>
    void DbgPrint(const TLiteralTable& lt) const {
        NWRED(DBGO << "NextToken: " << (NextToken() ? ToString(lt, *NextToken()) : "NULL") << Endl);
        Stack.DbgPrint();
        NWRED(DBGO << "Result:");
        if (Result.Get()) {
            NWRED(DBGO << Endl << Bind(lt, *Result) << Endl);
        } else {
            NWRED(DBGO << " NULL" << Endl);
        }
        NWRED(DBGO << "================================================================================" << Endl);
        Y_UNUSED(lt);
    }
};

template <class TLiteralTable>
TAstNodePtr ParseImpl(const TLiteralTable& lt, const TVectorTokens& tokens) {
    NWRE_UNUSED(lt);
    NPrivate::TParserState state(tokens);
    while (!state.Stack.empty()) {
        NWRED(state.DbgPrint(lt));
        NPrivate::TParseFunctionPtr f = state.Stack.top();
        state.Stack.pop();
        f->Invoke(state);
    }
    NWRED(DBGO << Endl);
    if (state.Depth)
        throw TMismatchedParens();
    NWRED(DBGO << "Ast:" << Endl);
    NWRED(DBGO << Bind(lt, *state.Result) << Endl);
    return state.Result;
}

} // NPrivate

} // NRemorph

#undef DBGO
