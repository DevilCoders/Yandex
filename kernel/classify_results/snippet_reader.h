#pragma once
#include <kernel/lemmer/alpha/abc.h>

#include <util/generic/vector.h>
#include <util/generic/strbuf.h>
#include <util/generic/ptr.h>
#include <util/generic/list.h>
#include <util/generic/hash_set.h>
#include <util/charset/utf8.h>
#include <util/charset/wide.h>
#include <utility>

// aho-corasick style compiled finite automaton
// differences with traditional implementation (as seen in util/draft/aho_corasick.h):
// 1) case-insensitive; also, doesn't make any difference between spaces and any of following symbols: .\/-
// 2) much less generic: works with utf-8 strings only
// 3) several times faster (but eats more memory), as compared to util/draft/aho_corasick.h
template <class TResult, bool RemoveDoubleSpaces = false>
class TSearcherAutomaton {
    struct TState {
        typedef TSearcherAutomaton<TResult, RemoveDoubleSpaces> TParent;

        TVector<TResult> Results;
        TVector<std::pair<unsigned char, TState*> > Transitions;
        TState* Fail;

        TVector<TState*> CompiledTransitions;

        TState()
            : Fail(nullptr)
            , CompiledTransitions(256)
        {
        }

        TState* AddTranslation(TParent *parent, unsigned char c) {
            for (size_t n = 0; n < Transitions.size(); ++n)
                if (Transitions[n].first == c)
                    return Transitions[n].second;
            TState* ret = parent->AddState();
            Transitions.push_back(std::make_pair(c, ret));
            return ret;
        }

        void Unify(const TString &symbols) {
            Y_ASSERT(!Transitions.empty());
            char c = Transitions.back().first;
            if (symbols.find(c) == TString::npos)
                return;
            TState* united = Transitions.back().second;
            for (size_t n = 0; n < symbols.size(); ++n) {
                if (symbols[n] != c)
                    Transitions.push_back(std::make_pair(symbols[n], united));
            }
        }

        void Unify(TState* next, unsigned char c) {
            for (size_t n = 0; n < Transitions.size(); ++n) {
                if (Transitions[n].first == c) {
                    Y_ASSERT(Transitions[n].second == next);
                    return;
                }
            }
            Transitions.push_back(std::make_pair(c, next));
        }
    };

    TVector<TAutoPtr<TState> > States; // storing TAutoPtr into vector is valid in arcadia, or at least pg@ states that :)
    const NLemmer::TAlphabetWordNormalizer *LanguageNormalizer;
    const TString Delimeters;


    TState* AddState() {
        States.push_back(new TState);
        return States.back().Get();
    }

    template <class TWorker>
    bool DoFromState(const TState **cur, const TBasicStringBuf<char> &s, TWorker *res, size_t addLen) const {
        bool prevWasSpace = false;
        for (const char* it = s.data(); it != s.end(); ++it) {
            unsigned char c = *it;
            if (RemoveDoubleSpaces) {
                bool isSpace = false;
                switch (c) {
                    case ' ':
                    case '.':
                    case ',':
                    case '/':
                    case '\\':
                    case '-':
                    case '[':
                    case ']':
                    case '\x07':
                        isSpace = true;
                        break;
                }
                if (prevWasSpace && isSpace)
                    continue;
                prevWasSpace = isSpace;
            }
            *cur = (*cur)->CompiledTransitions[c];
            for (size_t n = 0; n < (*cur)->Results.size(); ++n)
                if (res->Do((*cur)->Results[n], addLen + (it - s.data())))
                    return true;
        }
        return false;
    }
public:
    TSearcherAutomaton(const TString& delimeters=" ./\\-[]\x07")
        : Delimeters(delimeters)
    {
        States.push_back(new TState);
        LanguageNormalizer = NLemmer::GetAlphaRulesUnsafe(LANG_UNK);
    }

    void UseTurkishLanguage(bool yes) {
        LanguageNormalizer = NLemmer::GetAlphaRulesUnsafe(yes ? LANG_TUR : LANG_UNK);
    }

    void AddString(const TString &s, const TResult &res, bool combineDelimiters = false) {
        TState* cur = States[0].Get();
        for (size_t n = 0; n < s.size(); ++n) {
            TState* next = cur->AddTranslation(this, s[n]);
            if (combineDelimiters)
                cur->Unify(Delimeters);
            cur = next;
        }
        cur->Results.push_back(res);
    }

