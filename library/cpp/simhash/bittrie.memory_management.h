#pragma once

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
typename TBitTrie<TKey, TValue, ChunkSize, TAllocator>::TMiddleNode*
TBitTrie<TKey, TValue, ChunkSize, TAllocator>::CreateMiddleNode() {
    TMiddleNode* res = middleNodeAllocator.allocate(1);
    new (res) TMiddleNode();
    return res;
}

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
void TBitTrie<TKey, TValue, ChunkSize, TAllocator>::DeleteMiddleNode(TMiddleNode* N) {
    Y_ASSERT((N != nullptr) && "N is NULL");
    if (N == &root) {
        root.Clear();
        return;
    }
    N->~TMiddleNode();
    middleNodeAllocator.deallocate(N, 1);
}

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
typename TBitTrie<TKey, TValue, ChunkSize, TAllocator>::TLeafNode*
TBitTrie<TKey, TValue, ChunkSize, TAllocator>::CreateLeafNode() {
    TLeafNode* res = leafNodeAllocator.allocate(1);
    new (res) TLeafNode();

    ++count;

    return res;
}

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
void TBitTrie<TKey, TValue, ChunkSize, TAllocator>::DeleteLeafNode(TLeafNode* N) {
    Y_ASSERT((N != nullptr) && "N is NULL");
    N->~TLeafNode();
    leafNodeAllocator.deallocate(N, 1);

    --count;
}
