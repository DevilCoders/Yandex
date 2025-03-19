#include "searcharc.h"
#include "archives.h"

#include <kernel/multipart_archive/config/config.h>
#include <kernel/tarc/docdescr/docdescr.h>
#include <kernel/tarc/iface/farcface.h>
#include <kernel/tarc/markup_zones/searcharc_common.h>
#include <kernel/tarc/markup_zones/text_markup.h>

#include <library/cpp/object_factory/object_factory.h>

#include <util/folder/dirut.h>
#include <util/generic/algorithm.h>
#include <util/generic/set.h>
#include <util/generic/vector.h>


TSearchArchive::TSearchArchive() {
}

TSearchArchive::~TSearchArchive() {
}

static TBuffer ReadArchiveHeader(const TString& arcFile, EArchiveType arcType) {
    TBuffer header;
    if (arcType == AT_MULTIPART) {
        return header;
    }
    Y_ASSERT(arcType == AT_FLAT);
    TFileMap inp(arcFile, TMemoryMapCommon::oRdOnly | TMemoryMapCommon::oNotGreedy);
    inp.Map(0, 8);
    header.Assign((char*)inp.Ptr(), inp.MappedSize());
    return header;
}

void TSearchArchive::Open(const TString& index, EArchiveOpenMode mode, EArchiveType arcType,
                                       size_t cacheSize, size_t replica, size_t repCoef,
                                       const NRTYArchive::TMultipartConfig& multipartConfig,
                                       bool useMapping,
                                       bool lockMemory) {

    IArchive::TCreationContext arcCtx(multipartConfig);
    arcCtx.ArchiveName = index + "arc";
    arcCtx.ArchiveDir = (arcType == AT_FLAT) ? index + "dir" : "";
    arcCtx.IsFlatCompatible = (arcType == AT_FLAT);
    arcCtx.ArchiveHeader = ReadArchiveHeader(arcCtx.ArchiveName, arcType);
    arcCtx.IsFileReuse = (mode != AOM_FILE_NO_REUSE);

    ArchiveText.Reset(IArchive::TFactory::Construct({arcType, mode}, arcCtx));
    Y_ASSERT(!!ArchiveText.Get());

    IArchiveDir::TCreationContext dirCtx;
    dirCtx.ArchiveDir = index + "dir";
    dirCtx.ArchiveText = arcCtx.ArchiveName;
    dirCtx.UseMapping = useMapping;
    dirCtx.LockMemory = lockMemory;

    ArchiveDir.Reset(IArchiveDir::TFactory::Construct(arcType, dirCtx));

    // Turn off caching
    Y_UNUSED(cacheSize);
    Y_UNUSED(replica);
    Y_UNUSED(repCoef);
}

ui32 TSearchArchive::GetMaxHndl() const {
    return ui32(ArchiveDir->Size() - 1);
}

IArchive* TSearchArchive::ReadDocHeader(ui32 docId, TUnpackDocCtx* ctx) const {
    GetDocHeader(docId, *ArchiveText.Get(), *ArchiveDir.Get(), ctx);
    return ArchiveText.Get();
}

TBlob TSearchArchive::GetDocFullInfo(ui32 docId) const {
    if (IsOpen()) {
        TUnpackDocCtx ctx;
        return ReadDocHeader(docId, &ctx)->GetDocFullInfo(&ctx);
    }
    else {
        ythrow yexception() << "Incorrect GetDocFullInfo usage";
    }
}

int TSearchArchive::FindDocBlob(ui32 docId, TDocArchive& da) const {
    if (IsOpen()) {
        try {
            TUnpackDocCtx ctx;
            TBlob b = ReadDocHeader(docId, &ctx)->GetExtInfo(ctx);
            da.Blobsize = (int)b.Size();
            memcpy(da.GetBlob(), b.Data(), b.Size());
            da.Exists = true;
        } catch (...) {
            da.Clear();
            return -1;
        }
        return 0;
    }
    return -1;
}

TBlob TSearchArchive::GetDocText(ui32 docId) const {
    if (IsOpen()) {
        TUnpackDocCtx ctx;
        return ReadDocHeader(docId, &ctx)->GetDocText(ctx);
    }
    return TBlob();
}

