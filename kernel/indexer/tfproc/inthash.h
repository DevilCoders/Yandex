#pragma once

#include <util/memory/pool.h>
#include <util/generic/ptr.h>
#include <utility>

// Specialized hash ui32 -> TValue
/* Предположения для работы:
    1. Ключи - целые числа, младшие биты которых достаточно случайны
        (распределение достаточно равномерное).
    2. Происходит только добавление данных и изменения значений, но не
        удаление ключей; очистка - одна в конце жизни объекта.
    3. Не происходит повторной вставки уже существующих элементов
        (вызов insert(...) происходит только в случае find(...) == end()).
    4. Число корзин для хранимых объектов N_BUCKETS есть степень двойки. */
template<class TValue, size_t N_BUCKETS>
class TIntHash
{
public:
    typedef std::pair<ui32, TValue> TValueType;
private:
    //static const size_t N_BUCKETS = 0x1000; /* must be power of 2 */
    struct TNode
    {
        TNode* Next;
        TValueType Value;
    };
    TArrayHolder<TNode*> Buckets;
    int FirstUsedBucket;
    TArrayHolder<int> NextUsedBucket;
    size_t Size;
    friend class iterator;
public:
    typedef TMemoryPool TPoolType;
private:
    TPoolType* Pool;
public:
    TIntHash(TPoolType* pool)
        : FirstUsedBucket(-1)
        , Size(0)
        , Pool(pool)
    {
    }
    ~TIntHash()
    {
    }
    class iterator
    {
        TNode* Node;
        TIntHash* Parent; // needed for operator++
    public:
        iterator() : Node(nullptr), Parent(nullptr) {}
        iterator(TNode* n, TIntHash* p) : Node(n), Parent(p) {}
        TValueType* operator->()
        { return &Node->Value; }
        bool operator==(iterator right)
        { return Node == right.Node; }
        bool operator!=(iterator right)
        { return Node != right.Node; }
        iterator& operator++()
        {
            if (Node->Next)
                Node = Node->Next;
            else {
                ui32 bucket = Node->Value.first & (N_BUCKETS - 1);
                int next = Parent->NextUsedBucket.Get()[bucket];
                if (next == -1)
                    Node = nullptr;
                else
                    Node = Parent->Buckets.Get()[next];
            }
            return *this;
        }
        iterator operator++(int)
        { iterator old = *this; ++*this; return old; }
    };
    iterator begin()
    { return iterator(Size ? Buckets.Get()[FirstUsedBucket] : nullptr, this); }
    iterator end()
    { return iterator(nullptr, nullptr); }
    iterator find(ui32 key)
    {
        if (!Size)
            return iterator(nullptr, nullptr);
        ui32 bucket = key & (N_BUCKETS - 1);
        TNode* node = Buckets.Get()[bucket];
        while (node) {
            if (node->Value.first == key) {
                return iterator(node, this);
            }
            node = node->Next;
        }
        return iterator(nullptr, nullptr);
    }
    // assumes that find(key) has been already called and returned end()
    void insert(TValueType value)
    {
        if (!Size) {
            // delayed initialization
            Buckets.Reset(new TNode*[N_BUCKETS]);
            memset(Buckets.Get(), 0, N_BUCKETS * sizeof(TNode*));
            NextUsedBucket.Reset(new int[N_BUCKETS]);
        }
        TNode** bucketsArr = Buckets.Get();
        ui32 bucket = value.first & (N_BUCKETS - 1);
        TNode* newNode = (TNode*)Pool->Allocate(sizeof(TNode));
        newNode->Value = value;
        if (!bucketsArr[bucket]) {
            NextUsedBucket.Get()[bucket] = FirstUsedBucket;
            FirstUsedBucket = bucket;
        }
        newNode->Next = bucketsArr[bucket];
        bucketsArr[bucket] = newNode;
        Size++;
    }
    size_t size() const
    { return Size; }
};
