#pragma once

#include <kernel/tarc/disk/unpacker.h>
#include <kernel/tarc/docdescr/docdescr.h>

#include <util/memory/blob.h>
#include <util/stream/mem.h>
#include <util/system/file.h>

struct IMemoryArchiveDocGuard {
    virtual ~IMemoryArchiveDocGuard() noexcept {}
};

using IMemoryArchiveDocGuardPtr = THolder<IMemoryArchiveDocGuard>;

class IMemoryArchiveGuarder {
public:
    virtual ~IMemoryArchiveGuarder() {
    }
    virtual IMemoryArchiveDocGuardPtr GuardDoc(ui32 docId) const = 0;
};

class IMemoryArchive
    : public IOutputStream
    , public IMemoryArchiveGuarder
{
protected:
    ui32 MaxDocs;
public:
    IMemoryArchive(ui32 maxDocs)
        : MaxDocs(maxDocs)
    { }

    ~IMemoryArchive() override {
    }

    virtual void AddDoc(ui32 docId) = 0;
    int FindDoc(ui32 docId, TVector<int>& breaks, TDocArchive& da) const;
    int FindDocBlob(ui32 docId, TDocArchive& da) const;
    TBlob GetDocText(ui32 docId) const {
        TBlob extInfo, docText;
        GetExtInfoAndDocText(docId, extInfo, docText, /*getExtInfo=*/false, /*getDocText=*/true);
        return docText;
    }
    TBlob GetExtInfo(ui32 docId) const {
        TBlob extInfo, docText;
        GetExtInfoAndDocText(docId, extInfo, docText, /*getExtInfo=*/true, /*getDocText=*/false);
        return extInfo;
    }
    void DoWrite(const void* data, size_t size) override = 0;
    virtual void EraseDoc(ui32 docId) = 0;
    virtual bool HasDoc(ui32 docId) const = 0;

protected:
    virtual void GetExtInfoAndDocText(ui32 docId, TBlob& extInfoBlob, TBlob& docTextBlob, bool getExtInfo = true, bool getDocText = true) const = 0;
    static inline void GetExtInfoAndDocText(IArchive& archive, TUnpackDocCtx& ctx, TBlob& extInfoBlob, TBlob& docTextBlob, bool getExtInfo, bool getDocText) {
        if (ctx.GetOffset() == FAIL_DOC_OFFSET)
            return;
        if (getExtInfo)
            extInfoBlob = archive.GetExtInfo(ctx);
        if (getDocText)
            docTextBlob = archive.GetDocText(ctx);
    }
};
