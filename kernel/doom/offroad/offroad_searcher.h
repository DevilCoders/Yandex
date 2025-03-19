#pragma once

#include "offroad_io_factory.h"
#include "index_tables.h"

#include <kernel/doom/info/info_index_reader.h>

#include <library/cpp/offroad/keyinv/keyinv_searcher.h>
#include <library/cpp/offroad/tuple/limited_tuple_reader.h>
#include <library/cpp/offroad/key/key_reader.h>
#include <library/cpp/offroad/standard/key_data_traits.h>
#include <library/cpp/offroad/key/fat_key_seeker.h>

#include <tuple>

namespace NDoom {

namespace NPrivate {

    template <class Hit, class KeyData, class Vectorizer, class Subtractor, class KeyDataTraits = NOffroad::TKeyDataTraits<KeyData>>
    using TOffroadSearcherBase = NOffroad::TKeyInvSearcher<
        typename KeyDataTraits::TFactory,
        NOffroad::TLimitedTupleReader<Hit, Vectorizer, Subtractor, NOffroad::TDecoder64, 1, NOffroad::PlainOldBuffer>,
        NOffroad::TKeyReader<KeyData, typename KeyDataTraits::TVectorizer, typename KeyDataTraits::TSubtractor>,
        NOffroad::TFatKeySeeker<KeyData, typename KeyDataTraits::TSerializer>
    >;

} // namespace NPrivate

template <EIndexFormat Format, class Hit, class KeyData, class Vectorizer, class Subtractor>
class TOffroadSearcher : public TInfoIndexReader<Format, NPrivate::TOffroadSearcherBase<Hit, KeyData, Vectorizer, Subtractor>> {
    using TBase = TInfoIndexReader<Format, NPrivate::TOffroadSearcherBase<Hit, KeyData, Vectorizer, Subtractor>>;
public:
    using THitTable = typename TBase::THitTable;
    using TKeyTable = typename TBase::TKeyTable;
    using TKey = typename TBase::TKey;
    using TKeyRef = typename TBase::TKeyRef;
    using TIterator = typename TBase::TIterator;

    TOffroadSearcher(const TString& path, bool lockMemory = false) {
        Reset(path, lockMemory);
    }

    void Reset(const TString& path, bool lockMemory = false) {
        Inputs_.Reset(TOffroadIoFactory::OpenReaderInputs(path, lockMemory));
        Tables_.Reset(new TIndexTables<TBase>(Inputs_.Get()));

        Reset(Inputs_->InfoStream.Get(), Tables_.Get(), Tables_.Get(), Inputs_->HitSource, Inputs_->KeySource, Inputs_->FatSource, Inputs_->FatSubSource);
    }

    void Reset(IInputStream* infoInput, THitTable* hitTable, TKeyTable* keyTable, const TBlob& hitSource, const TBlob& keySource, const TBlob& fatSource, const TBlob& fatSubSource) {
        TBase::Reset(infoInput, std::forward_as_tuple(hitTable, hitSource), std::forward_as_tuple(keyTable, keySource), std::forward_as_tuple(fatSource, fatSubSource));
    }

    /* Bring this one here so that our users don't have to traverse 10 levels down the inheritance chain. */

    bool Find(const TKeyRef& key, TIterator* iterator) const {
        return TBase::Find(key, iterator);
    }

private:
    THolder<TOffreadReaderInputs> Inputs_;
    THolder<TIndexTables<TBase>> Tables_;
};


} // namespace NDoom
