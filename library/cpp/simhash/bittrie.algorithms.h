#pragma once

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
template <class TRes>
void TBitTrie<TKey, TValue, ChunkSize, TAllocator>::TNeighbors::NodeProcessor(
    ui32 LevelIndex,
    const std::pair<const TMiddleNode*, TNodeContext>& Node,
    TVector<std::pair<const TMiddleNode*, TNodeContext>>* NextLevel,
    TBFSContext<TRes>* BFSContext) {
    for (ui32 i = 0; i < GetNodePointerCount(); ++i) {
        void* voidPtr = Node.first->GetNext(i);
        if (voidPtr == nullptr) {
            continue;
        }
        const ui8 distance = Node.second.Distance + HammingDistance<ui8>(BFSContext->LevelKeys[LevelIndex], static_cast<ui8>(i));
        if (distance > BFSContext->Distance) {
            continue;
        }
        if (LevelIndex + 1 < GetLevelCount()) {
            NextLevel->push_back(std::pair<const TMiddleNode*, TNodeContext>(reinterpret_cast<const TMiddleNode*>(voidPtr), TNodeContext(distance)));
        } else {
            TLeafNode* node = reinterpret_cast<TLeafNode*>(voidPtr);

            BFSContext->Keys->push_back(node->operator TRes());
        }
    }
}

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
void TBitTrie<TKey, TValue, ChunkSize, TAllocator>::TNeighborCount::NodeProcessor(
    ui32 LevelIndex,
    const std::pair<const TMiddleNode*, TNodeContext>& Node,
    TVector<std::pair<const TMiddleNode*, TNodeContext>>* NextLevel,
    TBFSContext* BFSContext) {
    for (ui32 i = 0; i < GetNodePointerCount(); ++i) {
        void* voidPtr = Node.first->GetNext(i);
        if (voidPtr == nullptr) {
            continue;
        }
        const ui8 distance = Node.second.Distance + HammingDistance<ui8>(BFSContext->LevelKeys[LevelIndex], static_cast<ui8>(i));
        if (distance > BFSContext->Distance) {
            continue;
        }
        if (LevelIndex + 1 < GetLevelCount()) {
            NextLevel->push_back(std::pair<const TMiddleNode*, TNodeContext>(reinterpret_cast<const TMiddleNode*>(voidPtr), TNodeContext(distance)));
        } else {
            ++BFSContext->Count;
        }
    }
}

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
void TBitTrie<TKey, TValue, ChunkSize, TAllocator>::TCountNeighbors::NodeProcessor(
    ui32 LevelIndex,
    const std::pair<const TMiddleNode*, TNodeContext>& Node,
    TVector<std::pair<const TMiddleNode*, TNodeContext>>* NextLevel,
    TBFSContext* BFSContext) {
    const ui16 compare = Node.second.Compare;
    const ui32 start = (compare > 0) ? 0 : BFSContext->LevelKeys[LevelIndex];

    for (ui32 i = start; i < GetNodePointerCount(); ++i) {
        void* voidPtr = Node.first->GetNext(i);
        if (voidPtr == nullptr) {
            continue;
        }
        const ui8 distance =
            Node.second.Distance +
            HammingDistance<ui8>(BFSContext->LevelKeys[LevelIndex], static_cast<ui8>(i));
        const ui8 newCompare = (compare > 0) ? 1 : ((i == start) ? 0 : 1);
        if (distance > BFSContext->Distance) {
            continue;
        }
        if (LevelIndex + 1 < GetLevelCount()) {
            NextLevel->push_back(std::pair<const TMiddleNode*, TNodeContext>(reinterpret_cast<const TMiddleNode*>(voidPtr), TNodeContext(distance, newCompare)));
        } else {
            TLeafNode* node = reinterpret_cast<TLeafNode*>(voidPtr);

            BFSContext->Group.push_back(node->operator std::pair<TKey, TValue*>());
        }
    }
}

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
void TBitTrie<TKey, TValue, ChunkSize, TAllocator>::TDestructor::NodeProcessor(
    TBitTrie<TKey, TValue, ChunkSize, TAllocator>* parent,
    ui32 levelIndex,
    TMiddleNode* node,
    TVector<TMiddleNode*>* nextLevel) {
    for (ui32 i = 0; i < GetNodePointerCount(); ++i) {
        void* voidPtr = node->GetNext(i);
        if (voidPtr == nullptr) {
            continue;
        }
        if (levelIndex + 1 < GetLevelCount()) {
            nextLevel->push_back(reinterpret_cast<TMiddleNode*>(voidPtr));
        } else {
            TLeafNode* leafNode = reinterpret_cast<TLeafNode*>(voidPtr);

            parent->DeleteLeafNode(leafNode);
        }
    }
    parent->DeleteMiddleNode(node);
}

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
template <class TNodeProcessor, class TNodeContext, class TBFSContext>
void TBitTrie<TKey, TValue, ChunkSize, TAllocator>::BFS(TNodeProcessor Processor, const TNodeContext& RootContext, TBFSContext* BFSContext) const {
    TVector<std::pair<const TMiddleNode*, TNodeContext>> temp1;
    TVector<std::pair<const TMiddleNode*, TNodeContext>> temp2;

    TVector<std::pair<const TMiddleNode*, TNodeContext>>* level = &temp1;
    TVector<std::pair<const TMiddleNode*, TNodeContext>>* nextLevel = &temp2;

    level->push_back(std::make_pair(&root, RootContext));

    ui32 levelIndex = 0;
    while (level->size() > 0) {
        for (ui32 i = 0; i < (ui32)level->size(); ++i) {
            Processor(levelIndex, level->at(i), nextLevel, BFSContext);
        }
        ++levelIndex;
        DoSwap(level, nextLevel);
        nextLevel->clear();
    }
}

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
template <class TNodeProcessor>
void TBitTrie<TKey, TValue, ChunkSize, TAllocator>::BFS(TNodeProcessor Processor) {
    TVector<TMiddleNode*> temp1;
    TVector<TMiddleNode*> temp2;

    TVector<TMiddleNode*>* level = &temp1;
    TVector<TMiddleNode*>* nextLevel = &temp2;

    level->push_back(&root);

    ui32 levelIndex = 0;
    while (level->size() > 0) {
        for (ui32 i = 0; i < (ui32)level->size(); ++i) {
            Processor(this, levelIndex, level->at(i), nextLevel);
        }
        ++levelIndex;
        DoSwap(level, nextLevel);
        nextLevel->clear();
    }
}
