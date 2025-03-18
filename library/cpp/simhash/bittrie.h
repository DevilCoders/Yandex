#pragma once

#include "math_util.h"

#include <util/generic/vector.h>
#include <util/generic/noncopyable.h>
#include <util/generic/yexception.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/utility.h>

#include <util/system/yassert.h>
#include <util/system/defaults.h>

#include <util/memory/alloc.h>

#include <memory.h>

template <bool B>
struct TTruthGuard;

template <>
struct TTruthGuard<true> {};

#define NODE_POINTER_COUNT (1 << ChunkSize)
#define LEVEL_COUNT (sizeof(TKey) * 8 / ChunkSize + ((sizeof(TKey) * 8 % ChunkSize) ? 1 : 0))

template <typename TValue>
struct TValueGetter {
    TValue Value;

    TValueGetter(const TValue& value)
        : Value(value)
    {
    }
};

template <class TParent, size_t LevelCount, size_t NodePointerCount>
class TBitTrieIterator;

template <
    class TKey,
    class TValue,
    ui32 ChunkSize = 2,
    class TAllocator = std::allocator<TValue>>
class TBitTrie
   : private TTruthGuard<(ChunkSize > 0 && ChunkSize <= 8)>,
      private TNonCopyable {
    friend class TBitTrieIterator<TBitTrie, LEVEL_COUNT, NODE_POINTER_COUNT>;
    friend class TBitTrieIterator<const TBitTrie, LEVEL_COUNT, NODE_POINTER_COUNT>;

private:
    static Y_FORCE_INLINE ui32 GetBitCount() {
        return sizeof(TKey) * 8;
    }
    static Y_FORCE_INLINE ui32 GetChunkSize() {
        return ChunkSize;
    }

    static Y_FORCE_INLINE ui32 GetNodePointerCount() {
        return NODE_POINTER_COUNT;
    }
    static Y_FORCE_INLINE ui32 GetLevelCount() {
        return LEVEL_COUNT;
    }

private:
    struct TMiddleNode;
    struct TLeafNode;

public:
    typedef TKey TKeyType;
    typedef TValue TValueType;

    typedef TBitTrieIterator<TBitTrie, LEVEL_COUNT, NODE_POINTER_COUNT> TIt;
    typedef TBitTrieIterator<const TBitTrie, LEVEL_COUNT, NODE_POINTER_COUNT> TConstIt;

    typedef TReboundAllocator<TAllocator, TMiddleNode> TMiddleNodeAllocator;
    typedef TReboundAllocator<TAllocator, TLeafNode> TLeafNodeAllocator;

public:
    TBitTrie()
        : root()
        , middleNodeAllocator()
        , leafNodeAllocator()
        , count(0)
    {
    }

    ~TBitTrie() {
        BFS(TDestructor::NodeProcessor);
    }

    // Common interface
public:
    TConstIt CMin() const;
    TConstIt CMax() const;
    TIt Min();
    TIt Max();
    TConstIt Find(TKey Key) const;
    TIt Find(TKey Key);

    ui32 GetCount() const {
        return count;
    }
    bool Insert(
        TKey Key,
        const TValue& Value,
        TValue** OldValue = nullptr);
    bool Has(TKey Key) const;
    const TValue& Get(TKey Key) const;
    TValue& Get(TKey Key, bool Create = false);
    bool TryGet(TKey Key, TValue** Value) const;
    bool Delete(TKey Key, TValue* Value = nullptr);
    void Clear();

    // Special interface
public:
    template <class TRes>
    void GetNeighbors(
        TKey Key,
        ui32 Distance,
        TVector<TRes>* Keys) const;

    size_t GetNeighborCount(
        TKey Key,
        ui32 Distance) const;

    void CountNeighbors(
        THashMap<TKey, std::pair<TValue*, size_t>>& data,
        ui32 distance) const;

    // Base stuff
private:
    void SplitBitKey(
        TKey Key,
        ui8 LevelKeys[LEVEL_COUNT]) const;
    ui32 GetNodePath(
        ui8 LevelKeys[LEVEL_COUNT],
        void* Nodes[LEVEL_COUNT + 1]) const;
    void GetMinNodePath(
        ui8 LevelKeys[LEVEL_COUNT],
        void* Nodes[LEVEL_COUNT + 1]) const;
    void GetMaxNodePath(
        ui8 LevelKeys[LEVEL_COUNT],
        void* Nodes[LEVEL_COUNT + 1]) const;
    std::pair<TLeafNode*, bool> MakePath(ui8 LevelKeys[LEVEL_COUNT]);
    TLeafNode* GetPath(ui8 LevelKeys[LEVEL_COUNT]) const;
    bool DeletePath(
        ui8 LevelKeys[LEVEL_COUNT],
        void* Nodes[LEVEL_COUNT + 1]);

    // Algorithms
private:
    struct TNeighbors {
        struct TNodeContext {
            ui8 Distance;

            TNodeContext(ui8 D)
                : Distance(D)
            {
            }
        };

        template <class TRes>
        struct TBFSContext {
            ui32 Distance;
            ui8 LevelKeys[LEVEL_COUNT];
            TVector<TRes>* Keys;

            TBFSContext()
                : Distance(0)
                , Keys(nullptr)
            {
                memset(LevelKeys, 0, sizeof(ui8) * GetLevelCount());
            }
        };

        template <class TRes>
        static void NodeProcessor(
            ui32 LevelIndex,
            const std::pair<const TMiddleNode*, TNodeContext>& Node,
            TVector<std::pair<const TMiddleNode*, TNodeContext>>* NextLevel,
            TBFSContext<TRes>* BFSContext);
    };

    struct TNeighborCount {
        struct TNodeContext {
            ui8 Distance;

            TNodeContext(ui8 D)
                : Distance(D)
            {
            }
        };

        struct TBFSContext {
            ui32 Distance;
            ui8 LevelKeys[LEVEL_COUNT];
            size_t Count;

            TBFSContext()
                : Distance(0)
                , Count(0)
            {
                memset(LevelKeys, 0, sizeof(ui8) * GetLevelCount());
            }
        };

        static void NodeProcessor(
            ui32 LevelIndex,
            const std::pair<const TMiddleNode*, TNodeContext>& Node,
            TVector<std::pair<const TMiddleNode*, TNodeContext>>* NextLevel,
            TBFSContext* BFSContext);
    };

    struct TCountNeighbors {
        struct TNodeContext {
            ui8 Distance;
            ui8 Compare;

            TNodeContext(ui8 distance, ui8 compare)
                : Distance(distance)
                , Compare(compare)
            {
            }
        };

        struct TBFSContext {
            ui32 Distance;
            ui8 LevelKeys[LEVEL_COUNT];
            TVector<std::pair<TKey, TValue*>>& Group;

            TBFSContext(TVector<std::pair<TKey, TValue*>>& group)
                : Distance(0)
                , Group(group)
            {
                memset(LevelKeys, 0, sizeof(ui8) * GetLevelCount());
            }
        };

        static void NodeProcessor(
            ui32 LevelIndex,
            const std::pair<const TMiddleNode*, TNodeContext>& Node,
            TVector<std::pair<const TMiddleNode*, TNodeContext>>* NextLevel,
            TBFSContext* BFSContext);
    };

    struct TDestructor {
        static void NodeProcessor(
            TBitTrie<TKey, TValue, ChunkSize, TAllocator>* parent,
            ui32 levelIndex,
            TMiddleNode* node,
            TVector<TMiddleNode*>* nextLevel);
    };

    template <class TNodeProcessor, class TNodeContext, class TBFSContext>
    void BFS(TNodeProcessor Processor, const TNodeContext& RootContext, TBFSContext* BFSContext) const;

    template <class TNodeProcessor>
    void BFS(TNodeProcessor Processor);

    // Memory management
private:
    TMiddleNode* CreateMiddleNode();
    void DeleteMiddleNode(TMiddleNode* N);
    TLeafNode* CreateLeafNode();
    void DeleteLeafNode(TLeafNode* N);

private:
    TMiddleNode root;

    TMiddleNodeAllocator middleNodeAllocator;
    TLeafNodeAllocator leafNodeAllocator;

    ui32 count;
};

////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////

#include "bittrie.types.h"
#include "bittrie.common_interface.h"
#include "bittrie.special_interface.h"
#include "bittrie.base_stuff.h"
#include "bittrie.algorithms.h"
#include "bittrie.memory_management.h"

#undef NODE_POINTER_COUNT
#undef LEVEL_COUNT
