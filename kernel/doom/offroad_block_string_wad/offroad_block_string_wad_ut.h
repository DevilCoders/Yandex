#pragma once

#include "offroad_block_string_wad_io.h"

#include <kernel/doom/hits/panther_hit.h>
#include <kernel/doom/offroad/panther_hit_adaptors.h>
#include <kernel/doom/wad/mega_wad_buffer_writer.h>
#include <kernel/doom/wad/mega_wad_merger.h>
#include <kernel/doom/wad/wad_index_type.h>

#include <library/cpp/offroad/offset/data_offset.h>
#include <library/cpp/offroad/test/test_md5.h>

#include <util/generic/algorithm.h>
#include <util/generic/buffer.h>
#include <util/generic/hash_set.h>
#include <util/random/fast.h>
#include <util/random/shuffle.h>

#include <utility>

using namespace NDoom;
using namespace NOffroad;

using TBlockStringIo = TOffroadBlockStringWadIo<
    PantherNgramsIndexType>;

template<class Io>
class TIndexGenerator {
public:
    using TWriter = typename Io::TWriter;
    using TReader = typename Io::TReader;
    using TSearcher = typename Io::TSearcher;
    using TModel = typename Io::TSampler::TModel;
    using TData = TVector<TString>;
    using TDataRefs = TVector<TStringBuf>;

public:
    TIndexGenerator(ui32 numHashes) {
        GenerateData(numHashes);

        auto model = CreateModel();

        TBuffer garbageBuffer;
        TMegaWadBufferWriter garbageWriter(&garbageBuffer);
        garbageWriter.RegisterIndexType(Io::IndexType);
        garbageWriter.RegisterDocLumpType(TWadLumpId(Io::IndexType, EWadLumpRole::HitsModel));
        garbageWriter.WriteDocLump(0, TWadLumpId(Io::IndexType, EWadLumpRole::HitsModel), TArrayRef<const char>());
        garbageWriter.Finish();
        THolder<IWad> garbageWad;
        garbageWad.Reset(IWad::Open(TArrayRef<const char>(~garbageBuffer, +garbageBuffer)));

        TBuffer blockBuffer;
        TMegaWadBufferWriter blockWriter(&blockBuffer);
        TMegaWadBufferWriter subWriter(&SubBuffer_);

        TWriter writer(&subWriter, model, &blockWriter);
        for (const auto& p : Data_) {
            ui32 hashId = 0;
            writer.WriteBlock(p, &hashId);
        }

        writer.Finish();
        blockWriter.Finish();
        subWriter.Finish();

        SubWad_.Reset(IWad::Open(TArrayRef<const char>(~SubBuffer_, +SubBuffer_)));
        THolder<IWad> blockWad;
        blockWad.Reset(IWad::Open(TArrayRef<const char>(~blockBuffer, +blockBuffer)));

        TBufferOutput out(GarbagedBlockBuffer_);
        TMegaWadMerger merger(&out);
        merger.Add(std::move(garbageWad));
        merger.Add(std::move(blockWad));
        merger.Finish();
        out.Finish();

        GarbagedBlockWad_.Reset(IWad::Open(TArrayRef<const char>(~GarbagedBlockBuffer_, +GarbagedBlockBuffer_)));
    }

    static inline TString GenerateString(TFastRng<ui32>& rand, ui32 numHashes) {
        const size_t len = 1 + rand.GenRand() % (numHashes / 10 + 1);
        TString current;
        current.reserve(len);
        for (size_t i = 0; i < len; ++i) {
            current.push_back('a' + rand.GenRand() % 26);
        }
        return current;
    }

    void GenerateData(ui32 numHashes) {

        TFastRng<ui32> rand(numHashes + 43242);
        THashSet<TString> keysSet;
        while (keysSet.size() < numHashes) {
            keysSet.insert(GenerateString(rand, numHashes));
        }

        TVector<TString> keys(keysSet.begin(), keysSet.end());
        Sort(keys);

        Y_VERIFY(numHashes == keys.size());

        std::swap(keys, Data_);
        DataRefs_.reserve(Data_.size());
        for (const auto& entry: Data_) {
            DataRefs_.push_back(TStringBuf(entry.Data(), entry.Size()));
        }

    }

    TModel CreateModel() {
        typename Io::TSampler sampler;
        for (const auto& p : DataRefs_) {
            sampler.WriteBlock(p);
        }
        return sampler.Finish();
    }

    TSearcher GetSearcher() const {
        return TSearcher(SubWad_.Get(), GarbagedBlockWad_.Get());
    }

    TDataRefs GetDataRefs() const {
        return DataRefs_;
    }

    TReader GetReader() const {
        return TReader(SubWad_.Get(), GarbagedBlockWad_.Get());
    }

private:
    TData Data_;
    TDataRefs DataRefs_;
    TBuffer SubBuffer_;
    TBuffer GarbagedBlockBuffer_;
    THolder<IWad> SubWad_;
    THolder<IWad> GarbagedBlockWad_;
};
