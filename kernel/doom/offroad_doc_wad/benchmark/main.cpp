#include <kernel/doom/offroad_doc_wad/offroad_doc_wad_ut.h>
#include <library/cpp/testing/benchmark/bench.h>
#include <util/generic/singleton.h>

struct TIndexHolder {
    TIndexHolder(size_t keysCount, size_t docsCount)
        : DocsCount(docsCount)
    {
        Index = GenerateRandomIndex(keysCount, docsCount);
        Buffer = WriteIndex(Index);
        Wad.Reset(IWad::Open(TArrayRef<const char>(Buffer.data(), Buffer.size())));

        for (size_t i = 0; i < Index.Keys.size(); ++i) {
            Keys.push_back(Index.Keys[i]);
        }
        Shuffle(Keys.begin(), Keys.end());
    }

    size_t DocsCount;
    TIndex Index;
    TBuffer Buffer;
    THolder<IWad> Wad;
    TVector<std::pair<TString, ui32>> Keys;
};

void ReadIndex(const TIndexHolder& index) {
    TSearcher searcher(index.Wad.Get());
    TSearcher::TIterator iterator;

    THashMap<ui32, size_t> indexByDocId;
    for (size_t i = 0; i < index.Index.DocumentWithHits.size(); ++i) {
        indexByDocId[index.Index.DocumentWithHits[i].first] = i;
    }

    TVector<ui32> docIds(index.DocsCount);
    Iota(docIds.begin(), docIds.end(), 0);

    for (const auto& key : index.Keys) {
        TVector<TOffroadWadKey> searcherKeys;
        searcher.FindTerms(key.first, &iterator, &searcherKeys);

        for (const auto& offroadKey : searcherKeys) {
            Shuffle(docIds.begin(), docIds.end());
            for (ui32 docId : docIds) {
                if (searcher.Find(docId, offroadKey.Id, &iterator)) {
                    TReqBundleHit hit;
                    while (iterator.ReadHit(&hit)) {
                        Y_DO_NOT_OPTIMIZE_AWAY(hit.DocId());
                    }
                }
            }
        }
    }
}

Y_CPU_BENCHMARK(BuildIndex, iface) {
    for (auto i : xrange(iface.Iterations())) {
        Y_UNUSED(i);
        TIndexHolder index(423, 4243);
        Y_DO_NOT_OPTIMIZE_AWAY(index);
    }
}

Y_CPU_BENCHMARK(SearchIndex, iface) {
    TIndexHolder* index = Singleton<TIndexHolder>(42, 4243);
    for (auto i : xrange(iface.Iterations())) {
        Y_UNUSED(i);
        ReadIndex(*index);
    }
}
