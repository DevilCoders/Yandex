#include "part_iterator.h"

#include <util/generic/hash.h>
#include <util/generic/algorithm.h>

namespace NRTYArchive {

    void TOffsetsIterator::DoNext() {
        while (IsValid()) {
            TPosition pos = FAT->Get(CurrentOffset->Docid);
            if (pos.GetPart() != Owner->GetPartNum() || pos.GetOffset() != CurrentOffset->Offset) {
                ++CurrentOffset;
            } else if (Slave->SkipTo(CurrentOffset->Offset)) {
                return;
            } else
                CurrentOffset = Offsets->end();
        }
    }

    bool TOffsetsIterator::IsValid() {
        return CurrentOffset != Offsets->end();
    }

    void TOffsetsIterator::Next() {
        VERIFY_WITH_LOG(IsValid(), "iterator invalid");
        ++CurrentOffset;
        DoNext();
    }

    size_t TOffsetsIterator::GetDocId() {
        VERIFY_WITH_LOG(IsValid(), "iterator invalid");
        return CurrentOffset->Docid;
    }

    TBlob TOffsetsIterator::GetDocument() {
        VERIFY_WITH_LOG(IsValid(), "iterator invalid");
        return Slave->GetDocument();
    }

    TOffsetsIterator::TOffsetsIterator(TArchivePartThreadSafe::TPtr owner, IFat* fat, const TOffsetsPtr offests)
        : IPartIterator(owner)
        , Slave(owner->CreateSlaveIterator())
        , FAT(fat)
        , Offsets(offests)
        , CurrentOffset(Offsets->begin()) {
        DoNext();
    }


    bool TRawIterator::IsValid() {
        return !Document.Empty();
    }

    void TRawIterator::Next() {
        Position++;
        CurrentOffset = Slave->SkipNext();
        Document = Slave->GetDocument();
    }

    size_t TRawIterator::GetDocId() {
        return Owner->GetHeaderData(Position);
    }

    TBlob TRawIterator::GetDocument() {
        VERIFY_WITH_LOG(IsValid(), "iterator invalid");
        return Document;
    }

    TRawIterator::TRawIterator(TArchivePartThreadSafe::TPtr owner)
        : IPartIterator(owner)
        , Slave(owner->CreateSlaveIterator())
        , CurrentOffset(0)
    {
        Slave->SkipTo(0);
        Document = Slave->GetDocument();
    }

    TOffsetsPtr TOffsetsIterator::CreateOffsets(ui64 partIndex, IFat* fat) {
        TOffsetsPtr offsets = MakeSimpleShared<TOffsets>();
        const auto snapshot = fat->GetSnapshot();
        for (size_t id = 0; id != snapshot.size(); ++id) {
            const auto pos = snapshot[id];
            if (!pos.IsRemoved() && pos.GetPart() == partIndex) {
                offsets->emplace_back(pos.GetOffset(), id, 0);
            }
        }
        StableSort(*offsets);
        return offsets;
    }

    TMap<ui32, TOffsetsPtr> TOffsetsIterator::CreateOffsets(IFat* fat) {
        TMap<ui32, TOffsetsPtr> offsetsMap;
        const auto snapshot = fat->GetSnapshot();
        for (size_t id = 0; id != snapshot.size(); ++id) {
            const auto pos = snapshot[id];
            if (pos.IsRemoved()) {
                continue;
            }
            ui32 partIndex = pos.GetPart();
            TOffsetsPtr& offsets = offsetsMap[partIndex];
            if (!offsets) {
                offsets = MakeSimpleShared<TOffsets>();
            }
            offsets->emplace_back(pos.GetOffset(), id, 0);
        }

        for (auto [_, offsets] : offsetsMap) {
            StableSort(*offsets);
        }
        return offsetsMap;
    }
}
