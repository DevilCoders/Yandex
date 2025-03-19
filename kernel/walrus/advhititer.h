#pragma once

#include <util/generic/vector.h>
#include <kernel/keyinv/indexfile/oldindexfile.h>
#include <kernel/keyinv/indexfile/rdkeyit.h>

#include "deletelogic.h"

typedef TVector<TVector<ui32> > TFormCounts;

struct TRemapItem {
    ui32    NewDocId;
    ui32    OutputIdx;
};

class TBitIterator {
    ui32 Val;
    ui32 Index;
public:
    explicit TBitIterator(ui32 val)
        : Val(val & 0x7FFFFFFF)
        , Index(0xFFFFFFFF)
    {
        Y_ASSERT(IsBitset(val));
    }
    //! returns index of the current bit
    ui32 Get() const {
        return Index;
    }
    //! returns true if the next bit is found
    bool Next() {
        while (Val) {
            if (Val & 0x1) {
                Index += 1;
                Val >>= 1;
                return true;
            }
            Index += 1;
            Val >>= 1;
        }
        return false;
    }
    static bool IsBitset(ui32 val) {
        return (val & 0x80000000);
    }
};

template <typename TBaseHitIterator>
class TAdvancedHitIterator : public TBaseHitIterator {
    const TVector<ui32>* RemapTable;
    SUPERLONG           CurrentHit;
    bool                UseDeleteLogic;

public:

    TAdvancedHitIterator(size_t pageSizeBits = TBufferedHitIterator::DEF_PAGE_SIZE_BITS)
        : TBaseHitIterator(pageSizeBits)
        , RemapTable(nullptr)
        , CurrentHit(0)
        , UseDeleteLogic(false)
    {}

    void SetRemapTable(const TVector<ui32>* remapTable) {
        RemapTable = remapTable;
    }

    void SetUseDeleteLogic() {
        UseDeleteLogic = true;
    }

    bool GetUseDeleteLogic() const {
        return UseDeleteLogic;
    }

    inline void CountWordForms(TFormCounts& formCounts, const TRemapItem* finalRemapTable, size_t tableSize, TAttrDeleteLogic& deleteLogic, ui32 defaultOutputIdx);

    inline bool Next();
    void operator++() { Next(); }

    inline SUPERLONG Current() const {
        return CurrentHit;
    }

    inline const SUPERLONG& operator*() const {
        return CurrentHit;
    }

    void Restart() {
        this->Rewind();
        Next();
    }

    void FillHeapData(const TAdvancedHitIterator& other) {
        TBaseHitIterator::FillHeapData(other);
        RemapTable = other.RemapTable;
        UseDeleteLogic = other.UseDeleteLogic;
    }

    using TBaseHitIterator::Restart;

    //! @note can return 0 if a portion is read or if TBufferedHitIterator::Restart() was called with memorize == false
    ui32 GetCount() const {
        return this->LastCount;
    }

    void WriteHits(NIndexerCore::TRawIndexFile& output) {
        Y_ASSERT(this->LastCount && this->LastLength);
        TTempBuf buf;
        char* const data = buf.Data();
        const ui32 size = buf.Size();
        this->InvFile->Seek(this->LastStart, SEEK_SET);
        ui32 count = this->LastCount;
        ui32 length = this->LastLength;
        while (length) {
            const ui32 readBytes = this->InvFile->Read(data, Min(length, size));
            if (readBytes <= 0) {
                Y_ASSERT(false);
                break;
            }
            output.StoreHits(data, readBytes, count);
            count = 0;
            length -= readBytes;
        }
    }
};

template <typename TBaseHitIterator>
inline bool TAdvancedHitIterator<TBaseHitIterator>::Next() {
    while (true) {
        if (!TBaseHitIterator::Next())
            return false;
        if (!RemapTable) {
            CurrentHit = TBaseHitIterator::Current();
            return true;
        }
        TWordPosition pos(TBaseHitIterator::Current());
        Y_ASSERT(pos.Doc() < RemapTable->size());
        ui32 newdocid = (*RemapTable)[pos.Doc()];
        if (newdocid != (ui32)-1) {
            pos.SetDoc(newdocid);
            CurrentHit = pos.SuperLong();
            return true;
        }
    }
}

template <typename TBaseHitIterator>
inline void TAdvancedHitIterator<TBaseHitIterator>::CountWordForms(TFormCounts& formCounts,
    const TRemapItem* finalRemapTable, size_t tableSize, TAttrDeleteLogic& deleteLogic, ui32 defaultOutputIdx)
{
    while (TBaseHitIterator::Next()) {
        TWordPosition pos(TBaseHitIterator::Current());
        if (deleteLogic.OnPos(pos.Pos, UseDeleteLogic))
            continue;
        Y_ASSERT(!RemapTable);
        const ui32 newdocid = pos.Doc();
        ui32 outputIdx;
        if (finalRemapTable) {
            outputIdx = (newdocid >= tableSize ? defaultOutputIdx : finalRemapTable[newdocid].OutputIdx);
            if (outputIdx == Max<ui32>())
                continue;
        } else
            outputIdx = 0;
        int globalForm = this->FormsMap[pos.Form()];
        if (TBitIterator::IsBitset(outputIdx)) {
            TBitIterator bi(outputIdx);
            while (bi.Next()) {
                formCounts[bi.Get()][globalForm]++;
            }
        } else {
            formCounts[outputIdx][globalForm]++;
        }
    }
}

