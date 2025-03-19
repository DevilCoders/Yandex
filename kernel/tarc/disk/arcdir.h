#pragma once

#include "unpacker.h"

#include <kernel/multipart_archive/multipart.h>

class TArchiveDir : private TNonCopyable, public IArchiveDir {
public:
    TArchiveDir(const IArchiveDir::TCreationContext& ctx);

    TArchiveDir(const char* dir);

    TArchiveDir(const ui64 * const base, const ui32 size);

    void PreCharge() override;

    ui32 Size() const override;

    bool Empty() const override;

    bool HasDoc(ui32 docId) const override;

    ui64 operator[](ui32 docId) const override;

    static IArchiveDir::TFactory::TRegistrator<TArchiveDir> Registrator;

private:
    void DoInit();
    TBlob Blob;
    const ui64* Base;
    ui32 Len;
};

class TMultipartDir : private TNonCopyable, public IArchiveDir {
private:
    using TConstructContext = NRTYArchive::IArchivePart::TConstructContext;
    using TCompression = NRTYArchive::IArchivePart::TType;
public:
    TMultipartDir(const IArchiveDir::TCreationContext& ctx);

    void PreCharge() override;

    ui32 Size() const override;

    bool Empty() const override;

    bool HasDoc(ui32 docId) const override;

    ui64 operator[](ui32 docId) const override;

    static IArchiveDir::TFactory::TRegistrator<TMultipartDir> Registrator;

private:
    TMultipartArchivePtr Archive;
    ui32 Len;
    ui32 DocsCount;
};

