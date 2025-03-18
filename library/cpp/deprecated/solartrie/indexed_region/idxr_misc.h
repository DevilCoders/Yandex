#pragma once

#include "idxr_big.h"
#include "idxr_small.h"

namespace NIndexedRegion {
    class TCompoundIndexedRegion {
    protected:
        typedef TSimpleSharedPtr<TBigIndexedRegion> TRaPtr;

        ui64 BlockSize;

        NPagedVector::TPagedVector<TBuffer> Blocks;
        NPagedVector::TPagedVector<TRaPtr> BlocksRA;
        ui64 Count = 0;

        TBuffer Buffer;

        mutable ui64 PrevBlockIndex = -1;
        mutable ui64 CurrBlockIndex = -1;

        mutable TSimpleSharedPtr<TSmallIndexedRegion<ui32>> PrevBlock;
        mutable TSimpleSharedPtr<TSmallIndexedRegion<ui32>> CurrBlock;

        bool RandomAccess;
        bool Building = true;

    public:
        typedef NPrivate::TConstIterator<TCompoundIndexedRegion> TConstIterator;

    public:
        TCompoundIndexedRegion(bool ra, size_t blocksz, TCodecPtr lencodec, TCodecPtr blockcodec)
            : BlockSize(blocksz)
            , PrevBlock(new TSmallIndexedRegion<ui32>(lencodec, blockcodec))
            , CurrBlock(new TSmallIndexedRegion<ui32>(lencodec, blockcodec))
            , RandomAccess(ra)
        {
            Y_VERIFY(BlockSize, " ");
            BlocksRA.push_back(new TBigIndexedRegion);
        }

        void Swap(TCompoundIndexedRegion& r) {
            DoSwap(BlockSize, r.BlockSize);
            DoSwap(Blocks, r.Blocks);
            DoSwap(BlocksRA, r.BlocksRA);
            DoSwap(Count, r.Count);
            DoSwap(Buffer, r.Buffer);
            DoSwap(PrevBlockIndex, r.PrevBlockIndex);
            DoSwap(CurrBlockIndex, r.CurrBlockIndex);
            DoSwap(PrevBlock, r.PrevBlock);
            DoSwap(CurrBlock, r.CurrBlock);
            DoSwap(RandomAccess, r.RandomAccess);
            DoSwap(Building, r.Building);
        }

        void DisposePreviousBlocks(size_t idx) {
            Y_VERIFY(!Building, " ");
            Y_VERIFY(idx < Count, " ");
            const size_t bidx = idx / BlockSize;
            const size_t iidx = idx % BlockSize;

            if (!iidx && bidx) {
                if (RandomAccess) {
                    BlocksRA[bidx - 1].Reset(nullptr);
                } else {
                    Blocks[bidx - 1].Reset();
                }
            }
        }

        TStringBuf Get(size_t idx) const {
            Y_VERIFY(!Building, " ");
            Y_VERIFY(idx < Count, " ");
            const size_t bidx = idx / BlockSize;
            const size_t iidx = idx % BlockSize;

            if (RandomAccess) {
                return BlocksRA[bidx]->Get(iidx);
            } else {
                if (CurrBlockIndex != bidx) {
                    DoSwap(CurrBlock, PrevBlock);
                    DoSwap(CurrBlockIndex, PrevBlockIndex);
                    if (bidx != CurrBlockIndex) {
                        CurrBlockIndex = bidx;
                        CurrBlock->Clear();
                        const auto& b = Blocks[CurrBlockIndex];
                        CurrBlock->Decode(TStringBuf{b.data(), b.size()});
                    }
                }

                return CurrBlock->Get(iidx);
            }
        }

        TStringBuf operator[](size_t idx) const {
            return Get(idx);
        }

        size_t Size() const {
            return Count;
        }

        bool Empty() const {
            return !Size();
        }

        TStringBuf PushBack(TStringBuf r) {
            Building = true;
            CheckBlock();
            ++Count;
            if (RandomAccess) {
                return BlocksRA.back()->PushBack(r);
            } else {
                return CurrBlock->PushBack(r);
            }
        }

        TStringBuf PushBackReserve(size_t sz) {
            Buffer.Clear();
            Buffer.Resize(sz);
            memset(Buffer.Data(), 0, Buffer.Size());
            return PushBack(TStringBuf{Buffer.data(), Buffer.size()});
        }

        void Commit() {
            Building = false;
            FlushBlock();
            Buffer.Reset();
        }

        void Clear() {
            BlocksRA.clear();
            BlocksRA.push_back(new TBigIndexedRegion);
            Blocks.clear();
            CurrBlock->Clear();
            PrevBlock->Clear();
            Buffer.Reset();
            CurrBlockIndex = -1;
            PrevBlockIndex = -1;
            Count = 0;
        }

        void FlushBlock() {
            if (RandomAccess) {
                if (!BlocksRA.back()->Empty()) {
                    BlocksRA.back()->Commit();
                    BlocksRA.push_back(new TBigIndexedRegion);
                }
            } else {
                if (!CurrBlock->Empty()) {
                    CurrBlock->Commit();
                    Blocks.emplace_back();
                    CurrBlock->Encode(Blocks.back());
                    CurrBlock->Clear();
                }
            }
        }

        void CheckBlock() {
            if (RandomAccess) {
                if (BlocksRA.back()->Size() == BlockSize) {
                    FlushBlock();
                }
            } else {
                if (CurrBlock->Size() == BlockSize) {
                    FlushBlock();
                }
            }
        }

        TConstIterator Begin() const {
            return TConstIterator(this, 0);
        }

        TConstIterator End() const {
            return TConstIterator(this, Size());
        }
    };

}
