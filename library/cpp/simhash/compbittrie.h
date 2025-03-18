#pragma once

#include <library/cpp/pop_count/popcount.h>

#include <util/stream/output.h>
#include <utility>

#include <util/memory/blob.h>
#include <util/generic/noncopyable.h>
#include <util/system/defaults.h>
#include <util/generic/algorithm.h>
#include <util/generic/stack.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>
#include <util/generic/queue.h>
#include <library/cpp/deprecated/fgood/fgood.h>

#include <util/generic/bt_exception.h>

template <class TParent>
class TCompBitTrieIterator {
public:
    TCompBitTrieIterator(TParent& parent)
        : Parent(&parent)
        , EndReached(1)
        , Path()
    {
    }

    TCompBitTrieIterator(const TCompBitTrieIterator& other)
        : Parent(other.Parent)
        , EndReached(other.EndReached)
        , Path(other.Path)
    {
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
            ythrow TWithBackTrace<yexception>() << "Iterator overflow";
        }
        typename TParent::TNode* current = Path.top();
        if (current->Right != nullptr) {
            Path.push(current->Right);
            MoveCurrentToMin();
            return true;
        }
        while (true) {
            Path.pop();
            if (Path.size() == 0) {
                EndReached = 1;
                return false;
            }
            typename TParent::TNode* parent = Path.top();
            if (parent->Left == current) {
                return true;
            } else if (parent->Right != current) {
                ythrow TWithBackTrace<yexception>() << "Corrupted internal state";
            }
            current = parent;
        }
    }

    bool Dec() {
        if (EndReached > 0) {
            return MoveToMax();
        }
        if (Y_UNLIKELY(EndReached < 0)) {
            ythrow TWithBackTrace<yexception>() << "Iterator overflow";
        }
        typename TParent::TNode* current = Path.top();
        if (current->Left != nullptr) {
            Path.push(current->Left);
            MoveCurrentToMax();
            return true;
        }
        while (true) {
            Path.pop();
            if (Path.size() == 0) {
                EndReached = -1;
                return false;
            }
            typename TParent::TNode* parent = Path.top();
            if (parent->Right == current) {
                return true;
            } else if (parent->Left != current) {
                ythrow TWithBackTrace<yexception>() << "Corrupted internal state";
            }
            current = parent;
        }
    }

    bool MoveToMin() {
        Path = TStack<typename TParent::TNode*>();
        if (Parent->GetCount() != 0) {
            EndReached = 0;
            Path.push(Parent->Root);
            MoveCurrentToMin();
            return true;
        } else {
            EndReached = -1;
            return false;
        }
    }

    bool MoveToMax() {
        Path = TStack<typename TParent::TNode*>();
        if (Parent->GetCount() != 0) {
            EndReached = 0;
            Path.push(Parent->Root);
            MoveCurrentToMax();
            return true;
        } else {
            EndReached = 1;
            return false;
        }
    }

    const typename TParent::TSimhash& GetCurrentKey() const {
        if (Y_UNLIKELY(EndReached != 0)) {
            ythrow TWithBackTrace<yexception>() << "Bad iterator";
        }
        return GetCurrentNode()->Simhash;
    }

    const typename TParent::TValue& GetCurrentValue() const {
        if (Y_UNLIKELY(EndReached != 0)) {
            ythrow TWithBackTrace<yexception>() << "Bad iterator";
        }
        return GetCurrentNode()->Data;
    }

    typename TParent::TValue& GetCurrentValue() {
        if (Y_UNLIKELY(EndReached != 0)) {
            ythrow TWithBackTrace<yexception>() << "Bad iterator";
        }
        return GetCurrentNode()->Data;
    }

