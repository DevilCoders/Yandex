#pragma once

#include "offroad_io_factory.h"
#include "index_tables.h"

#include <kernel/doom/info/info_index_reader.h>
#include <kernel/doom/progress/progress.h>

#include <library/cpp/offroad/tuple/limited_tuple_reader.h>
#include <library/cpp/offroad/keyinv/keyinv_reader.h>
#include <library/cpp/offroad/key/key_reader.h>
#include <library/cpp/offroad/key/fat_key_reader.h>
#include <library/cpp/offroad/standard/key_data_traits.h>

#include <tuple>

namespace NDoom {


namespace NPrivate {

    template<class Hit, class KeyData, class Vectorizer, class Subtractor, class KeyDataTraits = NOffroad::TKeyDataTraits<KeyData>>
    using TOffroadReaderBase = NOffroad::TKeyInvReader<
        typename KeyDataTraits::TFactory,
        NOffroad::TLimitedTupleReader<Hit, Vectorizer, Subtractor, NOffroad::TDecoder64, 1, NOffroad::PlainOldBuffer>,
        NOffroad::TFatKeyReader<
            typename KeyDataTraits::TSerializer,
            NOffroad::TKeyReader<KeyData, typename KeyDataTraits::TVectorizer, typename KeyDataTraits::TSubtractor>
        >
    >;

} // namespace NPrivate

template<EIndexFormat Format, class Hit, class KeyData, class Vectorizer, class Subtractor>
class TOffroadReader : public TInfoIndexReader<Format, NPrivate::TOffroadReaderBase<Hit, KeyData, Vectorizer, Subtractor>> {
    using TBase = TInfoIndexReader<Format, NPrivate::TOffroadReaderBase<Hit, KeyData, Vectorizer, Subtractor>>;
public:
    using THitTable = typename TBase::THitTable;
    using TKeyTable = typename TBase::TKeyTable;

    enum {
        HasLowerBound = false,
        HitLayers = 1
    };

    TOffroadReader() {}

    TOffroadReader(const TString& path) {
        Reset(path);
    }

    void Reset(const TString& path) {
        Inputs_.Reset(TOffroadIoFactory::OpenReaderInputs(path));
        Tables_.Reset(new TIndexTables<TOffroadReader>(Inputs_.Get()));

        Reset(Inputs_->InfoStream.Get(), Tables_.Get(), Tables_.Get(), Inputs_->HitSource, Inputs_->KeySource, Inputs_->FatSource, Inputs_->FatSubSource);
    }

    void Reset(IInputStream* infoInput, THitTable* hitTable, TKeyTable* keyTable, const TBlob& hitSource, const TBlob& keySource, const TBlob& fatSource, const TBlob& fatSubSource) {
        TBase::Reset(infoInput, std::forward_as_tuple(hitTable, hitSource), std::forward_as_tuple(fatSource, fatSubSource, keyTable, keySource));
    }

    TProgress Progress() const {
        return TProgress(); // TODO
    }

private:
    THolder<TOffreadReaderInputs> Inputs_;
    THolder<TIndexTables<TOffroadReader>> Tables_;
};


} // namespace NDoom
