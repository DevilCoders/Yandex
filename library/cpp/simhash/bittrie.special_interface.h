#pragma once

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
template <class TRes>
void TBitTrie<TKey, TValue, ChunkSize, TAllocator>::GetNeighbors(
    TKey Key,
    ui32 Distance,
    TVector<TRes>* Keys) const {
    Y_ASSERT((Keys != nullptr) && "Keys is NULL");

    Keys->clear();

    typename TNeighbors::template TBFSContext<TRes> bfsContext;
    bfsContext.Distance = Distance;
    bfsContext.Keys = Keys;
    SplitBitKey(Key, bfsContext.LevelKeys);

    typename TNeighbors::TNodeContext rootContext(0);

    BFS(TNeighbors::template NodeProcessor<TRes>, rootContext, &bfsContext);
}

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
size_t TBitTrie<TKey, TValue, ChunkSize, TAllocator>::GetNeighborCount(
    TKey Key,
    ui32 Distance) const {
    typename TNeighborCount::TBFSContext bfsContext;
    bfsContext.Distance = Distance;
    SplitBitKey(Key, bfsContext.LevelKeys);

    typename TNeighborCount::TNodeContext rootContext(0);

    BFS(TNeighborCount::NodeProcessor, rootContext, &bfsContext);

    return bfsContext.Count;
}

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
void TBitTrie<TKey, TValue, ChunkSize, TAllocator>::CountNeighbors(
    THashMap<TKey, std::pair<TValue*, size_t>>& data,
    ui32 distance) const {
    data.clear();
    data.reserve((size_t)GetCount());

    TConstIt it = CMin();

    TVector<std::pair<TKey, TValue*>> group;
    typename TCountNeighbors::TBFSContext bfsContext(group);
    bfsContext.Distance = distance;

    while (!it.IsEndReached()) {
        TKey currentKey = it.GetCurrentKey();
        TValue* currentValue = &it.GetCurrentValue();

        group.clear();
        SplitBitKey(currentKey, bfsContext.LevelKeys);

        typename TCountNeighbors::TNodeContext rootContext(0, 0);
        BFS(TCountNeighbors::NodeProcessor, rootContext, &bfsContext);

        typename THashMap<TKey, std::pair<TValue*, size_t>>::iterator vertex =
            data.find(currentKey);
        if (vertex != data.end()) {
            vertex->second.second += group.size();
        } else {
            data[currentKey] = std::make_pair(currentValue, group.size());
        }
        const size_t groupSize = group.size();
        for (size_t i = 0; i < groupSize; ++i) {
            TKey key = group[i].first;
            TValue* value = group[i].second;

            if (key == currentKey) {
                continue;
            }

            vertex = data.find(key);
            if (vertex != data.end()) {
                vertex->second.second += 1;
            } else {
                data[key] = std::make_pair(value, 1);
            }
        }

        it.Inc();
    }
}
