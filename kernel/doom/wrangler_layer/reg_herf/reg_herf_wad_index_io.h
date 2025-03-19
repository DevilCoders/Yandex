#pragma once
#include <kernel/doom/offroad_reg_herf_wad/reg_herf_io.h>

namespace NDoom {

template <class TUnderlyingHit>
struct TStructDiffWadIndexHit {
    ui32 DocId = 0;
    ui32 Region = 0;
    const TUnderlyingHit* Hit = nullptr;
};

template <class ErfType, EWadIndexType indexType, EStandardIoModel defaultModel>
class TOffroadRegErfIndexWadWriter : public TRegErfGeneralIo<ErfType, indexType, defaultModel>::TWriter {
    using TBase = typename TRegErfGeneralIo<ErfType, indexType, defaultModel>::TWriter;
    using TBase::Write;
    using TKey = typename TBase::TKey;
public:
    using TModel = typename TBase::TModel;
    TOffroadRegErfIndexWadWriter(const TModel& model, const TString& path) : TBase(model, path) {

    }

    TOffroadRegErfIndexWadWriter(const TModel& model, TMegaWadWriter* writer) : TBase(model, writer) {

    }
    using TIndexHit = TStructDiffWadIndexHit<ErfType>;

    void WriteHit(const TIndexHit& hit) {
        Write(TKey{hit.DocId, hit.Region}, hit.Hit);
    }

    void WriteDoc(ui32) {

    }
};

template <class ErfType, EWadIndexType indexType, EStandardIoModel defaultModel>
class TOffroadRegErfIndexWadSampler: public TRegErfGeneralIo<ErfType, indexType, defaultModel>::TSampler {
    using TBase = typename TRegErfGeneralIo<ErfType, indexType, defaultModel>::TSampler;
    using TBase::Write;
    using TKey = typename TBase::TKey;
public:
    using TIndexHit = TStructDiffWadIndexHit<ErfType>;

    void WriteHit(const TIndexHit& hit) {
        Write(TKey{hit.DocId, hit.Region}, hit.Hit);
    }

    void WriteDoc(ui32) {

    }
};

template <class ErfType, EWadIndexType indexType, EStandardIoModel defaultModel>
struct TRegErfGeneralIndexIo : TRegErfGeneralIo<ErfType, indexType, defaultModel>
{
    using TSampler = TOffroadRegErfIndexWadSampler<ErfType, indexType, defaultModel>;
    using TWriter = TOffroadRegErfIndexWadWriter<ErfType, indexType, defaultModel>;
    using TData = typename TWriter::TData;
};

using TRegHostErfIndexIo = TRegErfGeneralIndexIo<TRegHostErfInfo, RegHostErfIndexType, DefaultRegHostErfIoModel>;
using TRegErfIndexIo = TRegErfGeneralIndexIo<TRegErfInfo, RegErfIndexType, NoStandardIoModel>;


} // namespace NDoom