public:
    TCompBitTrieIterator& operator=(const TCompBitTrieIterator& other) {
        Parent = other.Parent;
        EndReached = other.EndReached;
        Path = other.Path;

        return *this;
    }

    bool operator==(const TCompBitTrieIterator& other) const {
        if (Y_UNLIKELY(Parent != other.Parent)) {
            ythrow TWithBackTrace<yexception>() << "Different parents";
        }
        if (Parent->GetCount() == 0) {
            if (Y_UNLIKELY(EndReached == 0)) {
                ythrow TWithBackTrace<yexception>() << "Corrupted internal state";
            }
            if (Y_UNLIKELY(other.EndReached == 0)) {
                ythrow TWithBackTrace<yexception>() << "Corrupted internal state";
            }
            return true;
        }
        if (EndReached == other.EndReached) {
            if (EndReached == 0) {
                int cmp = TParent::TTraits::CompareSimhashes(GetCurrentKey(), other.GetCurrentKey());
                return cmp == 0;
            }
            return true;
        } else {
            return false;
        }
    }

    bool operator!=(const TCompBitTrieIterator& other) const {
        return !(other == *this);
    }

    bool operator<(const TCompBitTrieIterator& other) const {
        if (Y_UNLIKELY(Parent != other.Parent)) {
            ythrow TWithBackTrace<yexception>() << "Different parents";
        }
        if (Parent->GetCount() == 0) {
            if (Y_UNLIKELY(EndReached == 0)) {
                ythrow TWithBackTrace<yexception>() << "Corrupted internal state";
            }
            if (Y_UNLIKELY(other.EndReached == 0)) {
                ythrow TWithBackTrace<yexception>() << "Corrupted internal state";
            }
            return false;
        }
        if (EndReached < other.EndReached) {
            return true;
        } else if (EndReached > other.EndReached) {
            return false;
        } else {
            if (EndReached == 0) {
                int cmp = TParent::TTraits::CompareSimhashes(GetCurrentKey(), other.GetCurrentKey());
                return cmp < 0;
            }
            return false;
        }
    }

    bool operator<=(const TCompBitTrieIterator& other) const {
        if (Y_UNLIKELY(Parent != other.Parent)) {
            ythrow TWithBackTrace<yexception>() << "Different parents";
        }
        if (Parent->GetCount() == 0) {
            if (Y_UNLIKELY(EndReached == 0)) {
                ythrow TWithBackTrace<yexception>() << "Corrupted internal state";
            }
            if (Y_UNLIKELY(other.EndReached == 0)) {
                ythrow TWithBackTrace<yexception>() << "Corrupted internal state";
            }
            return true;
        }
        if (EndReached < other.EndReached) {
            return true;
        } else if (EndReached > other.EndReached) {
            return false;
        } else {
            if (EndReached == 0) {
                int cmp = TParent::TTraits::CompareSimhashes(GetCurrentKey(), other.GetCurrentKey());
                return cmp <= 0;
            }
            return true;
        }
    }

    bool operator>(const TCompBitTrieIterator& other) const {
        return !(*this <= other);
    }

    bool operator>=(const TCompBitTrieIterator& other) const {
        return !(*this < other);
    }

private:
    typename TParent::TNode* GetCurrentNode() const {
        if (Path.size() == 0) {
            ythrow TWithBackTrace<yexception>() << "Corrupted internal state";
        }
        return Path.top();
    }

    void MoveCurrentToMin() {
        while (true) {
            typename TParent::TNode* node = Path.top();
            if (node->Left != nullptr) {
                Path.push(node->Left);
            } else {
                break;
            }
        }
    }

    void MoveCurrentToMax() {
        while (true) {
            typename TParent::TNode* node = Path.top();
            if (node->Right != nullptr) {
                Path.push(node->Right);
            } else {
                break;
            }
        }
    }

private:
    TParent* Parent;
    i8 EndReached;
    TStack<typename TParent::TNode*> Path;
};

template <typename TSimhashType>
struct TSimhashTraits {
    typedef TSimhashType TSimhash;
    typedef TSimhashType TSimhashParam;
    typedef ui8 TBitIndex;

    static const ui32 BitCount = sizeof(TSimhash) * 8;

    static int CompareSimhashes(const TSimhashParam left, const TSimhashParam right) {
        if (left < right) {
            return -1;
        } else if (left > right) {
            return 1;
        } else {
            return 0;
        }
    }

    struct TBitCompareContext {
        TBitCompareContext()
            : Left()
            , Right()
            , Shift(0)
        {
        }

        void SetLeft(TSimhashParam left) {
            Left = left;
            if (Shift < BitCount) {
                Left <<= Shift;
            } else {
                Left = 0;
            }
        }

        void SetRight(TSimhashParam right) {
            Right = right;
            if (Shift < BitCount) {
                Right <<= Shift;
            } else {
                Right = 0;
            }
        }

        int CompareNextBits() {
            if (Shift > BitCount) {
                ythrow TWithBackTrace<yexception>() << "Wrong internal state";
            }
            ui32 l = CountLeadingZeroBits(Left);
            ui32 r = CountLeadingZeroBits(Right);
            ui32 shift = Min(l, r) + 1;
            Shift += shift;
            if (shift < BitCount) {
                Left <<= shift;
                Right <<= shift;
            } else {
                Left = 0;
                Right = 0;
            }

            if (l < r) {
                return -1;
            } else if (l > r) {
                return 1;
            } else {
                return 0;
            }
        }

        ui32 GetShift() const {
            return Shift;
        }

        ui32 GetNextLeftDividingBit() const {
            ui32 res = Shift + CountLeadingZeroBits(Left);
            return res > BitCount ? BitCount : res;
        }

    private:
        TSimhashType Left;
        TSimhashType Right;
        ui32 Shift;
    };

