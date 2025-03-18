#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/string/printf.h>

enum ETokenType {
    TT_IDENTIFIER    = 0x0001, // Also literal, actually
    TT_OPTION        = 0x0002,
    TT_TARGETTYPEDEF = 0x0004,
    TT_EXPLDEPENDS   = 0x0008,
    // UNUSED        = 0x0010,
    TT_DEPENDONPREV  = 0x0020,
    TT_LISTDIVIDER   = 0x0040,
    TT_CONDEPEND     = 0x0080,
    TT_DEFVAR        = 0x0100,
    TT_STRONGVAR     = 0x0200,
    TT_GROUPDEPEND   = 0x0400,
    TT_SEMAPHORE     = 0x0800,
    TT_NONRECURSIVE  = 0x1000,
    TT_DEPENDMAPPING = 0x2000,
};

class TToken {
public:
    TToken(const char *start, const char *end, int line, int pos, ETokenType type)
        : Text(start, end)
        , Line(line)
        , Pos(pos)
        , Type(type) {
    }

    const TString& GetText() const {
        return Text;
    }

    int GetPos() const {
        return Pos;
    }

    int GetLine() const {
        return Line;
    }

    TString GetWhere() const {
        return Sprintf("%d:%d", Line, Pos);
    }

    ETokenType GetType() const {
        return Type;
    }

    bool operator==(const TString &s) const {
        return Text == s;
    }

    bool operator==(char c) const {
        return Text.length() == 1 && Text[0] == c;
    }

private:
    TString Text;
    int Line;
    int Pos;
    ETokenType Type;
};

class TTokenStack {
public:
    TTokenStack() {
    }

    void AddToken(TToken t) {
        Stack.push_back(t);
    }

    bool Is(size_t pos, char what) const {
        return pos < Stack.size() && Stack[pos] == what;
    }

    bool Is(size_t pos, const char *what) const {
        return pos < Stack.size() && Stack[pos] == what;
    }

    bool Is(size_t pos, int what) const {
        return pos < Stack.size() && (Stack[pos].GetType() & what);
    }

    bool Has(size_t pos) const {
        return pos < Stack.size();
    }

    TString Get(size_t pos) const {
        if (pos >= Stack.size()) {
            if (Stack.empty())
                ythrow yexception() << "expected token, got nothing";
            else
                ythrow yexception() << "expected token, after " << Stack.back().GetText() << " at " << Stack.back().GetWhere() << ", got nothing";
        }

        return Stack[pos].GetText();
    }

    TString Get(size_t pos, int what) const {
        if (pos >= Stack.size()) {
            if (Stack.empty())
                ythrow yexception() << "expected token of type " << (int)what << ", got nothing";
            else
                ythrow yexception() << "expected token of type " << (int)what << " after " << Stack.back().GetText() << " at " << Stack.back().GetWhere() << ", got nothing";
        }

        if (!(what & Stack[pos].GetType()))
            ythrow yexception() << "expected token of type " << (int)what << " at " << Stack[pos].GetWhere() << " (" << Stack[pos].GetText() << "), got type " << (int)Stack[pos].GetType();

        return Stack[pos].GetText();
    }

    TToken GetToken(size_t pos) const {
        return Stack[pos];
    }

    void Clear() {
        Stack.clear();
    }

    bool Empty() const {
        return Stack.empty();
    }

    size_t Size() const {
        return Stack.size();
    }

    void Dump() const {
        for (TVector<TToken>::const_iterator i = Stack.begin(); i != Stack.end(); ++i) {
            printf("%d[%s] ", i->GetType(), i->GetText().data());
        }
        printf("\n");
    }

private:
    TVector<TToken> Stack;
};
