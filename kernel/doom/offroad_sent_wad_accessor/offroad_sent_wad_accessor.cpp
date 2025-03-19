#include <library/cpp/offroad/custom/null_vectorizer.h>

#include <kernel/doom/offroad_doc_wad/offroad_doc_wad_searcher.h>
#include <kernel/doom/offroad_sent_wad/sent_hit_adaptors.h>

#include "offroad_sent_wad_accessor.h"

namespace NDoom {

template <EWadIndexType indexType>
class TOffroadSentenceLengthsReader: public ISentenceLengthsReader {
    using TSearcher = TOffroadDocWadSearcher<indexType, TSentHit, TSentHitVectorizer, TSentHitSubtractor, NOffroad::TNullVectorizer, BitDocCodec>;
    using TIterator = typename TSearcher::TIterator;
public:
    TOffroadSentenceLengthsReader(const TString& path, bool lockMemory) {
        Wad_ = IWad::Open(path, lockMemory);
        Searcher_.Reset(Wad_.Get());
    }

    ~TOffroadSentenceLengthsReader() override {
    }

    void GetOffsets(ui32 docId, TSentenceOffsets* result, ISentenceLengthsPreLoader* /* preLoader */) const override {
        result->clear();

        TIterator iterator;
        if (!Searcher_.Find(docId, &iterator))
            return;

        TSentHit hit(docId, 0);
        while(iterator.ReadHit(&hit))
            result->push_back(hit.Offset());
    }

    THolder<ISentenceLengthsPreLoader> CreatePreLoader() const override {
        return nullptr;
    }

private:
    THolder<IWad> Wad_;
    TSearcher Searcher_;
};

THolder<ISentenceLengthsReader> NewOffroadFastSentWadIndex(const TString& path, bool lockMemory) {
    return MakeHolder<TOffroadSentenceLengthsReader<FastSentIndexType>>(path, lockMemory);
}

THolder<ISentenceLengthsReader> NewOffroadFactorSentWadIndex(const TString& path, bool lockMemory) {
    return MakeHolder<TOffroadSentenceLengthsReader<FactorSentIndexType>>(path, lockMemory);
}

} // NDoom