TBlob TSearchArchive::GetBertEmbedding(ui32 docId) const {
    Y_UNUSED(docId);
    ythrow yexception() << "GetBertEmbedding is not implemented in TSearchArchive";
    return TBlob();
}

TBlob TSearchArchive::GetExtInfo(ui32 docId) const {
    if (IsOpen()) {
        TUnpackDocCtx ctx;
        return ReadDocHeader(docId, &ctx)->GetExtInfo(ctx);
    }
    return TBlob();
}

int TSearchArchive::UnpackDoc(ui32 docId, TBuffer* out) const {
    out->Clear();
    if (IsOpen()) {
        TUnpackDocCtx ctx;
        TBlob doctext = ReadDocHeader(docId, &ctx)->GetDocText(ctx);
        GetAllText(doctext.AsUnsignedCharPtr(), out);
        return 0;
    }
    return -1;
}

int TSearchArchive::FindDoc(ui32 docId, TVector<int>& breaks, TDocArchive& da) const {
    if (!IsOpen())
        return -1;

    TBlob b, doctext;
    try {
        TUnpackDocCtx ctx;
        IArchive* arc = ReadDocHeader(docId, &ctx);

        b = arc->GetExtInfo(ctx);
        doctext = arc->GetDocText(ctx);
    } catch (...) {
        da.Clear();
        return -1;
    }
    return FindDocCommon(b, doctext, breaks, da);
}

TSearchFullArchive::TSearchFullArchive() {
}

TSearchFullArchive::~TSearchFullArchive() {
}

void TSearchFullArchive::Open(const TString& index, bool map /* = false*/) {
    TString tag(TString::Join(index, "tag"));
    if (!NFs::Exists(tag))
        return;
    TString tdr(TString::Join(index, "tdr"));
    THolder<IArchive> textArchive;
    if (!map)
       textArchive.Reset(new TFileArchive(tag.data(), true));
    else
       textArchive.Reset(new TMapArchive(tag.data()));
    THolder<TArchiveDir> archiveDir(new TArchiveDir(tdr.data()));
    ArchiveText.Reset(textArchive.Release());
    ArchiveDir.Reset(archiveDir.Release());
}

void TSearchFullArchive::UnpackDoc(ui32 docId, TBuffer* out) const {
    out->Clear();

    TUnpackDocCtx ctx;
    GetDocHeader(docId, *ArchiveText, *ArchiveDir, &ctx);

    TBlob doctext = ArchiveText->GetDocText(ctx);
    if (doctext.Empty()) {
        return;
    }

    GetDocTextPart(doctext, FABT_ORIGINAL, out);
}

TString TSearchFullArchive::GetDocumentUrl(ui32 docId) const {
    TUnpackDocCtx ctx;
    GetDocHeader(docId, *ArchiveText, *ArchiveDir, &ctx);
    return NormArcUrl(ArchiveText->GetExtInfo(ctx));
}

TString GetDocumentUrl(const TSearchArchive& archive, const TSearchFullArchive& fullArchive, ui32 docId) {

    if (fullArchive.IsOpen())
        return fullArchive.GetDocumentUrl(docId);

    char szNormalUrl[URL_MAX];
    szNormalUrl[0] = 0;
    TDocArchive dA;
    if(!archive.IsInArchive(docId))
    {
         return szNormalUrl;
    }
    int retCode = archive.FindDocBlob(docId, dA);
    if (!retCode && dA.Exists) {
        TDocDescr docD;
        docD.UseBlob(dA.GetBlob(), dA.Blobsize);
        if (docD.IsAvailable()) {
            const char* url = docD.get_url();
            if (!NormalizeUrl(szNormalUrl, URL_MAX, url))
                strlcpy(szNormalUrl, url, URL_MAX - 1);
        }
    }
    return szNormalUrl;
}

template<>
void Out<IArchive::TFactoryKey>(IOutputStream& os, TTypeTraits<IArchive::TFactoryKey>::TFuncParam key) {
    os << "{" << key.ArchiveType << "," << key.OpenMode << "}";
}
