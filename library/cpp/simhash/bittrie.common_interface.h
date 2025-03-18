#pragma once

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
TBitTrieIterator<const TBitTrie<TKey, TValue, ChunkSize, TAllocator>, LEVEL_COUNT, NODE_POINTER_COUNT> TBitTrie<TKey, TValue, ChunkSize, TAllocator>::CMin() const {
    TConstIt it(*this);
    it.MoveToMin();
    return it;
}

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
TBitTrieIterator<const TBitTrie<TKey, TValue, ChunkSize, TAllocator>, LEVEL_COUNT, NODE_POINTER_COUNT> TBitTrie<TKey, TValue, ChunkSize, TAllocator>::CMax() const {
    TConstIt it(*this);
    it.MoveToMax();
    return it;
}

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
TBitTrieIterator<TBitTrie<TKey, TValue, ChunkSize, TAllocator>, LEVEL_COUNT, NODE_POINTER_COUNT> TBitTrie<TKey, TValue, ChunkSize, TAllocator>::Min() {
    TIt it(*this);
    it.MoveToMin();
    return it;
}

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
TBitTrieIterator<TBitTrie<TKey, TValue, ChunkSize, TAllocator>, LEVEL_COUNT, NODE_POINTER_COUNT> TBitTrie<TKey, TValue, ChunkSize, TAllocator>::Max() {
    TIt it(*this);
    it.MoveToMax();
    return it;
}

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
bool TBitTrie<TKey, TValue, ChunkSize, TAllocator>::Insert(
    TKey Key,
    const TValue& Value,
    TValue** OldValue) {
    ui8 levelKeys[LEVEL_COUNT];
    SplitBitKey(Key, levelKeys);

    std::pair<TLeafNode*, bool> res = MakePath(levelKeys);
    Y_ASSERT((res.first != nullptr) && "Your program is wrong");
    if (res.second) {
        res.first->Key = Key;
        res.first->Value = Value;
    }
    if (OldValue != nullptr) {
        *OldValue = &res.first->Value;
    }

    return res.second;
}

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
bool TBitTrie<TKey, TValue, ChunkSize, TAllocator>::Has(TKey Key) const {
    ui8 levelKeys[LEVEL_COUNT];
    SplitBitKey(Key, levelKeys);

    return GetPath(levelKeys) != nullptr;
}

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
const TValue& TBitTrie<TKey, TValue, ChunkSize, TAllocator>::Get(TKey Key) const {
    ui8 levelKeys[LEVEL_COUNT];
    SplitBitKey(Key, levelKeys);

    TLeafNode* node = GetPath(levelKeys);
    if (node != NULL) {
        return node->Value;
    }
    ythrow yexception() << "No such key";
}

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
TValue& TBitTrie<TKey, TValue, ChunkSize, TAllocator>::Get(TKey Key, bool Create) {
    ui8 levelKeys[LEVEL_COUNT];
    SplitBitKey(Key, levelKeys);

    TLeafNode* node = GetPath(levelKeys);
    if (node != nullptr) {
        return node->Value;
    }
    if (Create) {
        std::pair<TLeafNode*, bool> res = MakePath(levelKeys);
        Y_ASSERT((res.first != nullptr) && "Your program is wrong");
        Y_ASSERT((res.second) && "Your program is wrong");
        res.first->Key = Key;
        res.first->Value = TValue();

        return res.first->Value;
    }
    ythrow yexception() << "No such key";
}

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
bool TBitTrie<TKey, TValue, ChunkSize, TAllocator>::TryGet(TKey Key, TValue** Value) const {
    Y_ASSERT((Value != nullptr) && "Value is NULL");

    ui8 levelKeys[LEVEL_COUNT];
    SplitBitKey(Key, levelKeys);

    TLeafNode* node = GetPath(levelKeys);
    if (node != nullptr) {
        *Value = &node->Value;
        return true;
    }
    return false;
}

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
bool TBitTrie<TKey, TValue, ChunkSize, TAllocator>::Delete(TKey Key, TValue* Value) {
    ui8 levelKeys[LEVEL_COUNT];
    SplitBitKey(Key, levelKeys);
    void* nodes[LEVEL_COUNT + 1];
    ui32 last = GetNodePath(levelKeys, nodes);
    if (last < GetLevelCount()) {
        return false;
    }
    TLeafNode* node = reinterpret_cast<TLeafNode*>(nodes[last]);
    Y_ASSERT((node->Key == Key) && "Your program is wrong");
    if (Value != nullptr) {
        *Value = node->Value;
    }
    bool res = DeletePath(levelKeys, nodes);
    Y_ASSERT(res && "Your program is wrong");
    return res;
}

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
void TBitTrie<TKey, TValue, ChunkSize, TAllocator>::Clear() {
    BFS(TDestructor::NodeProcessor);
    count = 0;
}
