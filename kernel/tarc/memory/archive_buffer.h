#pragma once

#include "archive.h"

#include <util/system/rwlock.h>

#include <array>

class TMemoryArchiveBuffer : public IMemoryArchive {
private:
    class TDocGuard;
    class TDoc
        : public TBuffer
        , public TAtomicRefCount<TDoc>
    {
    };
    using TDocPtr = TIntrusivePtr<TDoc>;
    using TLock = TRWMutex;

private:
    TVector<TDocPtr> DocBuffers;
    ui32 CurrentDocId = static_cast<ui32>(-1);

    mutable std::array<TLock, 32> Locks;

public:
    TMemoryArchiveBuffer(ui32 maxDocs);
    ~TMemoryArchiveBuffer() override;

    void AddDoc(ui32 docId) override;
    void DoWrite(const void* data, size_t size) override;
    void EraseDoc(ui32 docId) override;
    bool HasDoc(ui32 docId) const override;
    IMemoryArchiveDocGuardPtr GuardDoc(ui32 docId) const override;

private:
    TBlob GetDocBlob(ui32 docId) const;
    TDocPtr GetDoc(ui32 docId) const;
    void SetDoc(ui32 docId, TDocPtr doc);
protected:
    void GetExtInfoAndDocText(ui32 docId, TBlob& extInfoBlob, TBlob& docTextBlob, bool getExtInfo, bool getDocText) const override;
};
