#pragma once

#include "offroad_struct_diff_wad_io.h"

#include <kernel/doom/standard_models_storage/standard_models_storage.h>
#include <kernel/doom/wad/mega_wad.h>
#include <kernel/doom/wad/mega_wad_buffer_writer.h>
#include <kernel/doom/wad/mega_wad_reader.h>

#include <library/cpp/offroad/custom/subtractors.h>
#include <library/cpp/offroad/custom/ui32_vectorizer.h>

#include <util/generic/set.h>
#include <util/random/random.h>
#include <util/random/shuffle.h>

#include <type_traits>

using namespace NDoom;

inline ui64 Gen() {
    return RandomNumber<ui64>();
}
inline ui64 Gen(ui64 mx) {
    return RandomNumber<ui64>(mx);
}

template <typename StructData>
class TIndex {
    static_assert(std::is_trivially_copyable_v<StructData>);
public:
    using Io = TOffroadStructDiffWadIo<
        EWadIndexType::ArcIndexType,
        ui32,
        NOffroad::TUi32Vectorizer,
        NOffroad::TD1Subtractor,
        NOffroad::TUi32Vectorizer,
        StructData,
        EStandardIoModel::NoStandardIoModel>;
    using TWriter = typename Io::TWriter;
    using TReader = typename Io::TReader;
    using TSearcher = typename Io::TSearcher;
    using TSampler = typename Io::TSampler;
    using TData = TVector<std::pair<ui32, StructData>>;

public:
    TIndex(ui32 minDocId, ui32 maxDocId, ui32 docCount, bool use64BitSubWriter) {
        Build(minDocId, maxDocId, docCount, use64BitSubWriter);
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
    void Build(ui32 minDocId, ui32 maxDocId, ui32 docCount, bool use64BitSubWriter) {
        GenerateData(docCount, minDocId, maxDocId);
        typename TSampler::TModel model;
        {
            TSampler sampler;
            for (const auto& docHit : Data_) {
                sampler.Write(docHit.first, &docHit.second);
            }
            model = sampler.Finish();
        }
        TMegaWadBufferWriter megaWadWriter(&Buffer_);
        TWriter writer(model, &megaWadWriter, use64BitSubWriter);
        for (const auto& docHit : Data_) {
            writer.Write(docHit.first, &docHit.second);
        }
        writer.Finish();
        megaWadWriter.Finish();
        Wad_ = MakeHolder<TMegaWad>(TArrayRef<const char>(Buffer_.data(), Buffer_.size()));
    }

    void GenerateData(ui32 docCount, ui32 minDocId, ui32 maxDocId) {
        Y_ENSURE(maxDocId - minDocId + 1 >= docCount);
        Data_.reserve(docCount);

        TVector<ui32> docIdStash;
        if ((maxDocId - minDocId + 1) >> 1 >= docCount) {
            TSet<ui32> ids;
            for (ui32 i = 0; i < docCount; ++i) {
                ui32 doc;
                do {
                    doc = Gen(maxDocId - minDocId + 1) + minDocId;
                } while (ids.count(doc));
                docIdStash.push_back(doc);
                ids.insert(doc);
            }
        } else {
            docIdStash.reserve(maxDocId - minDocId + 1);
            for (ui32 i = minDocId; i <= maxDocId; ++i) {
                docIdStash.push_back(i);
            }
        }
        Sort(docIdStash.begin(), docIdStash.end());
        for (ui32 j = 0; j < docCount; j++) {
            ui32 doc = docIdStash[j];
            TString hit;
            hit.resize(sizeof(StructData));
            for (ui32 i = 0; i < sizeof(StructData); ++i) {
                hit[i] = Gen(256);
            }
            StructData data;
            memcpy(&data, hit.data(), sizeof(StructData));
            Data_.emplace_back(doc, data);
        }
        Sort(Data_.begin(), Data_.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.first < rhs.first;
        });
    }

private:
    TData Data_;
    TBuffer Buffer_;
    THolder<TMegaWad> Wad_;
};
