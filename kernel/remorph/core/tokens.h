#pragma once

#include "literal.h"
#include "ast.h"
#include "source_pos.h"

#include <util/generic/ptr.h>
#include <util/generic/hash.h>

#define WRE_TOKEN_TYPES_LIST \
    X(Literal)               \
    X(Asterisk)              \
    X(Question)              \
    X(Repeat)                \
    X(Pipe)                  \
    X(LParen)                \
    X(RParen)                \
    X(Plus)                  \
    X(RE)

namespace NRemorph {

class TToken: public TSimpleRefCount<TToken> {
public:
    enum Type {
#define X(T) T,
        WRE_TOKEN_TYPES_LIST
#undef X
    };

private:
    Type Type_;

public:
    TSourcePosPtr SourcePos;

private:
    static const TString& TypeAsString(Type t) {
        static struct TMapType {
            THashMap<int, TString> Data;
            TMapType() {
#define X(T) Data[T] = #T;
                WRE_TOKEN_TYPES_LIST
#undef X
            };
        } Map;
        return Map.Data[t];
    }

protected:
    TToken(Type t)
        : Type_(t) {
    }

public:
    virtual ~TToken() {
    }
    Type GetType() const {
        return Type_;
    }
    const TString& TypeAsString() const {
        return TypeAsString(Type_);
    }
};

typedef TIntrusivePtr<TToken> TTokenPtr;

#define WRE_STRUCT_TOKEN(TYPE)                            \
    struct TToken##TYPE##_: public TToken {               \
        TToken##TYPE##_():TToken(TYPE){}                  \
    };                                                    \
    struct TToken##TYPE: public TToken##TYPE##_

WRE_STRUCT_TOKEN(Literal) {
    TLiteral Lit;
    TTokenLiteral(TLiteral l)
        : Lit(l) {
    }
};

WRE_STRUCT_TOKEN(Asterisk) {
    bool Greedy;
    TTokenAsterisk()
        : Greedy(true) {
    }
};

WRE_STRUCT_TOKEN(Question) {
    bool Greedy;
    TTokenQuestion()
        : Greedy(true) {
    }
};

WRE_STRUCT_TOKEN(Plus) {
    bool Greedy;
    TTokenPlus()
        : Greedy(true) {
    }
};

WRE_STRUCT_TOKEN(Repeat) {
    bool Greedy;
    int Min;
    int Max;
    TTokenRepeat(int min, int max)
        : Greedy(true)
        , Min(min)
        , Max(max) {
    }
};

WRE_STRUCT_TOKEN(Pipe) {
};

WRE_STRUCT_TOKEN(LParen) {
    bool Capturing;
    TString Name;
    TTokenLParen(TSourcePosPtr sourcePos)
        : Capturing(true) {
        SourcePos = sourcePos;
    }
    TTokenLParen(TSourcePosPtr sourcePos, bool capturing)
        : Capturing(capturing) {
        SourcePos = sourcePos;
    }
    TTokenLParen(TSourcePosPtr sourcePos, const TString& name)
        : Capturing(true)
        , Name(name) {
        SourcePos = sourcePos;
    }
};

WRE_STRUCT_TOKEN(RParen) {
    TTokenRParen(TSourcePosPtr sourcePos) {
        SourcePos = sourcePos;
    }
};

WRE_STRUCT_TOKEN(RE) {
    TAstNodePtr Node;
    TTokenRE(TAstNode* node)
        : Node(node) {
    }
};

#define WRE_TOKEN_TO_STRING(T)                                          \
    template <class TLiteralTable>                                      \
    inline TString TokenToString##T(const TLiteralTable& lt, const TToken##T& t)

WRE_TOKEN_TO_STRING(Asterisk) {
    Y_UNUSED(lt);
    return t.TypeAsString() + (t.Greedy ? "" : "-NG");
}

WRE_TOKEN_TO_STRING(Question) {
    Y_UNUSED(lt);
    return t.TypeAsString() + (t.Greedy ? "" : "-NG");
}

WRE_TOKEN_TO_STRING(Plus) {
    Y_UNUSED(lt);
    return t.TypeAsString() + (t.Greedy ? "" : "-NG");
}

WRE_TOKEN_TO_STRING(Repeat) {
    Y_UNUSED(lt);
    return t.TypeAsString() + "{" + ::ToString(t.Min) + "," + ::ToString(t.Max) + "}" + (t.Greedy ? "" : "-NG");
}

WRE_TOKEN_TO_STRING(Pipe) {
    Y_UNUSED(lt);
    return t.TypeAsString();
}

WRE_TOKEN_TO_STRING(RParen) {
    Y_UNUSED(lt);
    return t.TypeAsString();
}

WRE_TOKEN_TO_STRING(RE) {
    Y_UNUSED(lt);
    return t.TypeAsString();
}

WRE_TOKEN_TO_STRING(Literal) {
    return t.TypeAsString() + '[' + lt.ToString(t.Lit) + ']';
}

WRE_TOKEN_TO_STRING(LParen) {
    Y_UNUSED(lt);
    return t.TypeAsString() + (t.Capturing ? (t.Name.empty() ? "" : "[" + t.Name + "]") : "-NC");
}

template <class TLiteralTable>
inline TString ToString(const TLiteralTable& lt, const TToken& t) {
    switch (t.GetType()) {
#define X(T)                                    \
        case TToken::T:                         \
            return TokenToString##T(lt, static_cast<const TToken##T&>(t));
        WRE_TOKEN_TYPES_LIST
#undef X
    }
    return TString();
}

typedef TVector<TTokenPtr> TVectorTokens;

template <class TLiteralTable>
inline void Print(IOutputStream& out, const TLiteralTable& lt, const TVectorTokens& tokens) {
    for (size_t i = 0; i < tokens.size(); ++i) {
        const TToken& t = *tokens[i];
        if (i) out << " ";
        out << ToString(lt, t);
        if (!!t.SourcePos)
            out << ":" << t.SourcePos;
    }
}

} // NRemorph