    static ui32 CountLeadingZeroBits(const TSimhashParam n) {
        if (n == 0) {
            return BitCount;
        }
#ifdef _unix_
        ui32 sub = (sizeof(unsigned long long) - sizeof(TSimhash)) * 8;
        if (sub > 56) {
            ythrow TWithBackTrace<yexception>() << "Wrong algorithm";
        }
        ui32 res = (ui32)__builtin_clzll((unsigned long long)n);
        if (res < sub) {
            ythrow TWithBackTrace<yexception>() << "Wrong algorithm";
        }
        return res - sub;
#else
        ythrow TWithBackTrace<yexception>() << "not implemented yet";
#endif
    }

    static void SetBit(TSimhash& simhash, TBitIndex index) {
        if (index >= BitCount) {
            ythrow TWithBackTrace<yexception>() << "BitIndex overflow";
        }
        simhash |= ((TSimhash)1) << (BitCount - index - 1);
    }

    static void ResetBit(TSimhash& simhash, TBitIndex index) {
        if (index >= BitCount) {
            ythrow TWithBackTrace<yexception>() << "BitIndex overflow";
        }
        simhash &= ~(((TSimhash)1) << (BitCount - index - 1));
    }

    //    static bool GetBit(const TSimhashParam n, TBitIndex index) {
    //        if (index >= BitCount) {
    //            ythrow TWithBackTrace<yexception>() << "BitIndex overflow";
    //        }
    //        return n & (((TSimhash)1) << (BitCount - index - 1));
    //    }

    static ui32 HammingDistance(
        TSimhash left,
        TSimhash right,
        TBitIndex prefixLength = sizeof(TSimhash) * 8) {
        if (prefixLength > BitCount) {
            ythrow TWithBackTrace<yexception>() << "BitIndex overflow";
        }
        if (prefixLength == 0) {
            return 0;
        }
        if (prefixLength < BitCount) {
            left >>= (BitCount - prefixLength);
            right >>= (BitCount - prefixLength);
        }
        return PopCount(left ^ right);
    }

    //    static void CopyBits(
    //            TSimhash from,
    //            TSimhash& to,
    //            TBitIndex start,
    //            TBitIndex length)
    //    {
    //        if (start + length > BitCount) {
    //            ythrow TWithBackTrace<yexception>() << "BitIndex overflow";
    //        }
    //        from <<= start;
    //        from >>= (BitCount - length);
    //        from <<= (BitCount - length - start);
    //        to |= from;
    //    }

    static bool HasCommonPrefixOfLength(
        TSimhash left,
        TSimhash right,
        TBitIndex length) {
        if (length == 0) {
            return true;
        }
        if (length > BitCount) {
            ythrow TWithBackTrace<yexception>() << "BitIndex overflow";
        }
        left >>= (BitCount - length);
        right >>= (BitCount - length);
        return left == right;
    }
};

namespace NCompBitTrieGetters {
    template <typename TSimhash>
    struct TSimhashGetter {
        TSimhash Simhash;

        TSimhashGetter(const TSimhash& simhash)
            : Simhash(simhash)
        {
        }

        operator TSimhash() const {
            return Simhash;
        }
    };

    template <typename TValue>
    struct TValueGetter {
        TValue Value;

        TValueGetter(const TValue& value)
            : Value(value)
        {
        }

        operator TValue() const {
            return Value;
        }
    };
}

namespace NImpl {
    template <typename TSimhashType, typename TValueType>
    struct TIndexNode {
        enum EIndexValues : ui32 { MAX_INDEX = 0xFFFFFFFF };

        typedef TSimhashType TSimhash;
        typedef TValueType TValue;

        operator std::pair<TSimhash, TValue>() const {
            return std::pair<TSimhash, TValue>(Simhash, Data);
        }

        operator std::pair<TSimhash, const TValue*>() const {
            return std::pair<TSimhash, const TValue*>(Simhash, &Data);
        }

        operator std::pair<TSimhash, TValue*>() {
            return std::pair<TSimhash, TValue*>(Simhash, &Data);
        }

        operator const TValue&() const {
            return Data;
        }

        operator TValue() const {
            return Data;
        }

        operator const TValue*() const {
            return &Data;
        }

        operator TValue*() {
            return &Data;
        }

        TSimhashType Simhash;
        TValueType Data;
        ui32 Left;
        ui32 Right;
        ui8 DividingBit;
    };

}

template <typename TSimhashType, typename TValueType, template <typename> class TSimhashTraits = ::TSimhashTraits>
class TCompBitTrie {
    friend class TCompBitTrieIterator<TCompBitTrie>;
    friend class TCompBitTrieIterator<const TCompBitTrie>;

public:
    typedef TSimhashTraits<TSimhashType> TTraits;

    typedef TSimhashType TSimhash;
    typedef TValueType TValue;
    typedef typename TTraits::TSimhashParam TSimhashParam;
    typedef typename TTraits::TBitIndex TBitIndex;

