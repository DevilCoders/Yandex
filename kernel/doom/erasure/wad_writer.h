#pragma once

#include "part_location.h"

#include <kernel/doom/standard_models_storage/standard_models_storage.h>
#include <kernel/doom/wad/wad_writer.h>
#include <kernel/doom/wad/mega_wad_writer.h>
#include <kernel/doom/wad/doc_lump_writer.h>

#include <library/cpp/offroad/codec/decoder_64.h>
#include <library/cpp/offroad/codec/multi_table.h>
#include <library/cpp/offroad/custom/subtractors.h>
#include <library/cpp/offroad/custom/ui64_vectorizer.h>
#include <library/cpp/offroad/sub/sub_writer.h>
#include <library/cpp/offroad/tuple/tuple_sampler.h>
#include <library/cpp/offroad/tuple/tuple_writer.h>

#include <util/string/cast.h>
#include <util/stream/output.h>
#include <util/stream/file.h>
#include <util/stream/direct_io.h>
#include <util/generic/maybe.h>

namespace NDoom {

// IWadWriter-compatible adapter for simple use cases
// Remember that adapters are not truly independent: doc ids may only be advanced simultaneosly,
// finish on adapter finishes the whole writer.
template<typename ErasureWadWriter>
struct TErasureWadWriterAdapter: public IWadWriter {
    TErasureWadWriterAdapter(ErasureWadWriter* erasureWadWriter, size_t index)
        : ErasureWadWriter_(erasureWadWriter)
        , Index_(index)
    {
    }

    void RegisterDocLumpType(TWadLumpId id) final {
        return RegisterDocLumpType(ToString(id));
    }
    void RegisterDocLumpType(TStringBuf id) final {
        return ErasureWadWriter_->RegisterDocLumpType(Index_, id);
    }

    IOutputStream* StartGlobalLump(TWadLumpId id) final {
        return StartGlobalLump(ToString(id));
    }
    IOutputStream* StartGlobalLump(TStringBuf id) final {
        return ErasureWadWriter_->StartGlobalLump(Index_, id);
    }

    IOutputStream* StartDocLump(ui32 docId, TWadLumpId id) final {
        return StartDocLump(docId, ToString(id));
    }
    IOutputStream* StartDocLump(ui32 docId, TStringBuf id) final {
        return ErasureWadWriter_->StartDocLump(Index_, docId, id);
    }

    bool IsFinished() const final {
        return ErasureWadWriter_->IsFinished();
    }

    void Finish() final {
        ErasureWadWriter_->Finish();
    }

private:
    ErasureWadWriter* ErasureWadWriter_ = nullptr;
    size_t Index_ = 0;
};

using TOffsetsMappingSampler = NOffroad::TTupleSampler<ui64, NOffroad::TUi64Vectorizer, NOffroad::TD2Subtractor, NOffroad::TSampler64, NOffroad::EBufferType::PlainOldBuffer>;

class TErasureWadOffsetWriter {
public:
    TErasureWadOffsetWriter() = default;

    template <class... TArgs>
    TErasureWadOffsetWriter(TArgs... args) {
        Reset(std::forward<TArgs>(args)...);
    }

    void Reset(const ui32 dataPartCount, TMegaWadWriter* wadWriter) {
        WadWriter = wadWriter;
        WadWriter->RegisterDocLumpType({ EWadIndexType::ErasurePartLocationsOffsetOnly, EWadLumpRole::Hits });
        WadWriter->RegisterDocLumpType({ EWadIndexType::ErasurePartLocationsOffsetOnly, EWadLumpRole::HitSub });
        WadWriter->RegisterDocLumpType({ EWadIndexType::ErasurePartLocationsOffsetOnly, EWadLumpRole::HitsModel });

        OffsetsByPart.resize(dataPartCount);
    }

