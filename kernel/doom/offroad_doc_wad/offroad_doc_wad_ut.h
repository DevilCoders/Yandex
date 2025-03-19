#pragma once

#include <kernel/doom/hits/reqbundle_hit.h>
#include <kernel/doom/offroad_common/reqbundle_hit_adaptors.h>
#include <kernel/doom/offroad_doc_wad/offroad_doc_wad_reader.h>
#include <kernel/doom/offroad_doc_wad/offroad_doc_wad_sampler.h>
#include <kernel/doom/offroad_doc_wad/offroad_doc_wad_writer.h>
#include <kernel/doom/offroad_doc_wad/offroad_doc_wad_searcher.h>
#include <kernel/doom/offroad_key_wad/offroad_key_wad_sampler.h>
#include <kernel/doom/offroad_key_wad/offroad_key_wad_writer.h>
#include <kernel/doom/offroad_wad/offroad_wad_searcher.h>
#include <kernel/doom/wad/mega_wad_buffer_writer.h>

#include <util/random/shuffle.h>

#include <random>

using namespace NDoom;

using THitSampler = TOffroadDocWadSampler<TReqBundleHit, TReqBundleHitVectorizer, TReqBundleHitSubtractor>;
using THitWriter = TOffroadDocWadWriter<FactorAnnIndexType, TReqBundleHit, TReqBundleHitVectorizer, TReqBundleHitSubtractor, TReqBundleHitPrefixVectorizer>;
using TKeySampler = TOffroadKeyWadSampler<ui32, NOffroad::TUi32Vectorizer, NOffroad::TI1Subtractor>;
using TKeyWriter = TOffroadKeyWadWriter<FactorAnnIndexType, ui32, NOffroad::TUi32Vectorizer, NOffroad::TI1Subtractor, NOffroad::TNullSerializer>;

using TSearcher = TOffroadWadSearcher<FactorAnnIndexType, TReqBundleHit, TReqBundleHitVectorizer, TReqBundleHitSubtractor, TReqBundleHitPrefixVectorizer>;

using THitModel = THitWriter::TModel;
using TKeyModel = TKeyWriter::TModel;

struct TIndex {
    TVector<std::pair<TString, ui32>> Keys;
    TVector<std::pair<ui32, TVector<TReqBundleHit>>> DocumentWithHits;
};

std::pair<THitModel, TKeyModel> BuildModels(const TIndex& index) {
    THitSampler hitSampler;
    for (const auto& doc : index.DocumentWithHits) {
        for (const auto& hit : doc.second)
            hitSampler.WriteHit(hit);
        hitSampler.WriteDoc(doc.first);
    }
    THitModel hitModel = hitSampler.Finish();

    TKeySampler keySampler;
    for (const auto& key : index.Keys)
        keySampler.WriteKey(key.first, key.second);
    TKeyModel keyModel = keySampler.Finish();

    return { hitModel, keyModel };
}

TBuffer WriteIndex(const TIndex& index) {
    THitModel hitModel;
    TKeyModel keyModel;
    std::tie(hitModel, keyModel) = BuildModels(index);

    TBuffer buffer;
    TMegaWadBufferWriter writer(&buffer);

    TKeyWriter keyWriter(keyModel, &writer);
    for (const auto& key : index.Keys) {
        keyWriter.WriteKey(key.first, key.second);
    }
    keyWriter.Finish();

    THitWriter hitWriter(hitModel, &writer);
    for (const auto& doc : index.DocumentWithHits) {
        for (const auto& hit : doc.second) {
            hitWriter.WriteHit(hit);
        }
        hitWriter.WriteDoc(doc.first);
    }
    hitWriter.Finish();

    writer.Finish();
    return buffer;
}

TIndex GenerateSimpleIndex() {
    TIndex res;

    res.Keys.emplace_back("Key1", 1);
    res.Keys.emplace_back("Key2", 0);
    res.Keys.emplace_back("Lalala", 2);

    // ui32 docId, ui32 breuk, ui32 word, ui32 relevance, ui32 form
    res.DocumentWithHits.emplace_back(0, TVector<TReqBundleHit>({ TReqBundleHit(0, 1, 1, 0, 1, 0), TReqBundleHit(1, 2, 13, 1, 3, 0) }));
    res.DocumentWithHits.emplace_back(1, TVector<TReqBundleHit>({ TReqBundleHit(1, 18, 19, 2, 19, 0), TReqBundleHit(2, 13, 15, 3, 10, 0), TReqBundleHit(2, 14, 7, 1, 123, 0) }));

    return res;
}

TIndex GenerateRandomIndex(size_t keysCount, size_t docsCount) {
    TIndex res;

    std::minstd_rand random(4243);

    TVector<TString> keys;
    for (size_t i = 0; i < keysCount; ++i) {
        size_t len = 1 + random() % 5;
        keys.emplace_back();
        for (size_t j = 0; j < len; ++j) {
            keys.back().push_back('a' + random() % 3);
        }
    }
    Sort(keys);
    keysCount = Unique(keys.begin(), keys.end()) - keys.begin();
    keys.resize(keysCount);

    TVector<ui32> perm(keysCount);
    Iota(perm.begin(), perm.end(), 0);
    Shuffle(perm.begin(), perm.end());

    for (size_t i = 0; i < keysCount; ++i) {
        res.Keys.emplace_back(keys[i], perm[i]);
    }

    for (size_t i = 0; i < docsCount; ++i) {
        TVector<TReqBundleHit> hits;
        size_t hitsCount = random() % 100;
        for (size_t j = 0; j < hitsCount; ++j) {
            ui32 termId = random() % keysCount;
            ui32 breuk = 1 + random() % 10;
            ui32 word = 1 + random() % 10;
            ui32 relevance = random() % 4;
            ui32 form = random() % 100;
            ui32 range = 0;
            hits.emplace_back(termId, breuk, word, relevance, form, range);
        }
        Sort(hits);
        hits.resize(Unique(hits.begin(), hits.end()) - hits.begin());
        res.DocumentWithHits.emplace_back(i, hits);
    }

    return res;
}
