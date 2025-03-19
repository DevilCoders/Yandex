#pragma once

#include <library/cpp/offroad/custom/ui64_vectorizer.h>
#include <library/cpp/offroad/custom/subtractors.h>
#include <kernel/doom/hits/panther_hit.h>
#include <kernel/doom/offroad/panther_hit_adaptors.h>
#include <kernel/doom/offroad_block_wad/offroad_block_wad_writer.h>
#include <kernel/doom/offroad_doc_wad/offroad_doc_wad_writer.h>
#include <kernel/doom/offroad_block_wad/offroad_block_wad_sampler.h>
#include <kernel/doom/standard_models/standard_models.h>

namespace NDoom {

    struct TPartitionedPantherIo {
        using THash = ui64;
        using THashVectorizer = NOffroad::TUi64Vectorizer;
        using THashSubtractor = NOffroad::TD2Subtractor;
        using THit = NDoom::TPantherHit;
        using TVectorizer = NDoom::TPantherHitVectorizer;
        using TSubtractor = NDoom::TPantherHitSubtractor;
        static constexpr size_t BlockSize = 64;

        using TBlockWriter = NDoom::TOffroadBlockWadWriter<NDoom::EWadIndexType::PantherIndexType, THash, THashVectorizer, THashSubtractor, NDoom::BitDocCodec, BlockSize>;
        using THitWriter = NDoom::TOffroadDocWadWriter<NDoom::EWadIndexType::PantherIndexType, THit, TVectorizer, TSubtractor, NOffroad::TNullVectorizer, NDoom::BitDocCodec>;

        using TBlockSampler = NDoom::TOffroadBlockWadSampler<THash, THashVectorizer, THashSubtractor, BlockSize>;
        using THitSampler = NDoom::TOffroadDocWadSampler<THit, TVectorizer, TSubtractor>;

        static constexpr NDoom::EStandardIoModel DefaultBlockModel = NDoom::EStandardIoModel::InvertedIndexPantherHashModelV1;
        static constexpr NDoom::EStandardIoModel DefaultHitModel = NDoom::EStandardIoModel::InvertedIndexPantherHitModelV1;
    };

}