    void Finish(const ui32 indexCount) {
        TOffsetsMappingSampler::TModel model = TStandardIoModelsStorage::Model<TOffsetsMappingSampler::TModel>(EStandardIoModel::DefaultErasureLocationsModel);
        auto table = NOffroad::NewMultiTable(model);
        model.Save(WadWriter->StartGlobalLump({ EWadIndexType::ErasurePartLocationsOffsetOnly, EWadLumpRole::HitsModel }));

        ::Save(WadWriter->StartGlobalLump({ EWadIndexType::ErasurePartLocationsOffsetOnly, EWadLumpRole::HitSub }), indexCount);

        for (ui32 part = 0; part < OffsetsByPart.size(); ++part) {
            NOffroad::TSubWriter<NOffroad::TUi64Vectorizer, NOffroad::TTupleWriter<
                ui64,
                NOffroad::TUi64Vectorizer,
                NOffroad::TD2Subtractor,
                NOffroad::TEncoder64,
                1,
                NOffroad::EBufferType::PlainOldBuffer>> writer(
                WadWriter->StartDocLump(part, { EWadIndexType::ErasurePartLocationsOffsetOnly, EWadLumpRole::HitSub }),
                table.Get(),
                WadWriter->StartDocLump(part, { EWadIndexType::ErasurePartLocationsOffsetOnly, EWadLumpRole::Hits }));

            for (const ui64& offset : OffsetsByPart[part]) {
                writer.WriteHit(offset);
            }

            ::Save(WadWriter->StartDocLump(part, { EWadIndexType::ErasurePartLocationsOffsetOnly, EWadLumpRole::HitsModel }), OffsetsByPart[part].size());

            writer.Finish();
        }
    }

    void Write(const ui32 part, const ui64 offset) {
        Y_ENSURE(part < OffsetsByPart.size());
        OffsetsByPart[part].push_back(offset);
    }

private:
    TVector<TVector<ui64>> OffsetsByPart;
    TMegaWadWriter* WadWriter = nullptr;
};

template<typename TCodecType>
class TErasureWadWriter {
private:
    template<typename TWriter>
    struct TLocationWriterHolder {
        TAccumulatingOutput Output;
        TWriter Writer;
    };

    struct TErasureWadSingleIndexWriter {
        THolder<TMegaWadWriter> LocationGlobalsWriter;

        TLocationWriterHolder<NOffroad::TFlatWriter<std::nullptr_t, TErasureBlobLocation, NOffroad::TNullVectorizer, TErasureBlobLocationVectorizer>> LocationData;
        TMegaWadInfo Info;
        TDocLumpWriter DocLumpWriter;
        bool PartOptimizedMappings_ = false;

        void Reset() {
            DocLumpWriter.Reset(&Info);
            LocationData.Output.Reset();
            LocationData.Writer.Reset(&LocationData.Output);
            PartOptimizedMappings_ = false;
        }

        void Reset(TString globalOutputPath, bool partOptimizedMappings) {
            Reset();

            LocationGlobalsWriter.Reset(new TMegaWadWriter(globalOutputPath));
            PartOptimizedMappings_ = partOptimizedMappings;
        }

        void Reset(IOutputStream* globalOutput, bool partOptimizedMappings) {
            Reset();

            LocationGlobalsWriter.Reset(new TMegaWadWriter(globalOutput));
            PartOptimizedMappings_ = partOptimizedMappings;
        }

        void WriteCheckSumLump() {
            IOutputStream* crcDocLump = DocLumpWriter.ResetDocLump(CheckSumDocLumpId);
            TCalcCheckSumStream<TCrcExtendCalcer>::FlushZeroCheckSum(crcDocLump);

            TCalcCheckSumStream checkSumStream = DocLumpWriter.CheckSumStream();
            crcDocLump = DocLumpWriter.ResetDocLump(CheckSumDocLumpId);
            checkSumStream.Flush(crcDocLump);
        }

        void Finish(const size_t docCount) {
            Info.DocCount = docCount;

            if (!PartOptimizedMappings_) {
                LocationData.Writer.Finish();
                LocationData.Output.Flush(LocationGlobalsWriter->StartGlobalLump({ EWadIndexType::ErasurePartLocations, EWadLumpRole::Hits }));
            }
            SaveMegaWadInfo(LocationGlobalsWriter->StartGlobalLump({ EWadIndexType::ErasurePartLocations, EWadLumpRole::HitSub }), Info);
            LocationGlobalsWriter->Finish();
        }
    };

private:
    static constexpr size_t DefaultEncodeBlockSize = 4096;

public:
    TErasureWadWriter(size_t encodeBlockSize = DefaultEncodeBlockSize)
        : EncodeBlockSize_(encodeBlockSize)
        , TotalPartCount(Codec_.GetTotalPartCount())
        , ParityPartCount(Codec_.GetParityPartCount())
        , DataPartCount(Codec_.GetDataPartCount())
        , WordSize(Codec_.GetWordSize())
    {
        Y_ENSURE(encodeBlockSize % WordSize == 0);
    }

