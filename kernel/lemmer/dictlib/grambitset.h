#pragma once

#include "grammar_index.h"
#include "tgrammar_processing.h"

#include <library/cpp/langmask/langmask.h>
#include <library/cpp/enumbitset/enumbitset.h>

#include <bitset>
#include <library/cpp/langs/langs.h>
#include <util/generic/set.h>

class TGramBitSet: public TEnumBitSet<EGrammar, gFirst, gMax> {
public:
    class TException: public yexception {
    };

    typedef TEnumBitSet<EGrammar, gFirst, gMax> TParent;

    TGramBitSet()
        : TParent()
    {
    }
    TGramBitSet(const TParent& p)
        : TParent(p)
    {
    } //non-explicit!

    template <class... R>
    explicit TGramBitSet(EGrammar c1, R... r)
        : TParent(c1, r...)
    {
    }

    //static constructor for building TGramBitSet directly from binary mask (writes only lower bits!)
    static TGramBitSet FromInt(unsigned long mask) {
        //unsigned long -> bitset -> TEnumBitSet -> TGramBitSet
        return TGramBitSet(TGramBitSet::TParent(TGramBitSet::TParent::TParent(mask)));
    }

    //from/to human-readable grammems string representation
    static TGramBitSet FromString(const TStringBuf& s, const char* delim = ",; ");
    static TGramBitSet FromWtring(const TWtringBuf& s, const char* delim = ",; ");
    static TGramBitSet FromUnsafeString(const TStringBuf& s, const char* delim = ",; "); // does not throw exception on unknown grammems, returns empty bitset

    TString ToString(const char* delim = " ", bool latinic = true) const;
    void ToString(IOutputStream& stream, const char* delim = " ", bool latinic = true) const;
    TUtf16String ToWtring(const char* delim = " ", bool latinic = true) const;

    // from/to Yandex byte-chain representaion
    static TGramBitSet FromBytes(const char* s);
    static TGramBitSet FromBytes(const char* s, size_t len);
    static TGramBitSet FromBytes(const TString& s) {
        return FromBytes(s.data(), s.length());
    }
    void ToBytes(TString& res) const;

    //from/to hexadecimal string (printable chars)
    static TGramBitSet FromHexString(const TString& s);
    TString ToHexString() const;

    bool Has(EGrammar grammem) const {
        //non-safe, intended for using with const @grammem known at compile-time (for brevity), e.g. grammems.Has(gPlural)
        return Test(grammem);
    }

    // reduce set/vector/collection of grammems to single grammem using operator |
    template <typename TGramBitSetsCollection>
    static TGramBitSet UniteGrammems(const TGramBitSetsCollection& grammems) {
        TGramBitSet res;
        for (typename TGramBitSetsCollection::const_iterator it = grammems.begin(); it != grammems.end(); ++it)
            res |= *it;
        return res;
    }

    // reduce set/vector/collection of grammems to single grammem using operator &
    template <typename TGramBitSetsCollection>
    static TGramBitSet IntersectGrammems(const TGramBitSetsCollection& grammems) {
        TGramBitSet res;
        typename TGramBitSetsCollection::const_iterator it = grammems.begin();
        if (it != grammems.end()) {
            res = *it;
            for (++it; it != grammems.end(); ++it)
                res &= *it;
        }
        return res;
    }

    void ReplaceByMask(const TGramBitSet& src_grammems, const TGramBitSet& mask) {
        *this &= ~mask;
        *this |= src_grammems & mask;
    }

    void ReplaceByMaskIfAny(const TGramBitSet& src_grammems, const TGramBitSet& mask) {
        TGramBitSet rep_grammems = src_grammems & mask;
        if (rep_grammems.any()) {
            *this &= ~mask;
            *this |= rep_grammems;
        }
    }

    TGramBitSet& Tr(const TGramBitSet& src, const TGramBitSet& dst, bool clean = false, bool reset = false) {
        if (HasAll(src)) {
            if (clean)
                *this &= ~src;
            if (reset)
                Reset();
            *this |= dst;
        }
        return *this;
    }

    TGramBitSet& Tr(EGrammar src, EGrammar dst, bool clean = false, bool reset = false) {
        if (Has(src)) {
            if (clean)
                Reset(src);
            if (reset)
                Reset();
            Set(dst);
        }
        return *this;
    }

    TGramBitSet& Tr(EGrammar src, const TGramBitSet& dst, bool clean = false, bool reset = false) {
        if (Has(src)) {
            if (clean)
                Reset(src);
            if (reset)
                Reset();
            *this |= dst;
        }
        return *this;
    }
};

namespace NSpike {
    inline TGramBitSet MakeMask(EGrammar code) {
        TGramBitSet res;
        res.SafeSet(code);
        return res;
    }

    inline TGramBitSet MakeMask(unsigned char c) {
        return MakeMask(NTGrammarProcessing::ch2tg(c));
    }

    const TGramBitSet AllMajorGenders(gMasculine, gFeminine, gNeuter);

    const TGramBitSet AllGenders(gMasculine, gFeminine, gNeuter, gMasFem);

    const TGramBitSet AllMajorCases = TGramBitSet(gNominative, gGenitive, gDative) |
                                      TGramBitSet(gAccusative, gInstrumental, gAblative);

    const TGramBitSet AllMinorCases(gPartitive, gLocative, gVocative);

    const TGramBitSet AllCases = AllMajorCases | AllMinorCases;

    const TGramBitSet AllNumbers(gSingular, gPlural);

    const TGramBitSet AllTimes(gPresent, gFuture, gPast, gNotpast);

    const TGramBitSet AllPersons(gPerson1, gPerson2, gPerson3);

    const TGramBitSet AllGrammems = ~TGramBitSet();

    using TGrammarBunch = TSet<TGramBitSet>;
    using TGramVector = TVector<TGramBitSet>;

    TGramBitSet GrammemesByClass(TGramClass grClass);

    void ToGramBitset(const char* gram, TGramBitSet& bitset);
    void ToGramBitset(const char* stem, const char* flex, TGramBitSet& bitset);
    void ToGrammarBunch(const char* stem, const char* const flex[], size_t size, TGrammarBunch& bunch);
    void ToGrammarBunch(const char* stem, TGrammarBunch& bunch);

    template <typename TGramBitSetsCollection>
    void ToGrammarBunch(const TGramBitSet& stem, const TGramBitSetsCollection& flexes, TGrammarBunch& bunch) {
        for (typename TGramBitSetsCollection::const_iterator it = flexes.begin(); it != flexes.end(); ++it) {
            TGramBitSet bits = stem | *it;
            if (bits.any())
                bunch.insert(bits);
        }
    }

    TGramBitSet Normalize(const TGramBitSet& gram);
    void Normalize(TGrammarBunch* bunch, ELanguage lang = LANG_UNK);

} //end of namespace NSpike
