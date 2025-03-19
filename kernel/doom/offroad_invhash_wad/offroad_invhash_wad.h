#pragma once

#include "invhash_entry.h"

#include <kernel/doom/wad/mega_wad_writer.h>
#include <kernel/doom/wad/wad.h>

#include <library/cpp/offroad/custom/subtractors.h>
#include <library/cpp/offroad/sub/sub_reader.h>
#include <library/cpp/offroad/sub/sub_sampler.h>
#include <library/cpp/offroad/sub/sub_seeker.h>
#include <library/cpp/offroad/sub/sub_writer.h>
#include <library/cpp/offroad/tuple/tuple_reader.h>
#include <library/cpp/offroad/tuple/tuple_sampler.h>
#include <library/cpp/offroad/tuple/tuple_writer.h>
#include <library/cpp/offroad/utility/tagged.h>

namespace NDoom {

class TInvHashBaseIo {
public:
    using Hit = TInvHashEntry;
    using Vectorizer = TInvHashEntryVectorizer;
    using PrefixVectorizer = TInvHashEntryPrefixVectorizer;

    using Subtractor = NOffroad::TPD2D1I1Subtractor;

    using TWriter = NOffroad::TSubWriter<PrefixVectorizer, NOffroad::TTupleWriter<Hit, Vectorizer, Subtractor, NOffroad::TEncoder64, 1, NOffroad::AutoEofBuffer>>;
    using TSampler = NOffroad::TSubSampler<PrefixVectorizer, NOffroad::TTupleSampler<Hit, Vectorizer, Subtractor, NOffroad::TSampler64, NOffroad::AutoEofBuffer>>;
    using TReader = NOffroad::TSubReader<PrefixVectorizer, NOffroad::TTupleReader<Hit, Vectorizer, Subtractor, NOffroad::TDecoder64, 1, NOffroad::AutoEofBuffer>>;
};

using TInvHashWadSampler = TInvHashBaseIo::TSampler;

class TInvHashLumps {
public:
    static TWadLumpId ModelLump() {
        return TWadLumpId(EWadIndexType::InvUrlHashesIndexType, EWadLumpRole::HitsModel);
    }

    static TWadLumpId HitsLump() {
        return TWadLumpId(EWadIndexType::InvUrlHashesIndexType, EWadLumpRole::Hits);
    }

    static TWadLumpId HitSubLump() {
        return TWadLumpId(EWadIndexType::InvUrlHashesIndexType, EWadLumpRole::HitSub);
    }
};

class TInvHashWadWriter {
    using TWriter = TInvHashBaseIo::TWriter;
public:
    using TTable = typename TWriter::TTable;
    using TModel = typename TWriter::TModel;

    void Reset(const TModel& model, IWadWriter* writer) {
        SubOutput_.Reset();

        Table_.Reset(new TTable(model));

        WadWriter_ = writer;

        model.Save(WadWriter_->StartGlobalLump(TInvHashLumps::ModelLump()));
        Writer_.Reset(&SubOutput_, Table_.Get(), WadWriter_->StartGlobalLump(TInvHashLumps::HitsLump()));
    }

    void WriteHit(const TInvHashBaseIo::Hit hit) {
        // due to usage of AutoEof, 1 is subtracted from doc id
        Y_ENSURE(hit.DocId != Max<ui32>(), "Index is not allowed to contain Max<ui32>() doc ids");

        Writer_.WriteHit(hit);
    }

    void Finish() {
        Writer_.Finish();
        SubOutput_.Flush(WadWriter_->StartGlobalLump(TInvHashLumps::HitSubLump()));
    }

private:
    THolder<TTable> Table_;
    TAccumulatingOutput SubOutput_;
    TWriter Writer_;
    IWadWriter* WadWriter_ = nullptr;
};

class TInvHashWadReader {
public:
    void Reset(const IWad* wad) {
        SubBlob_ = wad->LoadGlobalLump(TInvHashLumps::HitSubLump());
        HitsBlob_ = wad->LoadGlobalLump(TInvHashLumps::HitsLump());
        Model_.Load(wad->LoadGlobalLump(TInvHashLumps::ModelLump()));
        DecoderTable_.Reset(new TInvHashBaseIo::TSampler::TModel::TDecoderTable(Model_));

        Reader_.Reset(TArrayRef<const char>(reinterpret_cast<const char*>(SubBlob_.Data()), SubBlob_.Size()), DecoderTable_.Get(), HitsBlob_);
    }

    bool ReadHit(TInvHashBaseIo::Hit* hit) {
        return Reader_.ReadHit(hit);
    }

private:
    TInvHashBaseIo::TSampler::TModel Model_;
    THolder<TInvHashBaseIo::TSampler::TModel::TDecoderTable> DecoderTable_;
    TBlob SubBlob_;
    TBlob HitsBlob_;

    TInvHashBaseIo::TReader Reader_;
};

class TInvHashWadSearcher;

class TInvHashWadIterator : private NOffroad::NPrivate::TTaggedBase {
    friend class TInvHashWadSearcher;
private:
    TInvHashBaseIo::TReader Reader_;
};

class TInvHashWadSearcher {
public:
    friend class TInvHashWadIterator;

    void Reset(const IWad* wad) {
        SubBlob_ = wad->LoadGlobalLump(TInvHashLumps::HitSubLump());
        HitsBlob_ = wad->LoadGlobalLump(TInvHashLumps::HitsLump());
        Model_.Load(wad->LoadGlobalLump(TInvHashLumps::ModelLump()));
        DecoderTable_.Reset(new TInvHashBaseIo::TSampler::TModel::TDecoderTable(Model_));
    }

    bool Search(const ui64 hash, ui32* outputDocId, TInvHashWadIterator* iterator) const {
        if (iterator->Tag() != this) {
            iterator->SetTag(this);
            iterator->Reader_.Reset(TArrayRef<const char>(reinterpret_cast<const char*>(SubBlob_.Data()), SubBlob_.Size()), DecoderTable_.Get(), HitsBlob_);
        }

        TInvHashBaseIo::TReader& reader = iterator->Reader_;

        TInvHashEntry seekKey;
        seekKey.Hash = hash;

        TInvHashEntry entry;
        if (!reader.LowerBound(seekKey, &entry)) {
            return false;
        }

        while (entry.Hash != hash) {
            if (!reader.ReadHit(&entry)) {
                return false;
            }
            if ((entry.Hash >> 8) != (hash >> 8)) {
                return false;
            }
        }

        Y_ENSURE(entry.Hash == hash);
        *outputDocId = entry.DocId;
        return true;
    }

private:
    TInvHashBaseIo::TSampler::TModel Model_;
    THolder<TInvHashBaseIo::TSampler::TModel::TDecoderTable> DecoderTable_;
    TBlob SubBlob_;
    TBlob HitsBlob_;
};

class TInvHashWadIo {
public:
    using TReader = TInvHashWadReader;
    using TWriter = TInvHashWadWriter;
    using TSampler = TInvHashWadSampler;
    using TSearcher = TInvHashWadSearcher;
};

}
