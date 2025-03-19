#pragma once

#include <kernel/multipart_archive/archive_impl/part_iterator.h>

namespace NRTYArchive {
    typedef TVector<TOffsetAndDocid> TOffsets;
    typedef TSimpleSharedPtr<TOffsets> TOffsetsPtr;

    class TOffsetsIterator : public IPartIterator {
    public:
        TOffsetsIterator(TArchivePartThreadSafe::TPtr owner, IFat* fat, const TOffsetsPtr offests);
        TBlob GetDocument() override;
        size_t GetDocId() override;
        void Next() override;
        bool IsValid() override;

        static TOffsetsPtr CreateOffsets(ui64 partIndex, IFat* fat);
        static TMap<ui32, TOffsetsPtr> CreateOffsets(IFat* fat);

    protected:
        virtual void DoNext();

    protected:
        IArchivePart::IIterator::TPtr Slave;
        mutable IFat* FAT;
        const TOffsetsPtr Offsets;
        TOffsets::const_iterator CurrentOffset;
    };

    class TRawIterator : public IPartIterator {
    public:
        TRawIterator(TArchivePartThreadSafe::TPtr owner);
        TBlob GetDocument() override;
        size_t GetDocId() override;
        void Next() override;
        bool IsValid() override;

    private:
        IArchivePart::IIterator::TPtr Slave;
        TBlob Document;
        ui64 CurrentOffset;
    };
}