    typedef TCompBitTrieIterator<TCompBitTrie> TIt;
    typedef TCompBitTrieIterator<const TCompBitTrie> TConstIt;

private:
    struct TNode {
        TSimhash Simhash;
        TNode* Left;
        TNode* Right;
        TValue Data;
        TBitIndex DividingBit;

        TNode(TSimhashParam simhash, const TValue& data)
            : Simhash(simhash)
            , Left(nullptr)
            , Right(nullptr)
            , Data(data)
            , DividingBit(0)
        {
        }

        operator std::pair<TSimhash, TValue>() const {
            return std::pair<TSimhash, TValue>(Simhash, Data);
        }
        operator std::pair<TSimhash, const TValue*>() const {
            return std::pair<TSimhash, const TValue*>(Simhash, &Data);
        }
        operator std::pair<TSimhash, TValue*>() {
            return std::pair<TSimhash, TValue*>(Simhash, &Data);
        }

        operator TValue() {
            return Data;
        }

        operator const TValue&() const {
            return Data;
        }
        operator TValue&() {
            return Data;
        }

        operator const TValue*() const {
            return &Data;
        }
        operator TValue*() {
            return &Data;
        }

        operator NCompBitTrieGetters::TValueGetter<TValue>() const {
            return NCompBitTrieGetters::TValueGetter<TValue>(Data);
        }

        operator NCompBitTrieGetters::TSimhashGetter<TSimhash>() const {
            return NCompBitTrieGetters::TSimhashGetter<TSimhash>(Simhash);
        }
    };

public:
    TCompBitTrie()
        : Root(nullptr)
        , Count(0)
        , DebugCount(0)
    {
    }

    ~TCompBitTrie() {
        Clear();
    }

public:
    TConstIt CMin() const {
        TConstIt it(*this);
        it.MoveToMin();
        return it;
    }

    TConstIt CMax() const {
        TConstIt it(*this);
        it.MoveToMax();
        return it;
    }

    TIt Min() {
        TIt it(*this);
        it.MoveToMin();
        return it;
    }

    TIt Max() {
        TIt it(*this);
        it.MoveToMax();
        return it;
    }

    //    TConstIt Find(TKey Key) const;
    //    TIt Find(TKey Key);

public:
    void Clear() {
        if (Count == 0) {
            return;
        }
        RecursiveClear(Root);
        Root = nullptr;
        Count = 0;
    }

    ui64 GetCount() const {
        return Count;
    }

    bool Has(const TSimhashParam simhash) const {
        const TNode* node = Root;
        while (node != nullptr) {
            int cmp = TTraits::CompareSimhashes(simhash, node->Simhash);
            if (cmp < 0) {
                node = node->Left;
            } else if (cmp > 0) {
                node = node->Right;
            } else {
                return true;
            }
        }
        return false;
    }

    const TValue& Get(const TSimhashParam simhash) const {
        const TNode* node = Root;
        while (node != nullptr) {
            int cmp = TTraits::CompareSimhashes(simhash, node->Simhash);
            if (cmp < 0) {
                node = node->Left;
            } else if (cmp > 0) {
                node = node->Right;
            } else {
                return node->Data;
            }
        }
        ythrow TWithBackTrace<yexception>() << "no such key";
    }

    TValue& Get(const TSimhashParam simhash, bool create = false) {
        if (!create) {
            TNode* node = Root;
            while (node != nullptr) {
                int cmp = TTraits::CompareSimhashes(simhash, node->Simhash);
                if (cmp < 0) {
                    node = node->Left;
                } else if (cmp > 0) {
                    node = node->Right;
                } else {
                    return node->Data;
                }
            }
            ythrow TWithBackTrace<yexception>() << "no such key";
        } else {
            TValue* res = nullptr;
            Insert(simhash, TValue(), &res);
            return *res;
        }
    }

    bool TryGet(const TSimhashParam simhash, TValue** value) const {
        const TNode* node = Root;
        while (node != nullptr) {
            int cmp = TTraits::CompareSimhashes(simhash, node->Simhash);
            if (cmp < 0) {
                node = node->Left;
            } else if (cmp > 0) {
                node = node->Right;
            } else {
                *value = const_cast<TValue*>(&node->Data);
                return true;
            }
        }
        return false;
    }

