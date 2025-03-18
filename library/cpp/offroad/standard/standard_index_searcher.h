#pragma once

#include <tuple>

#include <library/cpp/offroad/keyinv/keyinv_searcher.h>
#include <library/cpp/offroad/tuple/limited_tuple_reader.h>
#include <library/cpp/offroad/key/key_reader.h>
#include <library/cpp/offroad/key/fat_key_seeker.h>

#include "key_data_traits.h"

namespace NOffroad {
    namespace NPrivate {
        template <class Hit, class KeyData, class Vectorizer, class Subtractor, class KeyDataTraits = TKeyDataTraits<KeyData>>
        using TStandardIndexSearcherBase = TKeyInvSearcher<
            typename KeyDataTraits::TFactory,
            TLimitedTupleReader<Hit, Vectorizer, Subtractor, TDecoder64, 1, PlainOldBuffer>,
            TKeyReader<KeyData, typename KeyDataTraits::TVectorizer, typename KeyDataTraits::TSubtractor>,
            TFatKeySeeker<KeyData, typename KeyDataTraits::TSerializer>>;
    }

    template <class Hit, class KeyData, class Vectorizer, class Subtractor>
    class TStandardIndexSearcher: public NPrivate::TStandardIndexSearcherBase<Hit, KeyData, Vectorizer, Subtractor> {
        using TBase = NPrivate::TStandardIndexSearcherBase<Hit, KeyData, Vectorizer, Subtractor>;

    public:
        using THitTable = typename TBase::THitTable;
        using TKeyTable = typename TBase::TKeyTable;

        TStandardIndexSearcher() = default;

        TStandardIndexSearcher(THitTable* hitTable, TKeyTable* keyTable, const TArrayRef<const char>& hitSource, const TArrayRef<const char>& keySource, const TArrayRef<const char>& fatSource, const TArrayRef<const char>& fatSubSource)
            : TBase(std::forward_as_tuple(hitTable, hitSource), std::forward_as_tuple(keyTable, keySource), std::forward_as_tuple(fatSource, fatSubSource))
        {
        }

        void Reset(THitTable* hitTable, TKeyTable* keyTable, const TArrayRef<const char>& hitSource, const TArrayRef<const char>& keySource, const TArrayRef<const char>& fatSource, const TArrayRef<const char>& fatSubSource) {
            TBase::Reset(std::forward_as_tuple(hitTable, hitSource), std::forward_as_tuple(keyTable, keySource), std::forward_as_tuple(fatSource, fatSubSource));
        }
    };

}
