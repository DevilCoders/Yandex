#pragma once

#include "worditerator.h"

#include <util/generic/strbuf.h>

namespace NGzt {

// Special gzt input type, represents single word without morphology
class TSingleWordInput {
public:
    TSingleWordInput(TWtringBuf word)
        : Word(word)
    {
    }

    // required by TWordIterator
    size_t size() const {
        return 1;
    }

    const TWtringBuf& operator [] (size_t i) const {
        Y_ASSERT(i == 0);
        return Word;
    }

public:
    TWtringBuf Word;
};

// TWordIterator specialization for TSingleWordInput with only EXACT_FORMS search
using TSingleWordIterator = TWordIterator<TSingleWordInput>;

template <>
class TWordIteratorTraits<TSingleWordInput> {
public:
    using TChar = wchar16;
    using TWordString = TWtringBuf;
    using TExactFormIter = NIter::TValueIterator<const TWordString>;
    using TLemmaIter = NIter::TVectorIterator<const TWordString>;
    using TGrammemIter = NIter::TVectorIterator<const TGramBitSet>;
    using TCharTester = TWideCharTester;
};

template <>
inline TSingleWordIterator::TWordString TSingleWordIterator::GetWordString(size_t wordIndex) const {
    return (*Input)[wordIndex];
}

template <>
inline TSingleWordIterator::TLemmaIter TSingleWordIterator::IterLemmas(size_t) const {
    // no lemmas, only EXACT_FORM search is supported
    return TLemmaIter();
}

template <>
inline TSingleWordIterator::TWordString TSingleWordIterator::GetLemmaString(const TLemmaIter&) {
    // no lemmas
    return TWordString();
}

template <>
inline TSingleWordIterator::TGrammemIter TSingleWordIterator::IterGrammems(const TLemmaIter&) {
    // no lemmas
    return TGrammemIter();
}


}   // namespace NGzt