    bool Insert(
        const TSimhashParam simhash,
        const TValue& value,
        TValue** oldValue = nullptr) {
        typename TTraits::TBitCompareContext context;
        context.SetLeft(simhash);

        TNode* node = nullptr;
        TNode** currentNode = &Root;

        bool modified = false;

        while (*currentNode != nullptr) {
            context.SetRight((*currentNode)->Simhash);
            int cmp = context.CompareNextBits();
            if (cmp < 0) {
                ui32 currentBit = context.GetShift();
                if (currentBit == 0) {
                    ythrow TWithBackTrace<yexception>() << "Wrong algorithm";
                }
                --currentBit;
                if (node == nullptr) {
                    node = new TNode(simhash, value);
                    modified = true;
                }
                node->DividingBit = (TBitIndex)currentBit;
                node->Left = (*currentNode);
                (*currentNode) = node;
                ++Count;
                if (oldValue != nullptr) {
                    *oldValue = &node->Data;
                }
                return true;
            } else if (cmp > 0) {
                currentNode = &(*currentNode)->Left;
            } else {
                cmp = TTraits::CompareSimhashes(
                    (node == nullptr ? simhash : node->Simhash),
                    (*currentNode)->Simhash);
                if (cmp < 0) {
                    ui32 currentBit = context.GetShift();
                    if (currentBit == 0) {
                        ythrow TWithBackTrace<yexception>() << "Wrong algorithm";
                    }
                    --currentBit;

                    if (node == nullptr) {
                        node = new TNode(simhash, value);
                        if (oldValue != nullptr) {
                            *oldValue = &node->Data;
                        }
                        modified = true;
                    }
                    node->DividingBit = (TBitIndex)currentBit;
                    node->Left = (*currentNode)->Left;
                    node->Right = (*currentNode)->Right;
                    (*currentNode)->Left = nullptr;
                    (*currentNode)->Right = nullptr;

                    DoSwap(*currentNode, node);
                    context.SetLeft(node->Simhash);

                    currentNode = &(*currentNode)->Right;
                } else if (cmp > 0) {
                    currentNode = &(*currentNode)->Right;
                } else {
                    if (modified) {
                        ythrow TWithBackTrace<yexception>() << "Wrong algorithm";
                    }
                    if (oldValue != nullptr) {
                        *oldValue = &(*currentNode)->Data;
                    }
                    return false;
                }
            }
        }

        if (node == nullptr) {
            node = new TNode(simhash, value);

            if (oldValue != nullptr) {
                *oldValue = &node->Data;
            }
        }
        node->DividingBit = context.GetNextLeftDividingBit();
        *currentNode = node;

        ++Count;

        return true;
    }

    bool Delete(const TSimhashParam simhash, TValue* value = nullptr) {
        TNode* currentNode1 = nullptr;
        TNode** currentPNode = &Root;
        while (*currentPNode != nullptr) {
            TNode* currentNode2 = *currentPNode;
            int cmp = TTraits::CompareSimhashes(simhash, currentNode2->Simhash);
            if (cmp < 0) {
                currentPNode = &currentNode2->Left;
            } else if (cmp > 0) {
                currentPNode = &currentNode2->Right;
            } else {
                break;
            }
        }
        if (*currentPNode == nullptr) {
            return false;
        }
        currentNode1 = *currentPNode;
        if (currentNode1->Right == nullptr) {
            *currentPNode = currentNode1->Left;
        } else {
            TNode* minimalRightNode = RemoveMinimal(&currentNode1->Right);
            minimalRightNode->Left = currentNode1->Left;
            minimalRightNode->Right = currentNode1->Right;
            minimalRightNode->DividingBit = currentNode1->DividingBit;
            *currentPNode = minimalRightNode;
        }
        if (value != nullptr) {
            *value = currentNode1->Data;
        }
        delete currentNode1;
        --Count;
        return true;
    }

    ui32 CheckConsistensy() const {
        if (Root == nullptr) {
            return 0;
        }
        TSimhash n = 0;
        ui64 count = 0;
        ui32 depth = CheckConsistensyImpl(Root, n, count);
        if (count != Count) {
            ythrow TWithBackTrace<yexception>() << "Count mismatch";
        }
        return depth;
    }

public:
    template <class TRes>
    void GetNeighbors(
        const TSimhashParam simhash,
        ui32 distance,
        TVector<TRes>* simhashes,
        bool clearRes = true) {
        if (clearRes) {
            simhashes->clear();
        }
        if (Root == nullptr) {
            return;
        }
        TGetNeighborsCallback<TRes, TNode> callback;
        callback.Simhashes = simhashes;

        RecursiveWalk(Root, simhash, distance, callback);
    }

    template <class TRes>
    void GetNeighbors(
        const TSimhashParam simhash,
        ui32 distance,
        TVector<TRes>* simhashes,
        bool clearRes = true) const {
        if (clearRes) {
            simhashes->clear();
        }
        if (Root == nullptr) {
            return;
        }
        TGetNeighborsCallback<TRes, const TNode> callback;
        callback.Simhashes = simhashes;

        RecursiveWalk(Root, simhash, distance, callback);
    }