    TErasureWadWriter(TVector<TString> globalPaths, TConstArrayRef<TString> parts, bool partOptimizedMappings = true)
        : TErasureWadWriter()
    {
        Reset(globalPaths, parts, partOptimizedMappings);
    }

    void Reset() {
        Finished_ = false;
        CurDocId_ = 0;
        DirectIo_.clear();
        PartOutputs_.clear();
        Locations_.clear();
        DocLumpBuffers_.clear();
        IndexWriters_.clear();
        WriterAdapters_.clear();

        ResetCheckSumDocLump();
    }

    void Reset(TVector<TString> globalPaths, TConstArrayRef<TString> parts, bool partOptimizedMappings = true) {
        Reset();

        PartOptimizedMappings_ = partOptimizedMappings;

        IndexWriters_.resize(globalPaths.size());
        for (size_t i = 0; i < globalPaths.size(); ++i) {
            IndexWriters_[i].Reset(globalPaths[i], PartOptimizedMappings_);
        }

        Y_ENSURE(!IndexWriters_.empty());
        OffsetWriter_.Reset(DataPartCount, IndexWriters_[0].LocationGlobalsWriter.Get());

        InitializeAdapters();

        DocLumpBuffers_.resize(DataPartCount);
        DirectIo_.resize(TotalPartCount);
        PartOutputsHolders_.resize(TotalPartCount);
        PartOutputs_.resize(TotalPartCount);
        for (size_t i = 0; i < TotalPartCount; ++i) {
            DirectIo_[i] = MakeHolder<TDirectIOBufferedFile>(parts[i], WrOnly | Direct | Seq | CreateAlways);
            PartOutputsHolders_[i] = MakeHolder<TRandomAccessFileOutput>(*DirectIo_[i]);
            PartOutputs_[i] = PartOutputsHolders_[i].Get();
        }
        Locations_.assign(DataPartCount, 0);
    }

    void Reset(TVector<IOutputStream*> globalOutputs, TConstArrayRef<IOutputStream*> partOutputs, bool partOptimizedMappings = true) {
        Reset();

        PartOptimizedMappings_ = partOptimizedMappings;

        IndexWriters_.resize(globalOutputs.size());
        for (size_t i = 0; i < globalOutputs.size(); ++i) {
            IndexWriters_[i].Reset(globalOutputs[i], PartOptimizedMappings_);
        }

        Y_ENSURE(!IndexWriters_.empty());
        OffsetWriter_.Reset(DataPartCount, IndexWriters_[0].LocationGlobalsWriter.Get());

        InitializeAdapters();

        DocLumpBuffers_.resize(DataPartCount);
        PartOutputs_.assign(partOutputs.begin(), partOutputs.end());
        Y_ENSURE(PartOutputs_.size() == TotalPartCount);
        Locations_.assign(DataPartCount, 0);
    }

    void SetExternalPartMapping(TVector<ui32> dataPartIndexByDocId) {
        Y_ENSURE(!UseExternalPartMapping_);
        Y_ENSURE(!DataPartIndexByDocId_.Defined());
        UseExternalPartMapping_ = true;
        DataPartIndexByDocId_ = std::move(dataPartIndexByDocId);
    }

    void SetSaveDataPartIndexByDocId() {
        Y_ENSURE(!UseExternalPartMapping_);
        Y_ENSURE(!DataPartIndexByDocId_.Defined());
        DataPartIndexByDocId_.ConstructInPlace();
    }

    TVector<ui32> TakeDataPartIndexByDocId() {
        Y_ENSURE(!UseExternalPartMapping_);
        Y_ENSURE(DataPartIndexByDocId_.Defined());
        TVector<ui32> res = std::move(DataPartIndexByDocId_.GetRef());
        DataPartIndexByDocId_.Clear();
        return res;
    }

    void RegisterDocLumpType(size_t index, TWadLumpId id) {
        RegisterDocLumpType(index, ToString(id));
    }
    void RegisterDocLumpType(size_t index, TStringBuf id) {
        Y_ENSURE(!IsFinished());
        Y_ENSURE(index < IndexWriters_.size());
        IndexWriters_[index].DocLumpWriter.RegisterDocLumpType(id);
    }

