#pragma once

#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/offroad/custom/ui32_vectorizer.h>
#include <library/cpp/offroad/custom/ui64_vectorizer.h>
#include <library/cpp/offroad/custom/subtractors.h>
#include <library/cpp/offroad/test/test_md5.h>
#include <library/cpp/offroad/offset/data_offset.h>

#include <library/cpp/offroad/utility/masks.h>

#include <kernel/doom/hits/panther_hit.h>
#include <kernel/doom/offroad/panther_hit_adaptors.h>
#include <kernel/doom/offroad_hashed_keyinv_wad/offroad_hashed_keyinv_wad_io.h>
#include <kernel/doom/wad/wad_index_type.h>
#include <kernel/doom/wad/mega_wad_buffer_writer.h>

#include <util/generic/algorithm.h>
#include <util/generic/buffer.h>
#include <util/random/fast.h>
#include <util/random/shuffle.h>

#include <utility>

using namespace NDoom;
using namespace NOffroad;

using TUi64Io = TOffroadHashedKeyInvWadIo<
    PantherNgramsIndexType,
    ui64,
    TUi64Vectorizer,
    TD2Subtractor,
    TPantherHit,
    TPantherHitVectorizer,
    TPantherHitSubtractor,
    NoStandardIoModel,
    NoStandardIoModel>;

using TUi32Io = TOffroadHashedKeyInvWadIo<
    PantherNgramsIndexType,
    ui32,
    TUi32Vectorizer,
    TD1Subtractor,
    TPantherHit,
    TPantherHitVectorizer,
    TPantherHitSubtractor,
    NoStandardIoModel,
    NoStandardIoModel>;

template<class C, class TGetKey>
inline void SortAndUniqueBy(C& c, const TGetKey& getKey) {
    Sort(c);
    c.erase(UniqueBy(c.begin(), c.end(), getKey), c.end());
}

template <class TInt>
class TFakeRng {
public:
    TFakeRng() {};

    inline TInt GenRand() noexcept {
        return Counter_++;
    }

private:
    TInt Counter_ = 0;
};

template<class Io>
class TIndexGenerator {
public:
    using TWriter = typename Io::TWriter;
    using TSearcher = typename Io::TSearcher;
    using TIterator = typename Io::TIterator;
    using TInt = typename TWriter::THash;
    using TBlockModel = typename Io::TSampler::TBlockModel;
    using THitModel = typename Io::TSampler::THitModel;
    using TData = TVector<std::pair<TInt, TVector<TPantherHit>>>;

public:
    TIndexGenerator(ui32 numHashes, ui32 numDocs, ui32 numBits, bool useFakeRng) {
        TFastRng<TInt> rng(424243 + numHashes);
        TFakeRng<TInt> fakeRng;
        if (useFakeRng) {
            GenerateData(numHashes, numDocs, numBits, fakeRng);
        } else {
            GenerateData(numHashes, numDocs, numBits, rng);
        }

        auto models = CreateModels();

        TMegaWadBufferWriter blockWriter(&BlockBuffer_);
        TMegaWadBufferWriter docsWriter(&DocsBuffer_);
        TMegaWadBufferWriter subWriter(&SubBuffer_);

        TWriter writer(&subWriter, models.first, &blockWriter, models.second, &docsWriter);
        for (const auto& p : Data_) {
            for (TPantherHit hit : p.second) {
                writer.WriteHit(hit);
            }
            writer.WriteBlock(p.first);
        }
        writer.Finish();
        docsWriter.Finish();
        blockWriter.Finish();
        subWriter.Finish();

        SubWad_.Reset(IWad::Open(TArrayRef<const char>(SubBuffer_.data(), SubBuffer_.size())));
        BlockWad_.Reset(IWad::Open(TArrayRef<const char>(BlockBuffer_.data(), BlockBuffer_.size())));
        DocsWad_.Reset(IWad::Open(TArrayRef<const char>(DocsBuffer_.data(), DocsBuffer_.size())));
    }

    template <typename HashesGenerator>
    void GenerateData(ui32 numHashes, ui32 numDocs, ui32 numBits, HashesGenerator& rng) {
        TFastRng32 docIdRng(42 + numDocs, 0);
        Data_.clear();
        for (ui32 i = 0; i < numHashes; ++i) {
            TInt hash = rng.GenRand() & ScalarMask(numBits);
            Data_.emplace_back(hash, TVector<TPantherHit>());
            for (ui32 j = 0; j < numDocs; ++j) {
                ui32 docIdAndRelevance = docIdRng.GenRand();
                Data_.back().second.push_back(TPantherHit(docIdAndRelevance >> 6, docIdAndRelevance & 63));
            }
            SortAndUniqueBy(Data_.back().second, [](TPantherHit hit) {
                return hit.DocId();
            });
        }
        SortAndUniqueBy(Data_, [](const auto& p){
            return p.first;
        });
    }

    std::pair<TBlockModel, THitModel> CreateModels() {
        typename Io::TSampler sampler;
        for (const auto& p : Data_) {
            for (TPantherHit hit : p.second) {
                sampler.WriteHit(hit);
            }
            sampler.WriteBlock(p.first);
        }
        return sampler.Finish();
    }

    TSearcher GetSearcher() const {
        return TSearcher(SubWad_.Get(), BlockWad_.Get(), DocsWad_.Get());
    }

    const TData& GetData() const {
        return Data_;
    }

private:
    TData Data_;
    TBuffer SubBuffer_;
    TBuffer BlockBuffer_;
    TBuffer DocsBuffer_;
    THolder<IWad> SubWad_;
    THolder<IWad> BlockWad_;
    THolder<IWad> DocsWad_;
};