    template <typename TRes, typename THints>
    void GetNeighborsWithHints(
        const TSimhashParam simhash,
        ui32 distance,
        TVector<TRes>* simhashes,
        bool clearRes,
        THints& hints) const {
        if (clearRes) {
            simhashes->clear();
        }
        if (Root == NULL) {
            return;
        }
        TGetNeighborsCallback<TRes, const TNode> callback;
        callback.Simhashes = simhashes;

        RecursiveWalkWithHints(Root, simhash, distance, callback, hints);
    }

    ui64 GetNeighborCount(
        const TSimhashParam simhash,
        ui32 distance) const {
        if (Root == nullptr) {
            return 0;
        }
        TGetNeighborCountCallback callback;
        callback.Count = 0;
        RecursiveWalk(Root, simhash, distance, callback);
        return callback.Count;
    }

    void CountNeighbors(
        THashMap<TSimhash, std::pair<TValue*, ui64>>& data,
        ui32 distance) {
        data.clear();
        if (Root == nullptr) {
            return;
        }
        TConstIt it = CMin();
        while (!it.IsEndReached()) {
            TCountNeighborsCallback<TNode, TValue> callback(&it.GetCurrentValue(), it.GetCurrentKey(), &data);
            RecursiveWalkOnGE(Root, it.GetCurrentKey(), distance, callback);
            it.Inc();
        }
    }

    void CountNeighbors(
        THashMap<TSimhash, std::pair<const TValue*, ui64>>& data,
        ui32 distance) const {
        data.clear();
        if (Root == NULL) {
            return;
        }
        TConstIt it = CMin();
        while (!it.IsEndReached()) {
            TCountNeighborsCallback<const TNode, const TValue> callback(&it.GetCurrentValue(), it.GetCurrentKey(), &data);
            RecursiveWalkOnGE(Root, it.GetCurrentKey(), distance, callback);
            it.Inc();
        }
    }

    void Save(IOutputStream& os) {
        if (Root == nullptr) {
            return;
        }

        TQueue<TNode*> queue;
        queue.push(Root);

        ui32 index = 0;
        THashMap<TNode*, ui32> nodeIndexes;
        while (!queue.empty()) {
            TNode* node = queue.front();
            queue.pop();

            auto it = nodeIndexes.find(node);
            Y_ASSERT(it == nodeIndexes.end());

            nodeIndexes[node] = index;

            if (node->Left != nullptr) {
                queue.push(node->Left);
            }

            if (node->Right != nullptr) {
                queue.push(node->Right);
            }

            ++index;
        }

        queue.push(Root);
        while (!queue.empty()) {
            TNode* node = queue.front();
            queue.pop();

            ui32 leftIndex = NImpl::TIndexNode<TSimhashType, TValueType>::MAX_INDEX;
            if (node->Left != nullptr) {
                auto it = nodeIndexes.find(node->Left);
                Y_ASSERT(it != nodeIndexes.end());
                leftIndex = it->second;
            }

            ui32 rightIndex = NImpl::TIndexNode<TSimhashType, TValueType>::MAX_INDEX;
            if (node->Right != nullptr) {
                auto it = nodeIndexes.find(node->Right);
                Y_ASSERT(it != nodeIndexes.end());
                rightIndex = it->second;
            }

            NImpl::TIndexNode<TSimhashType, TValueType> indexNode;
            indexNode.Simhash = node->Simhash;
            indexNode.Data = node->Data;
            indexNode.Left = leftIndex;
            indexNode.Right = rightIndex;
            indexNode.DividingBit = node->DividingBit;

            os.Write(&indexNode, sizeof(NImpl::TIndexNode<TSimhashType, TValueType>));

            if (node->Left != nullptr) {
                queue.push(node->Left);
            }

            if (node->Right != nullptr) {
                queue.push(node->Right);
            }
        }
    }

private:
    TNode* RemoveMinimal(TNode** currentPNode) {
        while ((*currentPNode)->Left != nullptr) {
            currentPNode = &((*currentPNode)->Left);
        }
        TNode* currentNode = *currentPNode;
        if (currentNode->Right != nullptr) {
            TNode* minimalRightNode = RemoveMinimal(&currentNode->Right);
            minimalRightNode->Left = currentNode->Left;
            minimalRightNode->Right = currentNode->Right;
            minimalRightNode->DividingBit = currentNode->DividingBit;
            *currentPNode = minimalRightNode;
        } else {
            *currentPNode = nullptr;
        }
        if (*currentPNode != nullptr) {
            (*currentPNode)->DividingBit = currentNode->DividingBit;
        }
        currentNode->Left = nullptr;
        currentNode->Right = nullptr;
        return currentNode;
    }

