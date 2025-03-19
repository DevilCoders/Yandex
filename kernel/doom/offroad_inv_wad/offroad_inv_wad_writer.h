#pragma once

#include <library/cpp/offroad/tuple/tuple_writer.h>
#include <library/cpp/offroad/sub/sub_writer.h>

#include <kernel/doom/offroad_common/accumulating_output.h>
#include <kernel/doom/wad/wad.h>
#include <kernel/doom/wad/wad_writer.h>

namespace NDoom {


template <EWadIndexType indexType, class Data, class Vectorizer, class Subtractor, class PrefixVectorizer>
class TOffroadInvWadWriter {
private:
    using TWriter = std::conditional_t<
        PrefixVectorizer::TupleSize == 0,
        NOffroad::TTupleWriter<Data, Vectorizer, Subtractor, NOffroad::TEncoder64, 1, NOffroad::PlainOldBuffer>,
        NOffroad::TSubWriter<PrefixVectorizer, NOffroad::TTupleWriter<Data, Vectorizer, Subtractor, NOffroad::TEncoder64, 1, NOffroad::PlainOldBuffer>>
    >;

    static constexpr bool HasSub = (PrefixVectorizer::TupleSize != 0);

public:
    using THit = Data;
    using TTable = typename TWriter::TTable;
    using TModel = typename TWriter::TModel;
    using TPosition = typename TWriter::TPosition;

    enum {
        Stages = TWriter::Stages
    };

    TOffroadInvWadWriter() {}

    TOffroadInvWadWriter(const TModel& model, IWadWriter* writer) {
        Reset(model, writer);
    }

    TOffroadInvWadWriter(const TModel& model, const TTable* table, IWadWriter* writer) {
        Reset(model, table, writer);
    }

    TOffroadInvWadWriter(const IWad* modelWad, IWadWriter* writer) {
        Reset(modelWad, writer);
    }

    void Reset(const IWad* modelWad, IWadWriter* writer) {
        TModel model;
        model.Load(modelWad->LoadGlobalLump(TWadLumpId(indexType, EWadLumpRole::HitsModel)));

        Reset(model, writer);
    }

    void Reset(const TModel& model, IWadWriter* writer) {
        if (!LocalTable_) {
            LocalTable_ = MakeHolder<TTable>();
        }
        LocalTable_->Reset(model);

        Reset(model, LocalTable_.Get(), writer);
    }

    void Reset(const TModel& model, const TTable* table, IWadWriter* writer) {
        Table_ = table;
        WadWriter_ = writer;

        /* Write out model right away. */
        model.Save(WadWriter_->StartGlobalLump(TWadLumpId(indexType, EWadLumpRole::HitsModel)));

        /* Set up i/o, data will be written next. */
        SubOutput_.Reset();
        ResetInternal(std::integral_constant<bool, HasSub>());
    }

    void WriteHit(const THit& data) {
        Writer_.WriteHit(data);
    }

    void WriteSeekPoint() {
        Writer_.WriteSeekPoint();
    }

    TPosition Position() const {
        return Writer_.Position();
    }

    void Finish() {
        Writer_.Finish();
        if (HasSub) {
            SubOutput_.Flush(WadWriter_->StartGlobalLump(TWadLumpId(indexType, EWadLumpRole::HitSub)));
        }
    }

    bool IsFinished() const {
        return Writer_.IsFinished();
    }

private:
    void ResetInternal(std::false_type) {
        IOutputStream* output = WadWriter_->StartGlobalLump(TWadLumpId(indexType, EWadLumpRole::Hits));
        Writer_.Reset(Table_, output);
    }

    void ResetInternal(std::true_type) {
        IOutputStream* output = WadWriter_->StartGlobalLump(TWadLumpId(indexType, EWadLumpRole::Hits));
        Writer_.Reset(&SubOutput_, Table_, output);
    }

private:
    THolder<TTable> LocalTable_;
    const TTable* Table_ = nullptr;

    IWadWriter* WadWriter_ = nullptr;
    TAccumulatingOutput SubOutput_;
    TWriter Writer_;
};


} // namespace NDoom
