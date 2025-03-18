#pragma once

#include <utility>
#include <util/memory/alloc.h>

template <class TNode, class TBaseAllocator>
class TNodeAllocator {
public:
    using TAllocator = TReboundAllocator<TBaseAllocator, TNode>;

    inline TNodeAllocator() = default;

    inline TNodeAllocator(const TNodeAllocator& right)
        : Allocator_(right.Allocator_)
    {
    }

    inline TNodeAllocator(TNodeAllocator&& right) noexcept
        : Allocator_(std::move(right.Allocator_))
    {
    }

    template <class TAllocParam>
    inline TNodeAllocator(TAllocParam* allocParam)
        : Allocator_(allocParam)
    {
    }

    inline void Swap(TNodeAllocator& right) noexcept {
        DoSwap(Allocator_, right.Allocator_);
    }

public:
    inline TNode* AllocNode() {
        return Allocator_.allocate(1);
    }

    inline void DeallocNode(TNode* node) {
        Allocator_.deallocate(node, 1);
    }

    template <class TArg>
    inline TNode* NewNode(const TArg& arg) {
        TNode* const ptr = AllocNode();

        try {
            return new (ptr) TNode(arg);
        } catch (...) {
            DeallocNode(ptr);
            throw;
        }
    }

    inline void DeleteNode(TNode* node) {
        node->~TNode();
        DeallocNode(node);
    }

public:
    inline TAllocator GetAllocator() const noexcept {
        return Allocator_;
    }

private:
    TAllocator Allocator_;
};