    ui32 CheckConsistensyImpl(const TNode* node, TSimhash& n, ui64& count) const {
        ui32 resl = 0;
        ui32 resr = 0;
        if (node->DividingBit > TTraits::BitCount) {
            ythrow TWithBackTrace<yexception>() << "Wrong bit index";
        }
        if (node->Left != nullptr) {
            if (TTraits::CompareSimhashes(node->Left->Simhash, node->Simhash) >= 0) {
                ythrow TWithBackTrace<yexception>() << "not a search tree";
            }
            if (node->Left->DividingBit <= node->DividingBit) {
                ythrow TWithBackTrace<yexception>() << "not a heap";
            }
            TTraits::ResetBit(n, node->DividingBit);
            resl = CheckConsistensyImpl(node->Left, n, count);
        }
        if (node->DividingBit < TTraits::BitCount) {
            TTraits::SetBit(n, node->DividingBit);
            if (!TTraits::HasCommonPrefixOfLength(n, node->Simhash, node->DividingBit + 1)) {
                ythrow TWithBackTrace<yexception>() << "Wrong internal state";
            }
            TTraits::ResetBit(n, node->DividingBit);
        }
        if (node->Right != nullptr) {
            if (TTraits::CompareSimhashes(node->Right->Simhash, node->Simhash) <= 0) {
                ythrow TWithBackTrace<yexception>() << "not a search tree";
            }
            if (node->Right->DividingBit <= node->DividingBit) {
                ythrow TWithBackTrace<yexception>() << "not a heap";
            }
            TTraits::SetBit(n, node->DividingBit);
            resr = CheckConsistensyImpl(node->Right, n, count);
            TTraits::ResetBit(n, node->DividingBit);
        }
        ++count;
        if (resl >= resr) {
            return 1 + resl;
        } else {
            return 1 + resr;
        }
    }

    void RecursiveClear(TNode* node) {
        TNode* left = node->Left;
        TNode* right = node->Right;
        delete node;
        if (left != nullptr) {
            RecursiveClear(left);
        }
        if (right != nullptr) {
            RecursiveClear(right);
        }
    }

private:
    template <typename TRes, typename TNodeType>
    struct TGetNeighborsCallback {
        TVector<TRes>* Simhashes;

        void operator()(TNodeType* node) {
            Simhashes->push_back(node->operator TRes());
        }
    };

    struct TGetNeighborCountCallback {
        ui64 Count;

        void operator()(const TNode*) {
            ++Count;
        }
    };

    template <typename TNodeType, typename TValueT>
    struct TCountNeighborsCallback {
        TValueT* Value;
        const TSimhash& Simhash;
        THashMap<TSimhash, std::pair<TValueT*, ui64>>* Data;

        TCountNeighborsCallback(
            TValueT* value,
            const TSimhash& simhash,
            THashMap<TSimhash, std::pair<TValueT*, ui64>>* data)
            : Value(value)
            , Simhash(simhash)
            , Data(data)
        {
            Data->insert(std::make_pair(Simhash, std::make_pair(Value, 0))).first->second.second += 1;
        }

        void operator()(TNodeType* node) {
            (*Data)[Simhash].second += 1;
            Data->insert(std::make_pair(node->Simhash, std::make_pair(&node->Data, 0))).first->second.second += 1;
        }
    };

#define DRECURSIVE_WALK_IMPL                                         \
    {                                                                \
        ui32 prefixDistance = TTraits::HammingDistance(              \
            node->Simhash,                                           \
            simhash,                                                 \
            node->DividingBit);                                      \
        if (prefixDistance > distance) {                             \
            return;                                                  \
        }                                                            \
        if (node->Left != NULL) {                                    \
            RecursiveWalk(node->Left, simhash, distance, callback);  \
        }                                                            \
        if (TTraits::HammingDistance(                                \
                node->Simhash,                                       \
                simhash) <= distance) {                              \
            callback(node);                                          \
        }                                                            \
        if (node->Right != NULL) {                                   \
            RecursiveWalk(node->Right, simhash, distance, callback); \
        }                                                            \
    }

#define DRECURSIVE_WALK_ONGE_IMPL                                            \
    {                                                                        \
        ui32 prefixDistance = TTraits::HammingDistance(                      \
            node->Simhash,                                                   \
            simhash,                                                         \
            node->DividingBit);                                              \
        if (prefixDistance > distance) {                                     \
            return;                                                          \
        }                                                                    \
        int cmp = TTraits::CompareSimhashes(simhash, node->Simhash);         \
        if (node->Left != NULL && cmp == -1) {                               \
            RecursiveWalkOnGE(node->Left, simhash, distance, callback);      \
        }                                                                    \
        if (cmp < 0) {                                                       \
            callback(node);                                                  \
        }                                                                    \
        if (cmp == 0) {                                                      \
            if (node->Right != NULL) {                                       \
                RecursiveWalk(node->Right, simhash, distance, callback);     \
            }                                                                \
        } else {                                                             \
            if (node->Right != NULL) {                                       \
                RecursiveWalkOnGE(node->Right, simhash, distance, callback); \
            }                                                                \
        }                                                                    \
    }

