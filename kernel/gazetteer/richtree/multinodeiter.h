#pragma once

#include "richnodeiter.h"
#include <kernel/gazetteer/common/iterators.h>

#include <util/generic/noncopyable.h>
#include <util/generic/vector.h>

namespace NGzt {

// Memory efficient alternative to TVector< TVector<T> >
template <typename T>
class TMultiVector : TNonCopyable {
public:
    TMultiVector()
        : UsedSize(0)
    {
    }

    size_t Size() const {
        return UsedSize;
    }

    const TVector<T>& operator [] (size_t index) const {
        Y_ASSERT(index < UsedSize);
        return SubVectors[index];
    }

    TVector<T>& operator [] (size_t index) {
        Y_ASSERT(index < UsedSize);
        return SubVectors[index];
    }


    void Clear() {
        for (size_t i = 0; i < UsedSize; ++i)
            SubVectors[i].clear();
        UsedSize = 0;
    }

    TVector<T>& PushBack() {
        while (UsedSize >= SubVectors.size())
            SubVectors.emplace_back();
        return SubVectors[UsedSize++];
    }

    void PushBack(TVector<T>& nodes) {
        nodes.swap(PushBack());
    }

    void Swap(TMultiVector<T>& t) {
        ::DoSwap(SubVectors, t.SubVectors);
        ::DoSwap(UsedSize, t.UsedSize);
    }

    //typedef TVectorIterator<const TVector<T> > TSubVectorIter;
    //typedef TSuperIterator<TVectorIterator<const T>, TSubVectorIter> TJoinedIter;

private:
    TVector< TVector<T> > SubVectors;
    size_t UsedSize;
};



class TMultiNodeVector: public TMultiVector<TRichNodePtr> {
    typedef TMultiVector<TRichNodePtr> TBase;
    typedef TVector<TRichNodePtr> TMultiNode;
    typedef TVectorIterator<const TRichNodePtr> TSubNodeIter;

    struct TNodeTextExtractor {
        typedef TUtf16String TResult;
        TResult operator()(const TRichNodePtr& node) const {
            return GetRichRequestNodeText(node.Get());
        }
    };

public:
    // required for TWordIterator<TMultiNodeVector>
    size_t size() const {
        return TBase::Size();
    }

    TUtf16String GetOriginalWordString(size_t wordIndex) const {
        const TMultiNode& multinode = TBase::operator[](wordIndex);
        return multinode.size() ? GetOriginalRichRequestNodeText(multinode[0].Get()) : TUtf16String();
    }

    TUtf16String GetNormalizedWordString(size_t wordIndex) const {
        const TMultiNode& multinode = TBase::operator[](wordIndex);
        return multinode.size() ? GetRichRequestNodeText(multinode[0].Get()) : TUtf16String();
    }

    typedef TMappedIterator2<TSubNodeIter, TNodeTextExtractor> TExtraFormIter;

    TExtraFormIter IterExtraForms(size_t wordIndex) const {
        const TMultiNode& multinode = TBase::operator[](wordIndex);
        TSubNodeIter subIt(multinode);
        TExtraFormIter it(subIt);
        if (it.Ok())
            ++it;   // skip first sub-node, as its text will be returned to gazetteer with GetXXXWordString

        return it;
    }


    typedef TSuperIterator<TMultitokenLemmaIterator, TSubNodeIter> TLemmaIter;

    TLemmaIter IterLemmas(size_t wordIndex) const {
        const TMultiNode& multinode = TBase::operator[](wordIndex);
        return TLemmaIter(TSubNodeIter(multinode));
    }
};



// TWordIterator specialization for TMultiNodeVector
typedef TWordIterator<TMultiNodeVector> TMultiNodeVectorIterator;
template <> class TWordIteratorTraits<TMultiNodeVector> {
public:
    typedef wchar16 TChar;
    typedef TUtf16String TWordString;
    typedef TMultiNodeVector::TExtraFormIter TExactFormIter;
    typedef TMultiNodeVector::TLemmaIter TLemmaIter;
    typedef TStemFlexGramIterator TGrammemIter;
    typedef TWideCharTester TCharTester;
};


// inlined methods

template <> inline TMultiNodeVectorIterator::TWordString TMultiNodeVectorIterator::GetWordString(size_t word) const {
    return Input->GetNormalizedWordString(word);
}

template <> inline TMultiNodeVectorIterator::TWordString TMultiNodeVectorIterator::GetOriginalWordString(size_t word) const {
    return Input->GetOriginalWordString(word);
}

template <> inline TMultiNodeVectorIterator::TExactFormIter TMultiNodeVectorIterator::IterExtraForms(size_t word) const {
    return Input->IterExtraForms(word);
}

template <> inline TMultiNodeVectorIterator::TLemmaIter TMultiNodeVectorIterator::IterLemmas(size_t word) const {
    return Input->IterLemmas(word);
}

template <> inline TMultiNodeVectorIterator::TWordString TMultiNodeVectorIterator::GetLemmaString(const TLemmaIter& lemma) {
    return lemma.GetLemmaText();
}

template <> inline TMultiNodeVectorIterator::TGrammemIter TMultiNodeVectorIterator::IterGrammems(const TLemmaIter& lemma) {
    return lemma.GetGrammems();
}

template <> inline ELanguage TMultiNodeVectorIterator::GetLanguage(const TLemmaIter& lemma) {
    return lemma.GetLanguage();
}


}   // namespace NGzt

