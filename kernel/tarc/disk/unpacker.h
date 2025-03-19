#pragma once

#include <kernel/multipart_archive/multipart.h>
#include <kernel/multipart_archive/config/config.h>
#include <kernel/tarc/enums/searcharc.h>
#include <kernel/tarc/iface/tarcface.h>
#include <kernel/tarc/iface/settings.h>

#include <library/cpp/charset/wide.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/system/defaults.h>
#include <util/generic/vector.h>
#include <util/system/filemap.h>
#include <util/system/file.h>
#include <util/memory/blob.h>
#include <util/generic/typetraits.h>
#include <util/generic/maybe.h>

using TMultipartArchivePtr = TArchiveOwner::TPtr;

class TUnpackDocCtx {
public:
    TUnpackDocCtx() {
        Offset = FAIL_DOC_OFFSET;
    }

    void Set(const TMemoryMap& arcFile, ui64 offset);
    void Set(const TFile& arcFile, ui64 offset);
    void Set(const TBlob& arcFile, ui64 offset);
    void Set(const TMultipartArchivePtr& arcFile, ui64 docid);

    const TArchiveHeader& Get() const {
        Y_ASSERT(Offset != FAIL_DOC_OFFSET);
        return Header;
    }

    // Have different meaning for multipart archive.
    ui64 GetOffset() const {
        return Offset;
    }

private:
    TArchiveHeader Header;
    ui64 Offset;
};

class IArchive {
public:
    virtual void GetFirstDocCtx(TUnpackDocCtx* ctx) const = 0;
    virtual void GetNextDocHeader(TUnpackDocCtx* ctx) const = 0;
    virtual void GetDocHeader(i64 offset, TUnpackDocCtx* ctx) const = 0;
    virtual TBlob GetDocFullInfo(const TUnpackDocCtx* ctx) const = 0;
    virtual TBlob GetExtInfo(const TUnpackDocCtx& ctx) const = 0;
    virtual TBlob GetDocText(const TUnpackDocCtx& ctx) const = 0;
    virtual ~IArchive() {}

public:
    struct TFactoryKey {
        EArchiveType ArchiveType;
        EArchiveOpenMode OpenMode;

        bool operator<(const TFactoryKey& key) const {
            return (ui32(ArchiveType) < (ui32)key.ArchiveType) ||
                   (ui32(ArchiveType) == (ui32)key.ArchiveType && ui32(OpenMode) < (ui32)key.OpenMode);
        }
    };

    struct TCreationContext {
        TString ArchiveName;
        TString ArchiveDir;
        TBuffer ArchiveHeader;
        bool IsFileReuse = true;
        bool IsFlatCompatible = false;
        const NRTYArchive::TMultipartConfig& MultipartConfig;

        TCreationContext(const NRTYArchive::TMultipartConfig& config)
            : MultipartConfig(config)
        {}
    };

    using TFactory = NObjectFactory::TParametrizedObjectFactory<IArchive, TFactoryKey, TCreationContext>;
};

class IArchiveDir {
public:
    virtual void PreCharge() = 0;
    virtual ui32 Size() const = 0;
    virtual bool Empty() const = 0;
    virtual bool HasDoc(ui32 docId) const = 0;
    virtual ui64 operator[](ui32 docId) const = 0;

    virtual ~IArchiveDir() {}

public:
    struct TCreationContext {
        TString ArchiveDir;
        TString ArchiveText;
        bool UseMapping = false;
        bool LockMemory = false;
    };

    using TFactory = NObjectFactory::TParametrizedObjectFactory<IArchiveDir, EArchiveType, TCreationContext>;
};

template<class T>
TBlob FromArchive(const T& arcFileType, ui64 docOffset, ui64 dataOffset, size_t length);

template<class T>
class TArchive: public IArchive {
public:
    TArchive(typename TTypeTraits<T>::TFuncParam arcFile, const TBuffer& archiveHeader)
        : ArcFile(arcFile)
    {
        if (archiveHeader.Empty()) {
            ArchiveVersion = ARCVERSION;
        } else {
            TMemoryInput tmp(archiveHeader.Data(), archiveHeader.Size());
            CheckTextArchiveHeader(tmp, ArchiveVersion);
        }
    }

    TArchive(typename TTypeTraits<T>::TFuncParam arcFile, bool hasTextArchiveHeader = true)
        : ArcFile(arcFile)
    {
        if (hasTextArchiveHeader) {
            TBlob arcHdr = FromArchive(ArcFile, 0, 0, 8);
            TMemoryInput tmp(arcHdr.Data(), arcHdr.Size());
            CheckTextArchiveHeader(tmp, ArchiveVersion);
        } else {
            ArchiveVersion = ARCVERSION;
        }
    }

    void GetFirstDocCtx(TUnpackDocCtx* ctx) const override {
        ctx->Set(ArcFile, 8);
    }

    void GetNextDocHeader(TUnpackDocCtx* ctx) const override {
        GetDocHeader(ctx->GetOffset() + ctx->Get().DocLen, ctx);
    }

    TBlob GetDocFullInfo(const TUnpackDocCtx* ctx) const override {
        return FromArchive(ArcFile, ctx->GetOffset(), 0, ctx->Get().DocLen);
    }

    void GetDocHeader(i64 offset, TUnpackDocCtx* ctx) const override {
        ctx->Set(ArcFile, offset);
    }

    TBlob GetExtInfo(const TUnpackDocCtx& ctx) const override {
        if (ctx.Get().ExtLen) {
            i64 offset = sizeof(TArchiveHeader);
            TBlob rawExtInfo = FromArchive(ArcFile, ctx.GetOffset(), offset, ctx.Get().ExtLen);
            return UnpackRawExtInfo(rawExtInfo, ArchiveVersion);
        }
        return TBlob();
    }

    TBlob GetDocText(const TUnpackDocCtx& ctx) const override {
        ui32 textLen = ctx.Get().GetTextLen();
        if (textLen) {
            i64 offset = sizeof(TArchiveHeader) + ctx.Get().ExtLen;
            return FromArchive(ArcFile, ctx.GetOffset(), offset, textLen);
        }
        return TBlob();
    }

    T& GetArcFile() {
        return ArcFile;
    }
protected:
    T ArcFile;
private:
    ui32 ArchiveVersion;
};


template<class T> inline void GetDocHeader(ui32 docId, const T& arc, const IArchiveDir& dir, TUnpackDocCtx* ctx) {
    if (docId >= dir.Size())
        ythrow yexception() << "docId " <<  docId << " > maxDocId=" <<  (unsigned long)(dir.Size() - 1);
    ui64 offset = dir[docId];
    arc.GetDocHeader(offset, ctx);
    if (ctx->GetOffset() != offset || (ctx->Get().DocId != docId && Singleton<TArchiveSettings>()->DocIdCheckerEnabled))
        ythrow yexception() << "bad docId " <<  docId;
}
