#pragma once

#include <kernel/gazetteer/worditerator.h>

#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/qtree/richrequest/printrichnode.h>

#include <library/cpp/deprecated/iter/vector.h>

#include <util/generic/noncopyable.h>

namespace NGzt
{

// Checks if @text correctly represents @node original word (lower-cased)
bool IsNodeText(const TRichRequestNode* node, const TUtf16String& text);



// An iterator for lemmas in richnode, which can be a multitoken (with sub-nodes)
// In the case of multitoken iterate over artificial lemmas composed from
// immutable head tokens (all except the last one) and mutable last token
// Used in TRichTreePtrIterator as TLemmaIter.
class TMultitokenLemmaIterator {
public:
    struct TArtificialLemma {
        TArtificialLemma(const TUtf16String& text,  const TVector<TGramBitSet>& flexGram, const TGramBitSet& stemGram)
            : Text(text), FlexGram(flexGram), StemGram(stemGram)
        {
            Text.to_lower();    // minimally imitate lemmer normalization
        }

        TUtf16String Text;
        TVector<TGramBitSet> FlexGram;
        TGramBitSet StemGram;
    };

    TMultitokenLemmaIterator()
        : Node(nullptr)
        , CurArtificialLemma(0)
    {
    }

    TMultitokenLemmaIterator(const TRichRequestNode* node)
        : Node(node)
        , CurArtificialLemma(0)
    {
        Init();
    }

    TMultitokenLemmaIterator(const TRichNodePtr& node)
        : Node(node.Get())
        , CurArtificialLemma(0)
    {
       Init();
    }

    bool Ok() const;
    void operator++();
    TUtf16String GetLemmaText() const;
    TStemFlexGramIterator GetGrammems() const;
    ELanguage GetLanguage() const;

    static bool IsArtificialText(const TRichRequestNode* node, const TUtf16String& text);

private:
    static void BuildArtificial(const TRichRequestNode* node, TVector<TArtificialLemma>& res);
    static bool Check(const TRichRequestNode* node);
    static TUtf16String ArtificialPrefix(const TRichRequestNode* node);

    void Init();

    const TRichRequestNode* Node;
    TDefaultWordInstanceLemmaIter It;
    TVector<TArtificialLemma> ArtificialLemmas;
    int CurArtificialLemma;
};


// TWordIterator specialization for TConstNodesVector (uses default TWordIteratorTraits)
typedef TWordIterator< TVector<const TRichRequestNode*> > TRichRequestNodeIterator;


// TWordIterator specialization for vector of TRichTreePtr
typedef TWordIterator< TVector<TRichNodePtr> > TRichTreePtrIterator;
template <> class TWordIteratorTraits<TVector<TRichNodePtr> > {
public:
    typedef wchar16 TChar;
    typedef TUtf16String TWordString;
    typedef NIter::TVectorIterator<const TWordString> TExactFormIter;
    typedef TMultitokenLemmaIterator TLemmaIter;
    typedef TStemFlexGramIterator TGrammemIter;
    typedef TWideCharTester TCharTester;
};


// A special nodes holder, produces several exact forms from all node's lemma-forms
// Used for iteration over untransliterated or untranslated tree in GeoUntranslate rule of the wizard.
class TExtraFormsRichNodes : TNonCopyable {
public:
    TExtraFormsRichNodes(const TVector<TRichNodePtr>& nodes)
        : Nodes(nodes)
    {
        CollectForms();
    }

    size_t size() const {
        return Nodes.size();
    }

    const TVector<TRichNodePtr>& GetNodes() const {
        return Nodes;
    }

    const TVector<TUtf16String>& GetExtraForms(size_t index) const {
        return ExtraForms[index];
    }

private:
    void CollectForms();

    const TVector<TRichNodePtr>& Nodes;
    TVector< TVector<TUtf16String> > ExtraForms;      // TODO: suboptimal, implement Reset() for re-use.
};


// TWordIterator specialization for vector of TExtraFormsRichNodes
template <> class TWordIteratorTraits<TExtraFormsRichNodes>: public TWordIteratorTraits<TVector<TRichNodePtr> > {
};

template <> class TWordIterator<TExtraFormsRichNodes>: public TRichTreePtrIterator {
public:
    TWordIterator()
        : TRichTreePtrIterator()
        , ExtraFormsInput(nullptr)
    {
    }

    TWordIterator(const TExtraFormsRichNodes& input)
        : TRichTreePtrIterator(input.GetNodes())
        , ExtraFormsInput(&input)
    {
    }

    TExactFormIter IterExtraForms(size_t word_index) const {
        return TExactFormIter(ExtraFormsInput->GetExtraForms(word_index));
    }

    TExactFormIter IterExtraForms() const {
        return IterExtraForms(LastWordIndex());
    }


private:
    const TExtraFormsRichNodes* ExtraFormsInput;
};


class TNormalizedRichNodes {
public:
    TNormalizedRichNodes(const TVector<TRichNodePtr>& nodes)
        : Nodes(nodes)
    {
        NLemmer::TConvertMode mode(NLemmer::CnvConvert, NLemmer::CnvDerenyx);
        NormalizedText.reserve(nodes.size());
        for (const auto& node : nodes) {
            wchar16 buf[64];
            size_t len = 0;
            if (node->WordInfo && node->WordInfo->NumLemmas()) {
                TWtringBuf word = node->WordInfo->GetNormalizedForm();
                // apply the same normalizations as TGztBuilder does when constructing the trie
                // (such as replacing cyrillic "yo" with "ye") and correct some trivial misspellings
                // (e.g. a latin i in the middle of a word in cyrillic script)
                auto res = NLemmer::GetAlphaRules(node->WordInfo->GetLemmas()[0].GetLanguage())->Normalize(word.data(), word.size(), buf, 64, mode);
                if (res.Valid && TWtringBuf(buf, res.Length) != word)
                    len = res.Length;
            }
            NormalizedText.emplace_back(buf, 0, len);
        }
    }

