#include "parser.h"

#include <util/stream/debug.h>

#define DBGO GetDebugOutPARSER()

namespace NRemorph {

namespace NPrivate {

void TParserStack::DbgPrint() const {
    NWRED(DBGO << "Stack:" << Endl);
    for (size_t i = 0; i < c.size(); ++i) {
        NWRED(DBGO << "  " << c[i]->GetTypeString() << Endl);
    }
}

#define WRE_STRUCT_PARSE_FUNCTION(TYPE) \
struct TParse ## TYPE: public TParseFunction {              \
    virtual const char* GetTypeString() const { return #TYPE; } \
    virtual void Invoke(TParserState& s);


WRE_STRUCT_PARSE_FUNCTION(RE)
};

WRE_STRUCT_PARSE_FUNCTION(Union)
};

WRE_STRUCT_PARSE_FUNCTION(CreateUnion)
    TAstNodePtr Result;
    TParseCreateUnion(TAstNode* node): Result(node) {}
};

WRE_STRUCT_PARSE_FUNCTION(Branch)
};

WRE_STRUCT_PARSE_FUNCTION(Catenation)
};

WRE_STRUCT_PARSE_FUNCTION(CreateCatenation)
    TAstNodePtr Result;
    TParseCreateCatenation(TAstNode* node): Result(node) {}
};

WRE_STRUCT_PARSE_FUNCTION(CreateSubmatch)
    const TTokenLParen* LParenToken;
    TParseCreateSubmatch(const TTokenLParen* lParenToken): LParenToken(lParenToken) {}
};

WRE_STRUCT_PARSE_FUNCTION(Postfix)
};

WRE_STRUCT_PARSE_FUNCTION(Atom)
};

TParserState::TParserState(const TVectorTokens& tokens)
    : Tokens(tokens)
    , Cur(0)
    , Result(nullptr)
    , Depth(0)
    , LastRParen(nullptr)
{
    Stack.push(new TParseRE());
}

void TParseRE::Invoke(TParserState& s) {
    s.Stack.push(new TParseUnion());
    s.Stack.push(new TParseBranch());
}

void TParseUnion::Invoke(TParserState& s) {
    const TToken* next = s.NextToken();
    if (!next) {
        return;
    }
    TToken::Type type = next->GetType();
    switch (type) {
        case TToken::RParen:
            if (s.Depth == 0)
                throw TUnexpectedToken(s.Cur, next);
            ++s.Cur;
            --s.Depth;
            s.LastRParen = static_cast<const TTokenRParen*>(next);
            break;
        case TToken::Pipe:
            ++s.Cur;
            s.Stack.push(new TParseCreateUnion(s.Result.Get()));
            s.Stack.push(new TParseUnion());
            s.Stack.push(new TParseBranch());
            break;
        default:
            return;
    }
}

void TParseCreateUnion::Invoke(TParserState& s) {
    s.Result = TAstUnion::Create(Result.Get(), s.Result.Get());
}

void TParseBranch::Invoke(TParserState& s) {
    s.Stack.push(new TParseCatenation());
    s.Stack.push(new TParseAtom());
}

void TParseCatenation::Invoke(TParserState& s) {
    const TToken* next = s.NextToken();
    if (!next) {
        return;
    }

    TToken::Type type = next->GetType();
    switch (type) {
        case TToken::Pipe:
        case TToken::RParen:
            return;
        default:
            s.Stack.push(new TParseCreateCatenation(s.Result.Get()));
            s.Stack.push(new TParseCatenation());
            s.Stack.push(new TParseAtom());
    }
}

void TParseCreateCatenation::Invoke(TParserState& s) {
    s.Result = TAstCatenation::Create(Result.Get(), s.Result.Get());
}

void TParseCreateSubmatch::Invoke(TParserState& s) {
    if (nullptr == s.LastRParen) {
        throw TParserError() << LParenToken->SourcePos << ": expected right parenthesis";
    }
    s.Result = TAstSubmatch::Create(s.Result.Get(), LParenToken->Name, LParenToken->SourcePos + s.LastRParen->SourcePos);
}

void TParsePostfix::Invoke(TParserState& s) {
    const TToken* next = s.NextToken();

    if (!next)
        return;

    TToken::Type type = next->GetType();
    int min = 0;
    int max = -1;
    bool greedy = true;

    switch (type) {
        case TToken::Asterisk: {
            const TTokenAsterisk* t = static_cast<const TTokenAsterisk*>(next);
            greedy = t->Greedy;
            break;
        }
        case TToken::Plus: {
            const TTokenPlus* t = static_cast<const TTokenPlus*>(next);
            greedy = t->Greedy;
            min = 1;
            break;
        }
        case TToken::Question: {
            const TTokenQuestion* t = static_cast<const TTokenQuestion*>(next);
            greedy = t->Greedy;
            max = 1;
            break;
        }
        case TToken::Repeat: {
            const TTokenRepeat* t = static_cast<const TTokenRepeat*>(next);
            greedy = t->Greedy;
            min = t->Min;
            if (min < 0) {
                throw TParserError() << next->SourcePos << ": incorrect repeater: min < 0";;
            }
            max = t->Max;
            if ((max < 1) && (max != -1)) {
                throw TParserError() << next->SourcePos << ": incorrect repeater: max is set and < 1";;
            }
            if ((max != -1) && (min > max)) {
                throw TParserError() << next->SourcePos << ": incorrect repeater: min >= max";;
            }
            break;
        }
        default:
            return;
    }
    ++s.Cur;
    s.Result = TAstIteration::Create(s.Result.Get(), min, max, greedy);
}

void TParseAtom::Invoke(TParserState& s) {
    const TToken* next = s.NextToken();

    if (!next) {
        throw TUnexpectedEND();
    }

    TToken::Type type = next->GetType();
    switch (type) {
        case TToken::LParen: {
            const TTokenLParen* t = static_cast<const TTokenLParen*>(next);
            ++s.Depth;
            ++s.Cur;
            s.Stack.push(new TParsePostfix());
            if (t->Capturing)
                s.Stack.push(new TParseCreateSubmatch(t));
            s.Stack.push(new TParseRE());
            break;
        }
        case TToken::Literal: {
            const TTokenLiteral* t = static_cast<const TTokenLiteral*>(next);
            s.Result = TAstLiteral::Create(t->Lit);
            ++s.Cur;
            if ((t->Lit.GetType() != TLiteral::Bos) && (t->Lit.GetType() != TLiteral::Eos)) {
                s.Stack.push(new TParsePostfix());
            }
            break;
        }
        case TToken::RE: {
            const TTokenRE* t = static_cast<const TTokenRE*>(next);
            s.Result = TAstTree::Clone(t->Node.Get());
            ++s.Cur;
            s.Stack.push(new TParsePostfix());
            break;
        }
        default:
            throw TUnexpectedToken(s.Cur, next);
    }
}

} // NPrivate

} // NRemorph

#undef DBGO