    IOutputStream* StartDocLump(size_t index, ui32 docId, TWadLumpId id) {
        return StartDocLump(index, docId, ToString(id));
    }
    IOutputStream* StartDocLump(size_t index, ui32 docId, TStringBuf id) {
        Y_ENSURE(!IsFinished());
        Y_ENSURE(index < IndexWriters_.size());
        Y_ENSURE(CurDocId_ <= docId);

        if (!CheckSumDocLumpInited_) {
            InitCheckSumDocLump();
        }

        while (CurDocId_ != docId) {
            FinishDoc();
        }

        return IndexWriters_[index].DocLumpWriter.StartDocLump(id);
    }

    void WriteDocLump(size_t index, ui32 docId, TWadLumpId id, const TArrayRef<const char>& data) {
        IOutputStream* output = StartDocLump(index, docId, id);
        output->Write(data.data(), data.size());
    }

    IOutputStream* StartGlobalLump(size_t index, TWadLumpId id) {
        return StartGlobalLump(index, ToString(id));
    }
    IOutputStream* StartGlobalLump(size_t index, TStringBuf id) {
        Y_ENSURE(!IsFinished());
        Y_ENSURE(index < IndexWriters_.size());
        return IndexWriters_[index].LocationGlobalsWriter->StartGlobalLump(id);
    }

    void WriteGlobalLump(size_t index, TWadLumpId id, const TArrayRef<const char>& data) {
        WriteGlobalLump(index, ToString(id), data);
    }
    void WriteGlobalLump(size_t index, TStringBuf id, const TArrayRef<const char>& data) {
        IOutputStream* output = StartGlobalLump(index, id);
        output->Write(data.data(), data.size());
    }

    bool IsFinished() const {
        return Finished_;
    }

    void Finish() {
        if (IsFinished()) {
            return;
        }

        FinishDoc();

        // Write total size of docs in each part to calculate size of last doc as total_offset - last_doc_offset
        for (ui32 part = 0; part < DataPartCount; ++part) {
            OffsetWriter_.Write(part, Locations_[part]);
        }

        OffsetWriter_.Finish(IndexWriters_.size());

        for (TErasureWadSingleIndexWriter& indexWriter : IndexWriters_) {
            indexWriter.Finish(CurDocId_);
        }

        size_t maxsz = 0;
        for (size_t i = 0; i < DataPartCount; ++i) {
            DocLumpBuffers_[i].AlignUp(WordSize);
            maxsz = Max(maxsz, DocLumpBuffers_[i].Size());
        }
        for (size_t i = 0; i < DataPartCount; ++i) {
            while (DocLumpBuffers_[i].Size() < maxsz)
                DocLumpBuffers_[i].Append('\0');
        }

        FlushBuffers();
        for (size_t i = 0; i < TotalPartCount; ++i) {
            PartOutputs_[i]->Finish();
        }

        DocLumpBuffers_.clear();
        PartOutputs_.clear();
        DirectIo_.clear();
        Finished_ = true;
    }

    TErasureWadWriterAdapter<TErasureWadWriter<TCodecType>>* GetWriterAdapter(size_t index) {
        return &WriterAdapters_[index];
    }

private:
    void FinishDoc() {
        ui32 dataPartIndex = Max<ui32>();
        if (!UseExternalPartMapping_) {
            std::pair<size_t, ui32> bestBuffer = { Max<size_t>(), 0 };
            for (ui32 i = 0; i < DataPartCount; ++i) {
                bestBuffer = Min(bestBuffer, {DocLumpBuffers_[i].Size(), i});
            }
            dataPartIndex = bestBuffer.second;

            if (DataPartIndexByDocId_.Defined()) {
                Y_ENSURE(DataPartIndexByDocId_->size() == CurDocId_);
                DataPartIndexByDocId_->push_back(dataPartIndex);
            }
        } else {
            Y_ENSURE(DataPartIndexByDocId_.Defined());
            Y_ENSURE(CurDocId_ < DataPartIndexByDocId_->size(), "Doc id " << CurDocId_ << " out of mappings size " << DataPartIndexByDocId_->size());
            dataPartIndex = DataPartIndexByDocId_.GetRef()[CurDocId_];
        }

        FillCheckSumDocLump();

        TBufferOutput out(DocLumpBuffers_[dataPartIndex]);

        for (TErasureWadSingleIndexWriter& indexWriter : IndexWriters_) {
            const ui32 docSize = indexWriter.DocLumpWriter.FinishDoc(&out);

            if (PartOptimizedMappings_) {
                OffsetWriter_.Write(dataPartIndex, Locations_[dataPartIndex]);
            } else {
                indexWriter.LocationData.Writer.Write(nullptr, TErasureBlobLocation{dataPartIndex, Locations_[dataPartIndex], docSize});
            }

            Locations_[dataPartIndex] += docSize;
        }

        if (ShouldFlush()) {
            FlushBuffers();
        }

        ++CurDocId_;
    }

