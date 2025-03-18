#pragma once

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
struct TBitTrie<TKey, TValue, ChunkSize, TAllocator>::TMiddleNode {
public:
    TMiddleNode()
        : count(0)
    {
        memset(next, 0, sizeof(void*) * NODE_POINTER_COUNT);
    }

public:
    ui32 GetCount() const {
        return count;
    }

    void* GetNext(ui32 Index) const {
        Y_ASSERT((Index < NODE_POINTER_COUNT) && "Index out of range");

        return next[Index];
    }

    void SetNext(ui32 Index, void* P) {
        Y_ASSERT((Index < NODE_POINTER_COUNT) && "Index out of range");

        if (next[Index] == P) {
            return;
        }

        next[Index] = P;

        if (P != nullptr) {
            ++count;
        } else {
            Y_ASSERT((count > 0) && "Your program is wrong");
            --count;
        }
    }

    void Clear() {
        memset(next, 0, sizeof(void*) * NODE_POINTER_COUNT);
        count = 0;
    }

private:
    void* next[NODE_POINTER_COUNT];
    ui32 count;
};

template <class TKey, class TValue, ui32 ChunkSize, class TAllocator>
struct TBitTrie<TKey, TValue, ChunkSize, TAllocator>::TLeafNode {
    TKey Key;
    TValue Value;

    TLeafNode()
        : Key()
        , Value()
    {
    }

    operator TKey() const {
        return Key;
    }
    operator std::pair<TKey, TValue>() const {
        return std::pair<TKey, TValue>(Key, Value);
    }
    operator std::pair<TKey, const TValue*>() const {
        return std::pair<TKey, const TValue*>(Key, &Value);
    }
    operator std::pair<TKey, TValue*>() {
        return std::pair<TKey, TValue*>(Key, &Value);
    }
    operator TValue*() {
        return &Value;
    }
    operator const TValue*() const {
        return &Value;
    }
    operator TValueGetter<TValue>() const {
        return TValueGetter<TValue>(Value);
    }
};

template <class TParent, size_t LevelCount, size_t NodePointerCount>
class TBitTrieIterator {
public:
    TBitTrieIterator(TParent& parent)
        : Parent(parent)
        , EndReached(1)
    {
    }

    TBitTrieIterator(const TBitTrieIterator& other)
        : Parent(other.Parent)
        , EndReached(other.EndReached)
    {
        memcpy(LevelKeys, other.LevelKeys, sizeof(ui8) * LevelCount);
        memcpy(Nodes, other.Nodes, sizeof(void*) * (LevelCount + 1));
    }

public:
    i8 IsEndReached() const {
        return EndReached;
    }

    bool IsMinEndReached() const {
        return EndReached < 0;
    }

    bool IsMaxEndReached() const {
        return EndReached > 0;
    }

    bool Inc() {
        if (EndReached < 0) {
            return MoveToMin();
        }
        if (Y_UNLIKELY(EndReached > 0)) {
            ythrow yexception() << "Iterator overflow";
        }
        size_t currentLevel = LevelCount;
        bool found = false;
        while (currentLevel > 0) {
            typename TParent::TMiddleNode* currentNode =
                reinterpret_cast<typename TParent::TMiddleNode*>(Nodes[currentLevel - 1]);
            for (size_t nodeIndex = LevelKeys[currentLevel - 1] + 1;
                 nodeIndex < NodePointerCount;
                 ++nodeIndex) {
                void* nextNode = currentNode->GetNext((ui32)nodeIndex);
                if (nextNode != nullptr) {
                    found = true;
                    LevelKeys[currentLevel - 1] = nodeIndex;
                    Nodes[currentLevel] = nextNode;
                    break;
                }
            }
            if (found) {
                break;
            }
            --currentLevel;
        }
        if (!found) {
            EndReached = 1;
            return false;
        }
        while (currentLevel < LevelCount) {
            found = false;
            typename TParent::TMiddleNode* currentNode =
                reinterpret_cast<typename TParent::TMiddleNode*>(Nodes[currentLevel]);
            for (size_t nodeIndex = 0;
                 nodeIndex < NodePointerCount;
                 ++nodeIndex) {
                void* nextNode = currentNode->GetNext((ui32)nodeIndex);
                if (nextNode != nullptr) {
                    found = true;
                    LevelKeys[currentLevel] = nodeIndex;
                    Nodes[currentLevel + 1] = nextNode;
                    break;
                }
            }
            if (Y_UNLIKELY(!found)) {
                ythrow yexception() << "Corrupted internal state";
            }
            ++currentLevel;
        }
        EndReached = 0;
        return true;
    }

