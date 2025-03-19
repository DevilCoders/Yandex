#pragma once

#include "offroad_io_factory.h"
#include "index_tables.h"

#include <kernel/doom/info/info_index_writer.h>

#include <library/cpp/offroad/tuple/tuple_writer.h>
#include <library/cpp/offroad/tuple/tuple_sampler.h>
#include <library/cpp/offroad/keyinv/keyinv_writer.h>
#include <library/cpp/offroad/key/fat_key_writer.h>
#include <library/cpp/offroad/key/key_sampler.h>
#include <library/cpp/offroad/key/key_writer.h>
#include <library/cpp/offroad/key/null_key_writer.h>
#include <library/cpp/offroad/standard/key_data_traits.h>

#include <util/stream/null.h>

#include <tuple>

namespace NDoom {

namespace NPrivate {

    template <class Hit, class KeyData, class Vectorizer, class Subtractor, class KeyDataTraits = NOffroad::TKeyDataTraits<KeyData>>
    using TOffroadHitSamplerBase = NOffroad::TKeyInvWriter<
        typename KeyDataTraits::TFactory,
        NOffroad::TTupleSampler<Hit, Vectorizer, Subtractor>,
        NOffroad::TNullKeyWriter<KeyData>
    >;

    template <class Hit, class KeyData, class Vectorizer, class Subtractor, class KeyDataTraits = NOffroad::TKeyDataTraits<KeyData>>
    using TOffroadKeySamplerBase = NOffroad::TKeyInvWriter<
        typename KeyDataTraits::TFactory,
        NOffroad::TTupleWriter<Hit, Vectorizer, Subtractor>,
        NOffroad::TKeySampler<KeyData, typename KeyDataTraits::TVectorizer, typename KeyDataTraits::TSubtractor>
    >;

    template <class Hit, class KeyData, class Vectorizer, class Subtractor, class KeyDataTraits = NOffroad::TKeyDataTraits<KeyData>>
    using TOffroadWriterBase = NOffroad::TKeyInvWriter<
        typename KeyDataTraits::TFactory,
        NOffroad::TTupleWriter<Hit, Vectorizer, Subtractor>,
        NOffroad::TFatKeyWriter<
            NOffroad::TFatOffsetDataWriter<KeyData, typename KeyDataTraits::TSerializer>,
            NOffroad::TKeyWriter<KeyData, typename KeyDataTraits::TVectorizer, typename KeyDataTraits::TSubtractor>
        >
    >;

} // namespace NPrivate


template <class Hit, class KeyData, class Vectorizer, class Subtractor>
class TOffroadHitSampler: public NPrivate::TOffroadHitSamplerBase<Hit, KeyData, Vectorizer, Subtractor> {
    using TBase = NPrivate::TOffroadHitSamplerBase<Hit, KeyData, Vectorizer, Subtractor>;
public:

    void Reset() {
        TBase::Reset(std::forward_as_tuple(), std::forward_as_tuple());
    }
};


template <class Hit, class KeyData, class Vectorizer, class Subtractor>
class TOffroadKeySampler: public NPrivate::TOffroadKeySamplerBase<Hit, KeyData, Vectorizer, Subtractor> {
    using TBase = NPrivate::TOffroadKeySamplerBase<Hit, KeyData, Vectorizer, Subtractor>;
public:
    using THitModel = typename TBase::THitModel;
    using THitTable = typename TBase::THitTable;

    TOffroadKeySampler() {}

    TOffroadKeySampler(const THitModel& model) {
        Reset(model);
    }

    void Reset(const THitModel& model) {
        Table_.Reset(new THitTable(model));
        Reset(Table_.Get());
    }

    void Reset(THitTable* hitTable) {
        TBase::Reset(std::forward_as_tuple(hitTable, &Cnull), std::forward_as_tuple());
    }

private:
    THolder<THitTable> Table_;
};


template<EIndexFormat Format, class Hit, class KeyData, class Vectorizer, class Subtractor>
class TOffroadWriter : public TInfoIndexWriter<Format, NPrivate::TOffroadWriterBase<Hit, KeyData, Vectorizer, Subtractor>> {
    using TBase = TInfoIndexWriter<Format, NPrivate::TOffroadWriterBase<Hit, KeyData, Vectorizer, Subtractor>>;
public:
    using THitModel = typename TBase::THitModel;
    using THitTable = typename TBase::THitTable;
    using TKeyModel = typename TBase::TKeyModel;
    using TKeyTable = typename TBase::TKeyTable;

    TOffroadWriter() {}

    TOffroadWriter(const TString& hitModelPath, const TString& keyModelPath, const TString& path) {
        Reset(hitModelPath, keyModelPath, path);
    }

    TOffroadWriter(const THitModel& hitModel, const TKeyModel& keyModel, const TString& path) {
        Reset(hitModel, keyModel, path);
    }

    ~TOffroadWriter() {
        Finish();
    }

    void Reset(const TString& hitModelPath, const TString& keyModelPath, const TString& path) {
        THitModel hitModel;
        hitModel.Load(hitModelPath);

        TKeyModel keyModel;
        keyModel.Load(keyModelPath);

        Reset(hitModel, keyModel, path);
    }

    void Reset(const THitModel& hitModel, const TKeyModel& keyModel, const TString& path) {
        Outputs_.Reset(TOffroadIoFactory::OpenWriterOutputs(path));
        Tables_.Reset(new TIndexTables<TOffroadWriter>(hitModel, keyModel));

        /* Save models right away so that we don't have to copy them. */
        hitModel.Save(Outputs_->HitModelStream.Get());
        keyModel.Save(Outputs_->KeyModelStream.Get());

        Reset(Outputs_->InfoStream.Get(), Tables_.Get(), Tables_.Get(), Outputs_->HitStream.Get(), Outputs_->KeyStream.Get(), Outputs_->FatStream.Get(), Outputs_->FatSubStream.Get());
    }

    void Reset(IOutputStream* infoOutput, THitTable* hitTable, TKeyTable* keyTable, IOutputStream* hitOutput, IOutputStream* keyOutput, IOutputStream* fatOutput, IOutputStream* fatSubOutput) {
        TBase::Reset(infoOutput, std::forward_as_tuple(hitTable, hitOutput), std::forward_as_tuple(fatOutput, fatSubOutput, keyTable, keyOutput));
    }

    void Finish() {
        if (TBase::IsFinished())
            return;

        TBase::Finish();
        Outputs_->Finish();
    }

private:
    THolder<TOffroadWriterOutputs> Outputs_;
    THolder<TIndexTables<TOffroadWriter>> Tables_;
};


} // namespace NDoom
