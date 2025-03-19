#pragma once

/// @file hash.h Специализации хэшей.

namespace NRemorph {
namespace NCommon {

/// Хэш для контейнера.
template <class TContainer>
struct TContainerHash {
    THash<typename TContainer::value_type> ValHash;

    inline size_t operator()(const TContainer& container) const {
        if (container.empty()) {
            return 0;
        }

        typename TContainer::const_iterator i = container.begin();
        size_t hash = ValHash(*i++);
        while (i != container.end()) {
            hash = ::CombineHashes(hash, ValHash(*i++));
        }

        return hash;
    }
};

} // NCommon
} // NRemorph