    bool Dec() {
        if (EndReached > 0) {
            return MoveToMax();
        }
        if (Y_UNLIKELY(EndReached < 0)) {
            ythrow yexception() << "Iterator overflow";
        }
        size_t currentLevel = LevelCount;
        bool found = false;
        while (currentLevel > 0) {
            typename TParent::TMiddleNode* currentNode =
                reinterpret_cast<typename TParent::TMiddleNode*>(Nodes[currentLevel - 1]);
            if (LevelKeys[currentLevel - 1] == 0) {
                --currentLevel;
                continue;
            }
            for (size_t nodeIndex = LevelKeys[currentLevel - 1] - 1;;
                 --nodeIndex) {
                void* nextNode = currentNode->GetNext((ui32)nodeIndex);
                if (nextNode != nullptr) {
                    found = true;
                    LevelKeys[currentLevel - 1] = nodeIndex;
                    Nodes[currentLevel] = nextNode;
                    break;
                }
                if (nodeIndex == 0) {
                    break;
                }
            }
            if (found) {
                break;
            }
            --currentLevel;
        }
        if (!found) {
            EndReached = -1;
            return false;
        }
        while (currentLevel < LevelCount) {
            found = false;
            typename TParent::TMiddleNode* currentNode =
                reinterpret_cast<typename TParent::TMiddleNode*>(Nodes[currentLevel]);
            for (size_t nodeIndex = NodePointerCount - 1;;
                 --nodeIndex) {
                void* nextNode = currentNode->GetNext((ui32)nodeIndex);
                if (nextNode != nullptr) {
                    found = true;
                    LevelKeys[currentLevel] = nodeIndex;
                    Nodes[currentLevel + 1] = nextNode;
                    break;
                }
                if (nodeIndex == 0) {
                    break;
                }
            }
            if (Y_UNLIKELY(!found)) {
                ythrow yexception() << "Corrupted internal state";
            }
            ++currentLevel;
        }
        EndReached = 0;
        return true;
    }

    bool MoveToMin() {
        if (Parent.GetCount() != 0) {
            EndReached = 0;
            Parent.GetMinNodePath(LevelKeys, Nodes);
            return true;
        } else {
            EndReached = -1;
            return false;
        }
    }

    bool MoveToMax() {
        if (Parent.GetCount() != 0) {
            EndReached = 0;
            Parent.GetMaxNodePath(LevelKeys, Nodes);
            return true;
        } else {
            EndReached = 1;
            return false;
        }
    }

    typename TParent::TKeyType GetCurrentKey() const {
        if (Y_UNLIKELY(EndReached != 0)) {
            ythrow yexception() << "Bad iterator";
        }
        const typename TParent::TLeafNode* node =
            reinterpret_cast<const typename TParent::TLeafNode*>(Nodes[LevelCount]);

        return node->Key;
    }

    const typename TParent::TValueType& GetCurrentValue() const {
        if (Y_UNLIKELY(EndReached != 0)) {
            ythrow yexception() << "Bad iterator";
        }
        const typename TParent::TLeafNode* node =
            reinterpret_cast<const typename TParent::TLeafNode*>(Nodes[LevelCount]);

        return node->Value;
    }

    typename TParent::TValueType& GetCurrentValue() {
        if (Y_UNLIKELY(EndReached != 0)) {
            ythrow yexception() << "Bad iterator";
        }
        typename TParent::TLeafNode* node =
            reinterpret_cast<typename TParent::TLeafNode*>(Nodes[LevelCount]);

        return node->Value;
    }

public:
    TBitTrieIterator& operator=(const TBitTrieIterator& other) {
        new (this) TBitTrieIterator(other);
        return *this;
    }

