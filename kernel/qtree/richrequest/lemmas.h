#pragma once

#include "richnode.h"
#include "nodeiterator.h"

#include <util/generic/algorithm.h>
#include <util/generic/vector.h>

class TLemmaProcessor {
public:
    virtual ~TLemmaProcessor() {}
    virtual void OnLemmerWordNode(const TWordNode& wInfo, bool& stop) = 0;
    virtual void OnNonLemmerWordNode(const TWordNode& wInfo, bool& stop) = 0;

    virtual void CollectLemmas(const TRichRequestNode* node) {
        bool stop = false;
        if (IsWord(*node)) {
            const TWordNode* wInfo = node->WordInfo.Get();
            if (wInfo->IsLemmerWord()) {
                OnLemmerWordNode(*wInfo, stop);
            } else {
                OnNonLemmerWordNode(*wInfo, stop);
            }
        } else {
            for (size_t i = 0; i != node->Children.size() && !stop; ++i) {
                CollectLemmas(node->Children[i].Get());
            }
        }
    }
};

class TLemmaTextProcessor : public TLemmaProcessor {
public:
    virtual void OnLemma(const TUtf16String& lemma, bool& stop) = 0;

    void OnLemmerWordNode(const TWordNode& wInfo, bool& stop) override {
        for (TWordNode::TCLemIterator lIt = wInfo.LemsBegin(); lIt != wInfo.LemsEnd() && !stop; ++lIt) {
            OnLemma(lIt->GetLemma(), stop);
        }
    }

    void OnNonLemmerWordNode(const TWordNode& wInfo, bool& stop) override {
        if (!stop) {
            OnLemma(wInfo.GetNormalizedForm(), stop);
        }
    }
};

class TVectorLemmaCollector : public TLemmaTextProcessor {
public:
    TVectorLemmaCollector(TVector<TUtf16String>& lemmas)
        : Lemmas(lemmas)
    {}

    void OnLemma(const TUtf16String& lemma, bool& /*stop*/) override {
        Lemmas.push_back(lemma);
    }
private:
    TVector<TUtf16String>& Lemmas;
};

class THashSetLemmaCollector : public TLemmaTextProcessor {
public:
    THashSetLemmaCollector(THashSet<TUtf16String>& lemmas)
        : Lemmas(lemmas)
    {}

    void OnLemma(const TUtf16String& lemma, bool& /*stop*/) override {
        Lemmas.insert(lemma);
    }
private:
    THashSet<TUtf16String>& Lemmas;
};

class TIntersectionChecker : public TLemmaTextProcessor {
public:
    TIntersectionChecker(const THashSet<TUtf16String>& lemmas)
        : Intersection(false)
        , Lemmas(lemmas)
    {}

    void OnLemma(const TUtf16String& lemma, bool& stop) override {
        if (Lemmas.find(lemma) != Lemmas.end()) {
            Intersection = stop = true;
        }
    }

    bool Intersects() const {
        return Intersection;
    }
private:
    bool Intersection;
    const THashSet<TUtf16String>& Lemmas;
};

template <class TVectorType>
void FilterUniqItems(TVectorType& items) {
    Sort(items.begin(), items.end());
    items.erase(std::unique(items.begin(), items.end()), items.end());
}

class TFormsProcessor {
public:
    virtual void OnForm(const TUtf16String& form, bool& stop) = 0;

    virtual void CollectForms(const TRichRequestNode* node) {
        bool stop = false;
        if (IsWord(*node)) {
            const TWordNode* wInfo = node->WordInfo.Get();
            if (wInfo->IsLemmerWord()) {
                for (TWordNode::TCLemIterator lIt = wInfo->LemsBegin(); lIt != wInfo->LemsEnd(); ++lIt) {
                    for (TLemmaForms::TFormMap::const_iterator it = lIt->GetForms().begin(); it != lIt->GetForms().end(); ++it) {
                        OnForm(it->first, stop);
                    }
                }
            } else {
                OnForm(wInfo->GetNormalizedForm(), stop);
            }
        } else {
            for (size_t i = 0; i != node->Children.size() && !stop; ++i) {
                CollectForms(node->Children[i].Get());
            }
        }
    }
};

class TVectorFormsCollector : public TFormsProcessor {
public:
    TVectorFormsCollector(TVector<TUtf16String>& forms)
        : Forms(forms)
    {}

    void OnForm(const TUtf16String& form, bool& /*stop*/) override {
        Forms.push_back(form);
    }
private:
    TVector<TUtf16String>& Forms;
};