    void FlushBuffers() {
        TVector<TBlob> inputs;
        size_t encodeBlock = Max<size_t>();
        for (size_t i = 0; i < DataPartCount; ++i) {
            encodeBlock = Min(::AlignDown(DocLumpBuffers_[i].Size(), WordSize), encodeBlock);
        }

        for (size_t i = 0; i < DataPartCount; ++i) {
            Y_ASSERT(DocLumpBuffers_[i].Size() >= encodeBlock);
            inputs.push_back(TBlob::NoCopy(DocLumpBuffers_[i].Data(), encodeBlock));
            PartOutputs_[i]->Write(DocLumpBuffers_[i].Data(), encodeBlock);
        }

        std::vector<TBlob> parities = Codec_.Encode(inputs);
        for (size_t i = 0; i < ParityPartCount; ++i) {
            Y_ASSERT(parities[i].Size() == encodeBlock);
            PartOutputs_[DataPartCount + i]->Write(parities[i].Data(), encodeBlock);
        }

        for (size_t i = 0; i < DataPartCount; ++i) {
            DocLumpBuffers_[i].Chop(0, encodeBlock);
            if (Max(EncodeBlockSize_, DocLumpBuffers_[i].Size()) + EncodeBlockSize_ < DocLumpBuffers_[i].Capacity()) {
                DocLumpBuffers_[i].ShrinkToFit();
            }
        }
    }

    bool ShouldFlush() {
        for (auto& buf: DocLumpBuffers_) {
            if (buf.Size() < EncodeBlockSize_) {
                return false;
            }
        }
        return true;
    }

    void InitCheckSumDocLump() {
        CheckSumDocLumpInited_ = true;
        for (size_t index = 0; index < IndexWriters_.size(); ++index) {
            RegisterDocLumpType(index, CheckSumDocLumpId);
        }
    }

    void FillCheckSumDocLump() {
        // Calculate checkSum with CheckSum DocLump == 0
        if (CheckSumDocLumpInited_) {
            for (TErasureWadSingleIndexWriter& indexWriter : IndexWriters_) {
                indexWriter.WriteCheckSumLump();
            }
        }
    }

    void ResetCheckSumDocLump() {
        CheckSumDocLumpInited_ = false;
    }

    void InitializeAdapters() {
        Y_ENSURE(WriterAdapters_.empty());
        for (size_t i = 0; i < IndexWriters_.size(); ++i) {
            WriterAdapters_.emplace_back(this, i);
        }
    }

private:
    ui32 CurDocId_ = 0;
    size_t EncodeBlockSize_ = 0;

    TCodecType Codec_;

    const size_t TotalPartCount;
    const size_t ParityPartCount;
    const size_t DataPartCount;
    const size_t WordSize;

    TVector<TErasureWadWriterAdapter<TErasureWadWriter<TCodecType>>> WriterAdapters_;
    TVector<TErasureWadSingleIndexWriter> IndexWriters_;
    TErasureWadOffsetWriter OffsetWriter_;

    // Common shared info:
    TVector<size_t> Locations_;
    TVector<TBuffer> DocLumpBuffers_;

    TVector<THolder<TDirectIOBufferedFile>> DirectIo_;
    TVector<THolder<IOutputStream>> PartOutputsHolders_;
    TVector<IOutputStream*> PartOutputs_;

    bool UseExternalPartMapping_ = false;
    TMaybe<TVector<ui32>> DataPartIndexByDocId_;

    bool CheckSumDocLumpInited_ = false;

    bool Finished_ = false;

    bool PartOptimizedMappings_ = false;
};

template<class Container>
TVector<IOutputStream*> MakeOutputStreamsPtrVector(Container& container) {
    TVector<IOutputStream*> outputStreams(Reserve(container.size()));
    for (IOutputStream& stream : container) {
        outputStreams.push_back(&stream);
    }
    return outputStreams;
}

}