    bool operator==(const TBitTrieIterator& other) const {
        if (Y_UNLIKELY(&Parent != &other.Parent)) {
            ythrow yexception() << "Different parents";
        }
        if (Parent.GetCount() == 0) {
            if (Y_UNLIKELY(EndReached == 0)) {
                ythrow yexception() << "Corrupted internal state";
            }
            if (Y_UNLIKELY(other.EndReached == 0)) {
                ythrow yexception() << "Corrupted internal state";
            }
            return true;
        }
        if (EndReached == other.EndReached) {
            if (EndReached == 0) {
                for (size_t i = 0; i < LevelCount; ++i) {
                    if (LevelKeys[i] != other.LevelKeys[i]) {
                        return false;
                    }
                    if (Y_UNLIKELY(Nodes[i] != other.Nodes[i])) {
                        ythrow yexception() << "Corrupted internal state";
                    }
                }
                if (Y_UNLIKELY(Nodes[LevelCount] != other.Nodes[LevelCount])) {
                    ythrow yexception() << "Corrupted internal state";
                }
            }
            return true;
        } else {
            return false;
        }
    }

    bool operator!=(const TBitTrieIterator& other) const {
        return !(other == *this);
    }

    bool operator<(const TBitTrieIterator& other) const {
        if (Y_UNLIKELY(&Parent != &other.Parent)) {
            ythrow yexception() << "Different parents";
        }
        if (Parent.GetCount() == 0) {
            if (Y_UNLIKELY(EndReached == 0)) {
                ythrow yexception() << "Corrupted internal state";
            }
            if (Y_UNLIKELY(other.EndReached == 0)) {
                ythrow yexception() << "Corrupted internal state";
            }
            return false;
        }
        if (EndReached < other.EndReached) {
            return true;
        } else if (EndReached > other.EndReached) {
            return false;
        } else {
            if (EndReached == 0) {
                for (size_t i = 0; i < LevelCount; ++i) {
                    if (LevelKeys[i] < other.LevelKeys[i]) {
                        return true;
                    }
                    if (LevelKeys[i] > other.LevelKeys[i]) {
                        return false;
                    }
                    if (Y_UNLIKELY(Nodes[i] != other.Nodes[i])) {
                        ythrow yexception() << "Corrupted internal state";
                    }
                }
                if (Y_UNLIKELY(Nodes[LevelCount] != other.Nodes[LevelCount])) {
                    ythrow yexception() << "Corrupted internal state";
                }
            }
            return false;
        }
    }

    bool operator<=(const TBitTrieIterator& other) const {
        if (Y_UNLIKELY(&Parent != &other.Parent)) {
            ythrow yexception() << "Different parents";
        }
        if (Parent.GetCount() == 0) {
            if (Y_UNLIKELY(EndReached == 0)) {
                ythrow yexception() << "Corrupted internal state";
            }
            if (Y_UNLIKELY(other.EndReached == 0)) {
                ythrow yexception() << "Corrupted internal state";
            }
            return true;
        }
        if (EndReached < other.EndReached) {
            return true;
        } else if (EndReached > other.EndReached) {
            return false;
        } else {
            if (EndReached == 0) {
                for (size_t i = 0; i < LevelCount; ++i) {
                    if (LevelKeys[i] < other.LevelKeys[i]) {
                        return true;
                    }
                    if (LevelKeys[i] > other.LevelKeys[i]) {
                        return false;
                    }
                    if (Y_UNLIKELY(Nodes[i] != other.Nodes[i])) {
                        ythrow yexception() << "Corrupted internal state";
                    }
                }
                if (Y_UNLIKELY(Nodes[LevelCount] != other.Nodes[LevelCount])) {
                    ythrow yexception() << "Corrupted internal state";
                }
            }
            return true;
        }
    }

    bool operator>(const TBitTrieIterator& other) const {
        return !(*this <= other);
    }

    bool operator>=(const TBitTrieIterator& other) const {
        return !(*this < other);
    }

private:
    TParent& Parent;
    i8 EndReached;
    ui8 LevelKeys[LevelCount];
    void* Nodes[LevelCount + 1];
};
