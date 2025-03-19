#pragma once

#include <util/generic/utility.h>
#include <util/generic/vector.h>

namespace NSegm {

struct TTokenTraits {
    enum {
        Ignore = 0,
        MustBe = 1,
        MustNotBe = 2,
    };

    union {
        ui8 Traits;

        struct {
            ui8 Title :2;
            ui8 Header :2;
            ui8 Link :2;
        };
    };

    explicit TTokenTraits(ui8 t = 0)
        : Traits(t)
    {}

    bool Accept(TTokenTraits a) const {
        return AcceptField(Title, a.Title) && AcceptField(Header, a.Header) && AcceptField(Link, a.Link);
    }

    static bool AcceptField(ui8 a, ui8 b) {
        switch (a) {
        default: return false;
        case Ignore: return true;
        case MustBe: return b;
        case MustNotBe: return !b;
        }
    }

    static TTokenTraits TitleFilter(bool in) {
        TTokenTraits t;
        t.Title = in ? MustBe : MustNotBe;
        return t;
    }
};

struct THashToken : TTokenTraits {
    size_t Hash;
    TAlignedPosting Pos;

    explicit THashToken(size_t hash = 0, TAlignedPosting pos = 0)
        : Hash(hash)
        , Pos(pos)
    {}

};

typedef TVector<size_t> THashValues;

struct THashTokens : public TVector<THashToken> {
    void CollectHashes(THashValues& tokens, TSpan span) const {
        CollectHashes(tokens, span, TTokenTraits());
    }

    void CollectHashes(THashValues& tokens, TTokenTraits filter) const {
        CollectHashes(tokens, TSpan(0, -1), filter);
    }

    void CollectHashes(THashValues& tokens, TSpan span, TTokenTraits filter) const {
        for (const_iterator it = begin(); it != end(); ++it)
            if (filter.Accept(*it) && span.Contains(it->Pos))
                tokens.push_back(it->Hash);
    }
};

}
