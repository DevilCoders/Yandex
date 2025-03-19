#pragma once

#include "offroad_struct_wad_io.h"
#include "serializers.h"

#include <kernel/doom/standard_models_storage/standard_models_storage.h>
#include <kernel/doom/wad/mega_wad.h>
#include <kernel/doom/wad/mega_wad_buffer_writer.h>
#include <kernel/doom/wad/mega_wad_reader.h>

#include <util/generic/set.h>
#include <util/random/random.h>
#include <util/random/shuffle.h>

using namespace NDoom;

ui64 gen() {
    return RandomNumber<ui64>();
}

ui64 gen(ui64 mx) {
    return RandomNumber<ui64>(mx);
}

template <EStructType structType, ECompressionType compressionType>
class TIndex {
public:
    using Io = TOffroadStructWadIo<OmniUrlType, TStringBuf, TStringBufSerializer, structType, compressionType>;
    using TWriter = typename Io::TWriter;
    using TReader = typename Io::TReader;
    using TSearcher = typename Io::TSearcher;
    using TSampler = typename Io::TSampler;
    using TData = TVector<std::pair<ui32, TString>>;

public:
    TIndex(ui32 sz, ui32 minDocId = 0, ui32 maxDocId = 0, ui32 docCount = 0) {
        if (docCount == 0)
            docCount = sz;
        Build(sz, minDocId, maxDocId, docCount);
    }

    const TMegaWad* GetWad() const {
        return Wad_.Get();
    }

    TData GetData() const {
        return Data_;
    }

    THolder<TReader> GetReader() const {
        return MakeHolder<TReader>(Wad_.Get());
    }

    THolder<TSearcher> GetSearcher() const {
        return MakeHolder<TSearcher>(nullptr, Wad_.Get());
    }

private:
    void Build(ui32 sz, ui32 minDocId, ui32 maxDocId, ui32 docCount) {
        GenerateData(docCount, minDocId, maxDocId ? maxDocId : docCount * 20, sz);
        typename TSampler::TModel model;
        {
            TSampler sampler;
            for (const auto& docHit : Data_) {
                sampler.WriteHit(TStringBuf(docHit.second));
                sampler.WriteDoc(docHit.first);
            }
            model = sampler.Finish();
        }
        TMegaWadBufferWriter megaWadWriter(&Buffer_);
        TWriter writer(model, &megaWadWriter);
        for (const auto& docHit : Data_) {
            writer.WriteHit(TStringBuf(docHit.second));
            writer.WriteDoc(docHit.first);
        }
        writer.Finish();
        megaWadWriter.Finish();
        Wad_ = MakeHolder<TMegaWad>(TArrayRef<const char>(Buffer_.data(), Buffer_.size()));
    }

    void GenerateData(ui32 docCount, ui32 minDocId, ui32 maxDocId, ui32 maxHitSize) {
        Y_ENSURE(maxDocId - minDocId + 1 >= docCount);
        Data_.reserve(docCount);

        ui32 len;
        TVector <ui32> docIdStash;
        if ((maxDocId - minDocId + 1) >> 1 >= docCount) {
            TSet<ui32> ids;
            for (ui32 i = 0; i < docCount; ++i) {
                ui32 doc;
                do {
                    doc = gen(maxDocId - minDocId + 1) + minDocId;
                } while (ids.count(doc));
                docIdStash.push_back(doc);
                ids.insert(doc);
            }
        } else {
            docIdStash.reserve(maxDocId - minDocId + 1);
            for (ui32 i = minDocId; i <= maxDocId; ++i) {
                docIdStash.push_back(i);
            }
            Shuffle(docIdStash.begin(), docIdStash.end());
        }

        for (ui32 j = 0; j < docCount; j++) {
            ui32 doc = docIdStash[j];
            if (structType != FixedSizeStructType) {
                len = gen(maxHitSize) + 1;
            } else {
                len = maxHitSize;
            }
            TString hit;
            hit.resize(len);
            for (ui32 i = 0; i < len; ++i) {
                if (structType == AutoEofStructType) {
                    hit[i] = gen(255) + 1;
                } else {
                    hit[i] = gen(256);
                }
            }
            Data_.push_back({doc, hit});
        }
        Sort(Data_.begin(), Data_.end());
    }

private:
    TData Data_;
    TBuffer Buffer_;
    THolder<TMegaWad> Wad_;
};
