#pragma once

#include <utility>                  /* For std::forward. */

#include <util/generic/strbuf.h>

namespace NDoom {


template<class KeyFilter, class Base>
class TKeyFilteringIndexReader : public Base {
public:
    using TKeyRef = typename Base::TKeyRef;
    using TKeyData = typename Base::TKeyData;

    using Base::Base; /* For default-constructible key filter. */

    template<class... Args>
    TKeyFilteringIndexReader(const KeyFilter& keyFilter, Args&&... args) : Base(std::forward<Args>(args)...), KeyFilter_(keyFilter) {}

    bool ReadKey(TKeyRef* key, TKeyData* data = NULL) {
        while (Base::ReadKey(key, data)) {
            if (KeyFilter_(*key)) {
                return true;
            } else {
                continue;
            }
        }

        return false;
    }

private:
    KeyFilter KeyFilter_;
};


/**
 * Key filter that accepts keys only up to (and including) provided maximal length.
 */
template<size_t MaxLength>
struct TLengthKeyFilter {
    bool operator()(const TStringBuf& key) const {
        return key.size() <= MaxLength;
    }
};


/**
 * Key filter that accepts only regular keys (that is, keys with empty prefix).
 *
 * Attribute & zone keys are filtered out.
 */
struct TEmptyPrefixKeyFilter {
    bool operator()(const TStringBuf& key) const {
        return key && (key.size() == 1 || key[0] != '#' && key[0] != '(' && key[0] != ')');
    }
};


/**
 * Key filter that accepts regular keys, plus positional telephone attributes.
 */
struct TEmptyPrefixExceptTelKeyFilter {
    bool operator()(const TStringBuf& key) const {
        if (key.StartsWith("#tel_full") || key.StartsWith("#tel_local")) {
            return true;
        }
        return TEmptyPrefixKeyFilter()(key);
    }
};


} // namespace NDoom