    void AddWtroka(const TUtf16String &w, const TResult &res) {
        TState* cur = States[0].Get();
        TUtf16String lower(w); LanguageNormalizer->ToLower(lower);
        TUtf16String upper(w); LanguageNormalizer->ToUpper(upper);
        TString lowerS = WideToUTF8(lower);
        TString upperS = WideToUTF8(upper);
        Y_ASSERT(lower.size() == upper.size());
        size_t l = 0, u = 0;
        for (size_t n = 0; n < lower.size(); ++n) {
            size_t llen = UTF8RuneLen(lowerS[l]);
            TState* next = cur;
            for (size_t m = 0; m < llen; ++m)
                next = next->AddTranslation(this, lowerS[l + m]);
            l += llen;

            size_t ulen = UTF8RuneLen(upperS[u]);
            TState* nextU = cur;
            for (size_t m = 0; m < ulen - 1; ++m)
                nextU = nextU->AddTranslation(this, upperS[u + m]);
            nextU->Unify(next, upperS[u + ulen - 1]);
            u += ulen;

            if (llen == 1)
                cur->Unify(Delimeters);
            cur = next;
        }
        Y_ASSERT(l == lowerS.size());
        Y_ASSERT(u == upperS.size());
        cur->Results.push_back(res);
    }

    template <class TWorker>
    bool Do(const TBasicStringBuf<char> &s, TWorker *res) const {
        const TState *cur = States[0].Get();
        return DoFromState(&cur, s, res, 0);
    }

    template <class TWorker>
    bool Do(const TBasicStringBuf<char> &prefix, const TBasicStringBuf<char> &s, const TBasicStringBuf<char> &suffix, TWorker *res) const {
        const TState *cur = States[0].Get();
        if (DoFromState(&cur, prefix, res, 0))
            return true;
        size_t done = prefix.length();
        if (DoFromState(&cur, s, res, done))
            return true;
        done += s.length();
        return DoFromState(&cur, suffix, res, done);
    }

    void Finalize() {
        for (size_t n = 0; n < States.size(); ++n) {
            TState* c = States[n].Get();
            for (size_t m = 0; m < c->Transitions.size(); ++m) {
                unsigned char symb = c->Transitions[m].first;
                TState* next = c->Transitions[m].second;
                c->CompiledTransitions[symb] = next;
            }
        }

        TState* root = States[0].Get();
        root->Fail = root;

        TList<TState*> processQueue;
        THashSet<TState*> processed;
        for (size_t n = 0; n < root->Transitions.size(); ++n) {
            TState* v = root->Transitions[n].second;
            if (processed.find(v) == processed.end()) {
                processQueue.push_back(v);
                processed.insert(v);
            }
            v->Fail = root;
        }

        while (!processQueue.empty()) {
            TState* c = processQueue.front();
            processQueue.pop_front();
            for (size_t n = 0; n < c->Transitions.size(); ++n) {
                unsigned char symb = c->Transitions[n].first;
                TState* v = c->Transitions[n].second;
                if (processed.find(v) == processed.end()) {
                    processQueue.push_back(v);
                    processed.insert(v);
                }
                TState* h = c->Fail;
                bool reachedRoot = false;
                while (!h->CompiledTransitions[symb]) {
                    if (h->Fail == h) {
                        v->Fail = h;
                        reachedRoot = true;
                        break;
                    }
                    h = h->Fail;
                }
                if (reachedRoot)
                    continue;
                TState* f = h->CompiledTransitions[symb];
                v->Fail = f;
                for (size_t m = 0; m < f->Results.size(); ++m)
                    v->Results.push_back(f->Results[m]);
            }
        }

        for (size_t n = 0; n < States.size(); ++n) {
            TState* c = States[n].Get();
            for (size_t symb = 0; symb < 256; ++symb) {
                if (c->CompiledTransitions[symb])
                    continue;
                TState* f = c;
                bool found = false;
                while (!found && f != f->Fail) {
                    f = f->Fail;
                    for (size_t m = 0; m < f->Transitions.size(); ++m) {
                        if (f->Transitions[m].first == symb) {
                            f = f->Transitions[m].second;
                            found = true;
                            break;
                        }
                    }
                }
                c->CompiledTransitions[symb] = f;
            }
        }
    }
};

bool IsSnipBreakSymbol(char c);
