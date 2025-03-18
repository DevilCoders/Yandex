#pragma once

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
void TBitTrie<TKey, TValue, ChunkSize, TAllocator>::SplitBitKey(
    TKey Key,
    ui8 LevelKeys[LEVEL_COUNT]) const {
    for (ui32 i = 0; i < GetLevelCount(); ++i) {
        LevelKeys[i] = Key & ((1 << GetChunkSize()) - 1);
        Key >>= GetChunkSize();
        Y_ASSERT((LevelKeys[i] + 1L < NODE_POINTER_COUNT + 1L) && "Your program is wrong");
    }
}

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
ui32 TBitTrie<TKey, TValue, ChunkSize, TAllocator>::GetNodePath(
    ui8 LevelKeys[LEVEL_COUNT],
    void* Nodes[LEVEL_COUNT + 1]) const {
    memset(Nodes, 0, sizeof(void*) * (GetLevelCount() + 1));

    Nodes[0] = const_cast<TMiddleNode*>(&root);
    TMiddleNode* current = const_cast<TMiddleNode*>(&root);
    for (ui32 i = 0; i < GetLevelCount(); ++i) {
        ui8 key = LevelKeys[i];
        void* temp = current->GetNext(key);
        if (temp == nullptr) {
            Y_ASSERT((Nodes[i] != nullptr) && "Your program is wrong");
            return i;
        }

        Nodes[i + 1] = temp;

        if ((i + 1) < GetLevelCount()) {
            TMiddleNode* node = reinterpret_cast<TMiddleNode*>(temp);
            current = node;
        }
    }
    Y_ASSERT((Nodes[GetLevelCount()] != nullptr) && "Your program is wrong");
    return GetLevelCount();
}

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
void TBitTrie<TKey, TValue, ChunkSize, TAllocator>::GetMinNodePath(
    ui8 LevelKeys[LEVEL_COUNT],
    void* Nodes[LEVEL_COUNT + 1]) const {
    Nodes[0] = const_cast<TMiddleNode*>(&root);
    TMiddleNode* current = const_cast<TMiddleNode*>(&root);
    for (ui32 i = 0; i < GetLevelCount(); ++i) {
        Nodes[i + 1] = nullptr;
        for (ui32 j = 0; j < GetNodePointerCount(); ++j) {
            void* temp = current->GetNext(j);
            if (temp != nullptr) {
                LevelKeys[i] = j;
                Nodes[i + 1] = temp;
                current = reinterpret_cast<TMiddleNode*>(temp);
                break;
            }
        }
        if (Nodes[i + 1] == nullptr) {
            if (i == 0) {
                ythrow yexception() << "Trie is empty";
            } else {
                ythrow yexception() << "Corrupted internal state";
            }
        }
    }
}

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
void TBitTrie<TKey, TValue, ChunkSize, TAllocator>::GetMaxNodePath(
    ui8 LevelKeys[LEVEL_COUNT],
    void* Nodes[LEVEL_COUNT + 1]) const {
    Nodes[0] = const_cast<TMiddleNode*>(&root);
    TMiddleNode* current = const_cast<TMiddleNode*>(&root);
    for (ui32 i = 0; i < GetLevelCount(); ++i) {
        Nodes[i + 1] = nullptr;
        for (ui32 j = GetNodePointerCount() - 1;; --j) {
            void* temp = current->GetNext(j);
            if (temp != nullptr) {
                LevelKeys[i] = j;
                Nodes[i + 1] = temp;
                current = reinterpret_cast<TMiddleNode*>(temp);
                break;
            }
            if (j == 0) {
                break;
            }
        }
        if (Nodes[i + 1] == nullptr) {
            if (i == 0) {
                ythrow yexception() << "Trie is empty";
            } else {
                ythrow yexception() << "Corrupted internal state";
            }
        }
    }
}

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
std::pair<typename TBitTrie<TKey, TValue, ChunkSize, TAllocator>::TLeafNode*, bool>
TBitTrie<TKey, TValue, ChunkSize, TAllocator>::MakePath(ui8 LevelKeys[LEVEL_COUNT]) {
    void* nodes[LEVEL_COUNT + 1];
    ui32 last = GetNodePath(LevelKeys, nodes);
    if (last == GetLevelCount()) {
        return std::pair<TLeafNode*, bool>(reinterpret_cast<TLeafNode*>(nodes[last]), false);
    }

    for (ui32 i = last; i < GetLevelCount(); ++i) {
        TMiddleNode* current = reinterpret_cast<TMiddleNode*>(nodes[i]);
        Y_ASSERT((current != nullptr) && "Your program is wrong");
        void* node = nullptr;
        if ((i + 1) == GetLevelCount()) {
            node = CreateLeafNode();
        } else {
            node = CreateMiddleNode();
        }
        nodes[i + 1] = node;
        current->SetNext(LevelKeys[i], node);
    }

    Y_ASSERT((nodes[GetLevelCount()] != nullptr) && "Your program is wrong");
    return std::pair<TLeafNode*, bool>(reinterpret_cast<TLeafNode*>(nodes[GetLevelCount()]), true);
}

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
typename TBitTrie<TKey, TValue, ChunkSize, TAllocator>::TLeafNode*
TBitTrie<TKey, TValue, ChunkSize, TAllocator>::GetPath(ui8 LevelKeys[LEVEL_COUNT]) const {
    void* nodes[LEVEL_COUNT + 1];
    GetNodePath(LevelKeys, nodes);

    return reinterpret_cast<TLeafNode*>(nodes[GetLevelCount()]);
}

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
bool TBitTrie<TKey, TValue, ChunkSize, TAllocator>::DeletePath(
    ui8 LevelKeys[LEVEL_COUNT],
    void* Nodes[LEVEL_COUNT + 1]) {
    if (Nodes[GetLevelCount()] == nullptr) {
        return false;
    }
    for (ui32 i = GetLevelCount(); i >= 1; --i) {
        TMiddleNode* parent = reinterpret_cast<TMiddleNode*>(Nodes[i - 1]);
        parent->SetNext(LevelKeys[i - 1], nullptr);
        if (i == GetLevelCount()) {
            TLeafNode* node = reinterpret_cast<TLeafNode*>(Nodes[i]);
            DeleteLeafNode(node);
        } else {
            TMiddleNode* node = reinterpret_cast<TMiddleNode*>(Nodes[i]);
            DeleteMiddleNode(node);
        }
        if (parent->GetCount() > 0) {
            break;
        }
    }
    return true;
}
