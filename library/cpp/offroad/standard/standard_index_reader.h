#pragma once

#include <tuple>

#include <library/cpp/offroad/tuple/limited_tuple_reader.h>
#include <library/cpp/offroad/keyinv/keyinv_reader.h>
#include <library/cpp/offroad/key/key_reader.h>
#include <library/cpp/offroad/key/fat_key_reader.h>

#include "key_data_traits.h"

namespace NOffroad {
    namespace NPrivate {
        template <class Hit, class KeyData, class Vectorizer, class Subtractor, class KeyDataTraits = TKeyDataTraits<KeyData>>
        using TStandardIndexReaderBase = TKeyInvReader<
            typename KeyDataTraits::TFactory,
            TLimitedTupleReader<Hit, Vectorizer, Subtractor, TDecoder64, 1, PlainOldBuffer>,
            TFatKeyReader<
                typename KeyDataTraits::TSerializer,
                TKeyReader<KeyData, typename KeyDataTraits::TVectorizer, typename KeyDataTraits::TSubtractor>>>;
    }

    template <class Hit, class KeyData, class Vectorizer, class Subtractor>
    class TStandardIndexReader: public NPrivate::TStandardIndexReaderBase<Hit, KeyData, Vectorizer, Subtractor> {
        using TBase = NPrivate::TStandardIndexReaderBase<Hit, KeyData, Vectorizer, Subtractor>;

    public:
        using THitTable = typename TBase::THitTable;
        using TKeyTable = typename TBase::TKeyTable;

        TStandardIndexReader() = default;

        TStandardIndexReader(THitTable* hitTable, TKeyTable* keyTable, const TArrayRef<const char>& hitSource, const TArrayRef<const char>& keySource, const TArrayRef<const char>& fatSource, const TArrayRef<const char>& fatSubSource)
            : TBase(std::forward_as_tuple(hitTable, hitSource), std::forward_as_tuple(fatSource, fatSubSource, keyTable, keySource))
        {
        }

        void Reset(THitTable* hitTable, TKeyTable* keyTable, const TArrayRef<const char>& hitSource, const TArrayRef<const char>& keySource, const TArrayRef<const char>& fatSource, const TArrayRef<const char>& fatSubSource) {
            TBase::Reset(std::forward_as_tuple(hitTable, hitSource), std::forward_as_tuple(fatSource, fatSubSource, keyTable, keySource));
        }
    };

}
