#pragma once

#include "position.h"

#include <util/generic/intrlist.h>
#include <util/system/sys_alloc.h>

namespace NReqBundleIteratorImpl {
    using NReqBundleIterator::TPosition;

    class THitsStorageBlock
        : public TIntrusiveSListItem<THitsStorageBlock>
    {
    private:
        size_t AllocatedHitCount;

        THitsStorageBlock(size_t count)
            : AllocatedHitCount(count)
        {}
        ~THitsStorageBlock() {}

        static size_t GetMemorySize(size_t count) {
            return sizeof(THitsStorageBlock) + count * sizeof(NReqBundleIterator::TPosition);
        }

    public:
        static THitsStorageBlock* Allocate(size_t count) {
            void* raw = y_allocate(GetMemorySize(count));
            return new(raw) THitsStorageBlock(count);
        }
        void Destroy() {
            this->~THitsStorageBlock();
            y_deallocate(this);
        }
        NReqBundleIterator::TPosition* FirstHit() const {
            return (NReqBundleIterator::TPosition*)(this + 1);
        }
        size_t NumHits() const {
            return AllocatedHitCount;
        }
    };

    class THitsStorage {
    private:
        TIntrusiveSList<THitsStorageBlock> Blocks;
        TIntrusiveSList<THitsStorageBlock>::TIterator CurBlock = nullptr;
        TPosition* FirstFree = nullptr;
        size_t LeftInCurBlock = 0;

        void InitBlock() {
            Y_ASSERT(CurBlock != Blocks.End());
            FirstFree = CurBlock->FirstHit();
            LeftInCurBlock = CurBlock->NumHits();
        }

    public:
        THitsStorage(size_t initialHitCount = 8100) {
            Blocks.PushFront(THitsStorageBlock::Allocate(initialHitCount));
        }
        ~THitsStorage() {
            while (!Blocks.Empty())
                Blocks.PopFront()->Destroy();
        }

        void Reset() {
            CurBlock = Blocks.Begin();
            InitBlock();
        }
        TPosition* Reserve(size_t count) {
            while (Y_UNLIKELY(LeftInCurBlock < count)) {
                if (CurBlock->IsEnd()) {
                    CurBlock->SetNext(THitsStorageBlock::Allocate(CurBlock->NumHits() * 2));
                }
                CurBlock.Next();
                InitBlock();
            }
            return FirstFree;
        }
        void Advance(size_t count) {
            Y_ASSERT(CurBlock != Blocks.End());
            Y_ASSERT(count <= LeftInCurBlock);
            FirstFree += count;
            LeftInCurBlock -= count;
        }
    };
} // NReqBundleIteratorImpl
