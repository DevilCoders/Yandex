#pragma once

#include "grammar_enum.h"

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NTGrammarProcessing {
    inline EGrammar ch2tg(char c) {
        return (EGrammar)(unsigned char)c;
    }

    inline char tg2ch(EGrammar c) {
        return (char)c;
    }

    inline size_t GetGramIndex(const char* grammar, const char* grs) {
        return strcspn(grammar, grs);
    }

    inline bool HasGram(const char* grammar, EGrammar gr) {
        return strchr(grammar, tg2ch(gr)) != nullptr;
    }

    inline bool HasGram(const char* grammar, const char* grs) {
        return grammar[GetGramIndex(grammar, grs)] != 0;
    }

    inline const char* HasGram(const char* const flexGram[], size_t gramNum, EGrammar gram) {
        for (size_t i = 0; i < gramNum; ++i) {
            if (NTGrammarProcessing::HasGram(flexGram[i], gram))
                return flexGram[i];
        }
        return nullptr;
    }

    inline bool HasAllGrams(const char* grammar, const char* needed) {
        for (; *needed; needed++) {
            if (!HasGram(grammar, ch2tg(*needed)))
                return false;
        }
        return true;
    }

    inline EGrammar GetGram(const char* grammar, size_t ind) {
        return ch2tg(grammar[ind]);
    }

    inline EGrammar GetGramOf(const char* grammar, const char* grs) {
        return ch2tg(grammar[GetGramIndex(grammar, grs)]);
    }

    inline size_t GetGramIndex(const TString& grammar, const char* grs) {
        return GetGramIndex(grammar.c_str(), grs);
    }

    inline bool HasGram(const TString& grammar, EGrammar gr) {
        return HasGram(grammar.c_str(), gr);
    }

    inline bool HasGram(const TString& grammar, const char* grs) {
        return HasGram(grammar.c_str(), grs);
    }

    inline EGrammar GetGram(const TString& grammar, size_t ind) {
        return GetGram(grammar.c_str(), ind);
    }

    inline EGrammar GetGramOf(const TString& grammar, const char* grs) {
        return GetGramOf(grammar.c_str(), grs);
    }

}

class TGrammarVector: public TVector<EGrammar> {
public:
    typedef TVector<EGrammar> TParent;

public:
    TGrammarVector(const char* grs = nullptr) {
        AddCharGrammas(grs);
    }

    TGrammarVector(const TString& grs) {
        AddCharGrammas(grs);
    }

    using TParent::push_back;
    void push_back(char gr) {
        TParent::push_back(NTGrammarProcessing::ch2tg(gr));
    }

    void AddCharGrammas(const TString& grs) {
        AddCharGrammas(grs.c_str());
    }

    void AddCharGrammas(const char* grs) {
        if (!grs)
            return;

        while (*grs)
            push_back(*(grs++));
    }

    inline size_t GetGramIndex(EGrammar gr) const {
        const_iterator i = begin();
        for (; i != end(); ++i) {
            if (*i == gr)
                break;
        }
        return i - begin();
    }

    inline bool HasGram(EGrammar gr) const {
        return GetGramIndex(gr) < size();
    }

    inline size_t GetGramIndex(const char* grs) const {
        if (!grs)
            return size();
        return GetGramIndex(grs, grs);
    }

    inline size_t GetGramIndex(const TGrammarVector& grs) const {
        return GetGramIndex(grs.begin(), grs.end());
    }

    inline bool HasGram(const char* grs) const {
        return GetGramIndex(grs) < size();
    }

    inline bool HasGram(const TGrammarVector& grs) const {
        return GetGramIndex(grs) < size();
    }

    inline EGrammar GetGramOf(const char* grs) const {
        return GetGramOfT(grs);
    }

    inline EGrammar GetGramOf(const TGrammarVector& grs) const {
        return GetGramOfT<const TGrammarVector&>(grs);
    }

    inline bool HasAllGrams(const char* grs) const {
        if (!grs)
            return size() > 0;
        return GetGramIndex(grs, grs) > 0;
    }

    inline bool HasAllGrams(const TGrammarVector& grs) const {
        return GetGramIndex(grs.begin(), grs.end()) > 0;
    }

private:
    inline size_t GetGramIndex(char gr) const {
        return GetGramIndex(NTGrammarProcessing::ch2tg(gr));
    }

    inline bool HasGram(char gr) const {
        return HasGram(NTGrammarProcessing::ch2tg(gr));
    }

    template <class Ti>
    inline size_t GetGramIndex(Ti grs, Ti end) const {
        Ti i = grs;
        for (; Valid(i, end); ++i) {
            if (GetGramIndex(*i) < size())
                break;
        }
        return i - grs;
    }

    template <class Ti>
    inline bool HasAllGrams(Ti grs, Ti end) const {
        for (; Valid(grs, end); ++grs) {
            if (!HasGram(*grs))
                return false;
        }
        return true;
    }

    template <class T>
    inline EGrammar GetGramOfT(T grs) const {
        size_t r = GetGramIndex(grs);
        if (r < size())
            return *(begin() + r);
        return gInvalid;
    }

    static bool Valid(const char* p, const char*) {
        return *p != 0;
    }
    static bool Valid(const_iterator i, const_iterator end) {
        return i != end;
    }
};