    size_t size() const {
        return Nodes.size();
    }

    const TUtf16String& operator[](size_t word_index) const {
        return NormalizedText[word_index];
    }

    const TVector<TRichNodePtr>& GetNodes() const {
        return Nodes;
    }

private:
    const TVector<TRichNodePtr>& Nodes;
    TVector<TUtf16String> NormalizedText;  // empty elements mean the node's text is already in normal form
};

template <> class TWordIteratorTraits<TNormalizedRichNodes>: public TWordIteratorTraits<TVector<TRichNodePtr> > {
};

template <> class TWordIterator<TNormalizedRichNodes>: public TRichTreePtrIterator {
public:
    TWordIterator()
        : TRichTreePtrIterator()
        , NormalizedInput(nullptr)
    {}

    TWordIterator(const TNormalizedRichNodes& input)
        : TRichTreePtrIterator(input.GetNodes())
        , NormalizedInput(&input)
    {}

    TExactFormIter IterExtraForms(size_t word_index) const {
        if (!NormalizedInput || !(*NormalizedInput)[word_index])
            return TExactFormIter();
        const TUtf16String* norm = &(*NormalizedInput)[word_index];
        return TExactFormIter(norm, norm + 1);
    }

    TExactFormIter IterExtraForms() const {
        return IterExtraForms(LastWordIndex());
    }

private:
    const TNormalizedRichNodes* NormalizedInput;
};


// inlined methods

// TWordIterator specialization for vector of TRichRequestNode*
template <> inline TRichRequestNodeIterator::TWordString TRichRequestNodeIterator::GetWordString(size_t word_index) const {
    const TRichRequestNode* node = (*Input)[word_index];
    if (!!node->WordInfo)
        return node->WordInfo->GetNormalizedForm();
    else
        return node->GetText();   //WordInfo could be empty for some multitokens.
}

template <> inline TRichRequestNodeIterator::TLemmaIter TRichRequestNodeIterator::IterLemmas(size_t word_index) const {
    const TWordNode* w = (*Input)[word_index]->WordInfo.Get();
    if (w != nullptr)
        return TLemmaIter(w->LemsBegin(), w->LemsEnd());
    else
        return TLemmaIter();
}

template <> inline TRichRequestNodeIterator::TWordString TRichRequestNodeIterator::GetLemmaString(const TLemmaIter& lemma) {
    Y_ASSERT(lemma.Ok());
    return lemma->GetLemma();
}

template <> inline TRichRequestNodeIterator::TGrammemIter TRichRequestNodeIterator::IterGrammems(const TLemmaIter& lemma) {
    Y_ASSERT(lemma.Ok());
    return TGrammemIter(lemma->GetStemGrammar(), lemma->GetFlexGrammars());
}

template <> inline ELanguage TRichRequestNodeIterator::GetLanguage(const TLemmaIter& lemma) {
    Y_ASSERT(lemma.Ok());
    return lemma->GetLanguage();
}

template <> inline TRichTreePtrIterator::TWordString TRichTreePtrIterator::GetWordString(size_t word_index) const {
    return GetLowerCaseRichRequestNodeText((*Input)[word_index].Get());
}

template <> inline TRichTreePtrIterator::TWordString TRichTreePtrIterator::GetOriginalWordString(size_t word_index) const {
    return GetOriginalRichRequestNodeText((*Input)[word_index].Get());
}

template <> inline TRichTreePtrIterator::TLemmaIter TRichTreePtrIterator::IterLemmas(size_t word_index) const {
    return TLemmaIter((*Input)[word_index].Get());
}

template <> inline TRichTreePtrIterator::TWordString TRichTreePtrIterator::GetLemmaString(const TLemmaIter& lemma) {
    return lemma.GetLemmaText();
}

template <> inline TRichTreePtrIterator::TGrammemIter TRichTreePtrIterator::IterGrammems(const TLemmaIter& lemma) {
    return lemma.GetGrammems();
}

template <> inline ELanguage TRichTreePtrIterator::GetLanguage(const TLemmaIter& lemma) {
    return lemma.GetLanguage();
}

}   // namespace NGzt
