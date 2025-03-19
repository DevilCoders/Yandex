#pragma once

#include "part_thread_safe.h"

#include <kernel/multipart_archive/abstract/globals.h>


namespace NRTYArchive {

    struct TDocInfo {
        ui32 PartNum;
        ui64 NumInPart;

        TDocInfo(ui32 partNum, ui64 posInPart)
            : PartNum(partNum)
            , NumInPart(posInPart) {
        }

        ui32 GetPartIdx() const {
            return PartNum;
        }

        ui32 GetDocIdx() const {
            return NumInPart;
        }
    };

    class IPartIterator: public TAtomicRefCount<IPartIterator> {
    public:
        typedef TIntrusivePtr<IPartIterator> TPtr;

        using TDocInfo = NRTYArchive::TDocInfo;

        IPartIterator(TArchivePartThreadSafe::TPtr owner)
            : Owner(owner)
            , Position(0) {}

        virtual ~IPartIterator() {}

        virtual TDocInfo GetDocumentInfo() {
            return TDocInfo(Owner->GetPartNum(), Position);
        }

        virtual TBlob GetDocument() = 0;
        virtual size_t GetDocId() = 0;
        virtual void Next() = 0;
        virtual bool IsValid() = 0;

        virtual bool IsFullBlock() const {
            return false;
        }
        virtual void NextBlock() {
            FAIL_LOG("invalid usage");
        }

    protected:
        TArchivePartThreadSafe::TPtr Owner;
    protected:
        ui32 Position;
    };
}
