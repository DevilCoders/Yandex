#pragma once


template <class TIterator, class TKey>
bool Advance(TIterator& i, TIterator end, const TKey& key) {
    while (i != end && i->first < key) {
        ++i;
    }
    return i != end && i->first == key;
}

template <class TIterator, class TKey>
bool AdvanceSimple(TIterator& i, TIterator end, const TKey& key) {
    while (i != end && *i < key) {
        ++i;
    }
    return i != end && *i == key;
}

template <class TIterator, class TKey, class TIdAcceptor>
bool Advance(TIterator& i, TIterator end, const TKey& key, TIdAcceptor& pred) {
    while (i != end && pred(*i) < key) {
        ++i;
    }
    return i != end && pred(*i) == key;
}
