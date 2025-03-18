#pragma once

#include <tuple>

#include <util/stream/null.h>

#include <library/cpp/offroad/tuple/tuple_writer.h>
#include <library/cpp/offroad/tuple/tuple_sampler.h>
#include <library/cpp/offroad/keyinv/keyinv_writer.h>
#include <library/cpp/offroad/key/fat_key_writer.h>
#include <library/cpp/offroad/key/key_sampler.h>
#include <library/cpp/offroad/key/key_writer.h>
#include <library/cpp/offroad/key/null_key_writer.h>

#include "key_data_traits.h"

namespace NOffroad {
    namespace NPrivate {
        template <class Hit, class KeyData, class Vectorizer, class Subtractor, class KeyDataTraits = TKeyDataTraits<KeyData>>
        using TStandardIndexHitSamplerBase = TKeyInvWriter<
            typename KeyDataTraits::TFactory,
            TTupleSampler<Hit, Vectorizer, Subtractor>,
            TNullKeyWriter<KeyData>>;

        template <class Hit, class KeyData, class Vectorizer, class Subtractor, class KeyDataTraits = TKeyDataTraits<KeyData>>
        using TStandardIndexKeySamplerBase = TKeyInvWriter<
            typename KeyDataTraits::TFactory,
            TTupleWriter<Hit, Vectorizer, Subtractor>,
            TKeySampler<KeyData, typename KeyDataTraits::TVectorizer, typename KeyDataTraits::TSubtractor>>;

        template <class Hit, class KeyData, class Vectorizer, class Subtractor, class KeyDataTraits = TKeyDataTraits<KeyData>>
        using TStandardIndexWriterBase = TKeyInvWriter<
            typename KeyDataTraits::TFactory,
            TTupleWriter<Hit, Vectorizer, Subtractor>,
            TFatKeyWriter<
                TFatOffsetDataWriter<KeyData, typename KeyDataTraits::TSerializer>,
                TKeyWriter<KeyData, typename KeyDataTraits::TVectorizer, typename KeyDataTraits::TSubtractor>>>;

    }

    template <class Hit, class KeyData, class Vectorizer, class Subtractor>
    class TStandardIndexHitSampler: public NPrivate::TStandardIndexHitSamplerBase<Hit, KeyData, Vectorizer, Subtractor> {
        using TBase = NPrivate::TStandardIndexHitSamplerBase<Hit, KeyData, Vectorizer, Subtractor>;

    public:
        TStandardIndexHitSampler() = default;

        void Reset() {
            TBase::Reset(std::forward_as_tuple(), std::forward_as_tuple());
        }
    };

    template <class Hit, class KeyData, class Vectorizer, class Subtractor>
    class TStandardIndexKeySampler: public NPrivate::TStandardIndexKeySamplerBase<Hit, KeyData, Vectorizer, Subtractor> {
        using TBase = NPrivate::TStandardIndexKeySamplerBase<Hit, KeyData, Vectorizer, Subtractor>;

    public:
        using THitTable = typename TBase::THitTable;

        TStandardIndexKeySampler() = default;

        TStandardIndexKeySampler(THitTable* hitTable)
            : TBase(std::forward_as_tuple(hitTable, &Cnull), std::forward_as_tuple())
        {
        }

        void Reset(THitTable* hitTable) {
            TBase::Reset(std::forward_as_tuple(hitTable, &Cnull), std::forward_as_tuple());
        }
    };

    template <class Hit, class KeyData, class Vectorizer, class Subtractor>
    class TStandardIndexWriter: public NPrivate::TStandardIndexWriterBase<Hit, KeyData, Vectorizer, Subtractor> {
        using TBase = NPrivate::TStandardIndexWriterBase<Hit, KeyData, Vectorizer, Subtractor>;

    public:
        using THitTable = typename TBase::THitTable;
        using TKeyTable = typename TBase::TKeyTable;

        TStandardIndexWriter() = default;

        TStandardIndexWriter(THitTable* hitTable, TKeyTable* keyTable, IOutputStream* hitOutput, IOutputStream* keyOutput, IOutputStream* fatOutput, IOutputStream* fatSubOutput)
            : TBase(std::forward_as_tuple(hitTable, hitOutput), std::forward_as_tuple(fatOutput, fatSubOutput, keyTable, keyOutput))
        {
        }

        void Reset(THitTable* hitTable, TKeyTable* keyTable, IOutputStream* hitOutput, IOutputStream* keyOutput, IOutputStream* fatOutput, IOutputStream* fatSubOutput) {
            TBase::Reset(std::forward_as_tuple(hitTable, hitOutput), std::forward_as_tuple(fatOutput, fatSubOutput, keyTable, keyOutput));
        }
    };

}