    template <typename TCallback>
    void RecursiveWalk(
        const TNode* node,
        const TSimhashParam simhash,
        ui32 distance,
        TCallback& callback) const {
        DRECURSIVE_WALK_IMPL
    }

    template <typename TCallback>
    void RecursiveWalk(
        TNode* node,
        const TSimhashParam simhash,
        ui32 distance,
        TCallback& callback) {
        DRECURSIVE_WALK_IMPL
    }

    template <typename TCallback, typename THints>
    void RecursiveWalkWithHints(
        TNode* node,
        const TSimhashParam simhash,
        ui32 distance,
        TCallback& callback,
        THints& hints) const {
        ui32 prefixDistance = TTraits::HammingDistance(
            node->Simhash,
            simhash,
            node->DividingBit);
        if (prefixDistance > distance) {
            return;
        }
        if (!hints.IsPrefixAllowed(node->Simhash, node->DividingBit)) {
            return;
        }
        if (node->Left != NULL) {
            RecursiveWalk(node->Left, simhash, distance, callback);
        }
        if (TTraits::HammingDistance(node->Simhash, simhash) <= distance) {
            callback(node);
        }
        if (node->Right != NULL) {
            RecursiveWalk(node->Right, simhash, distance, callback);
        }
    }

    template <typename TCallback>
    void RecursiveWalkOnGE(
        TNode* node,
        const TSimhashParam simhash,
        ui32 distance,
        TCallback& callback) {
        DRECURSIVE_WALK_ONGE_IMPL
    }

    template <typename TCallback>
    void RecursiveWalkOnGE(
        const TNode* node,
        const TSimhashParam simhash,
        ui32 distance,
        TCallback& callback) const {
        DRECURSIVE_WALK_ONGE_IMPL
    }

private:
    TNode* Root;
    ui64 Count;
    mutable ui64 DebugCount;
};

template <typename TSimhashType, typename TValueType, template <typename> class TSimhashTraits = ::TSimhashTraits>
class TCompBitIndex: private TNonCopyable {
private:
    typedef TSimhashTraits<TSimhashType> TTraits;

    typedef TSimhashType TSimhash;
    typedef TValueType TValue;
    typedef typename TTraits::TSimhashParam TSimhashParam;
    typedef typename TTraits::TBitIndex TBitIndex;

public:
    TCompBitIndex()
        : DataBlob()
        , MappedPtr(nullptr)
    {
    }

    explicit TCompBitIndex(const TString& indexPath)
        : DataBlob()
        , MappedPtr(NULL)
    {
        Open(indexPath);
    }

    void Open(const TString& indexPath) {
        if (MappedPtr != nullptr) {
            DataBlob.Drop();
        }

        DataBlob = TBlob::PrechargedFromFile(indexPath);

        MappedPtr = (const NImpl::TIndexNode<TSimhashType, TValueType>*)DataBlob.Data();
    }

    ~TCompBitIndex() {
        MappedPtr = nullptr;
        DataBlob.Drop();
    }

    bool IsMapped() const {
        return MappedPtr != nullptr;
    }

    template <class TCallback>
    void GetNeighbors(
        const TSimhashParam simhash,
        ui32 distance,
        TCallback& callback) const {
        if (!IsMapped()) {
            ythrow yexception() << "Index not mapped";
        }

        RecursiveWalk(0, simhash, distance, callback);
    }

private:
    template <class TCallback>
    bool RecursiveWalk(
        int nodeIndex,
        const TSimhashParam simhash,
        ui32 distance,
        TCallback& callback) const {
        const NImpl::TIndexNode<TSimhashType, TValueType>& node = MappedPtr[nodeIndex];

        ui32 prefixDistance = TTraits::HammingDistance(
            node.Simhash,
            simhash,
            node.DividingBit);

        if (prefixDistance > distance) {
            return true;
        }

        if (node.Left != NImpl::TIndexNode<TSimhashType, TValueType>::MAX_INDEX) {
            if (!RecursiveWalk(node.Left, simhash, distance, callback)) {
                return false;
            }
        }

        if (TTraits::HammingDistance(node.Simhash, simhash) <= distance) {
            if (!callback(node.Simhash, node.Data)) {
                return false;
            }
        }

        if (node.Right != NImpl::TIndexNode<TSimhashType, TValueType>::MAX_INDEX) {
            if (!RecursiveWalk(node.Right, simhash, distance, callback)) {
                return false;
            }
        }

        return true;
    }

private:
    TBlob DataBlob;
    const NImpl::TIndexNode<TSimhashType, TValueType>* MappedPtr;
};
