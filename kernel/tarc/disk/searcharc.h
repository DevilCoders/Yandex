#pragma once

#include "unpacker.h"

#include <kernel/tarc/iface/fulldoc.h>
#include <kernel/tarc/iface/tarcface.h>

#include <library/cpp/string_utils/old_url_normalize/url.h>

#include <util/memory/blob.h>
#include <util/generic/buffer.h>
#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/system/file.h>


class TDocArchive;

namespace NRTYArchive {
    struct TMultipartConfig;
}

class TSearchArchive {
private:
    THolder<IArchive> ArchiveText;
    THolder<IArchiveDir> ArchiveDir;
    THolder<TMappedAllocation> CacheData;
    THolder<TMappedArray<ui64> > CacheDirData;
    THolder<IArchive> CacheArchive;
    THolder<IArchiveDir> CacheDir;
public:
    TSearchArchive();
    ~TSearchArchive();

    void Open(
       const TString& index,
       EArchiveOpenMode mode,
       EArchiveType arcType,
       size_t cacheSize,
       size_t replica,
       size_t repCoef,
       const NRTYArchive::TMultipartConfig& config = Default<NRTYArchive::TMultipartConfig>(),
       bool useMapping = false,
       bool lockMemory = false);


    void Open(
        const TString& index,
        EArchiveOpenMode mode = AOM_FILE,
        EArchiveType arcType = AT_FLAT,
        bool useMapping = false,
        bool lockMemory = false)
    {
        Open(index, mode, arcType, 0, 0, 1, Default<NRTYArchive::TMultipartConfig>(), useMapping, lockMemory);
    }

    bool IsOpen() const {
        return !!ArchiveText && !!ArchiveDir;
    }

    bool IsInArchive(ui32 docid) const {
        return !!ArchiveDir && ArchiveDir->HasDoc(docid);
    }

    ui32 GetMaxHndl() const;
    TBlob GetDocFullInfo(ui32 docId) const;
    /**
     * @return 0 on success, nonzero (-1) on failures
     **/
    int FindDocBlob(ui32 docId, TDocArchive& da) const;
    int UnpackDoc(ui32 docId, TBuffer* out) const;
    TBlob GetDocText(ui32 docId) const;
    TBlob GetExtInfo(ui32 docId) const;
    TBlob GetBertEmbedding(ui32 docId) const;
    int FindDoc(ui32 docId, TVector<int>& breaks, TDocArchive& da) const;
private:
    IArchive* ReadDocHeader(ui32 docId, TUnpackDocCtx* ctx) const;
};

class TSearchFullArchive {
private:
    THolder<IArchive> ArchiveText;
    THolder<IArchiveDir> ArchiveDir;
public:
    TSearchFullArchive();
    ~TSearchFullArchive();

    void Open(const TString& index, bool map = false);

    bool IsOpen() const {
        return !!ArchiveText && !!ArchiveDir;
    }

    void UnpackDoc(ui32 docId, TBuffer* out) const;
    TString GetDocumentUrl(ui32 docId) const;

    static TString NormArcUrl(const TBlob& info) {
        char szNormalUrl[URL_MAX];
        szNormalUrl[0] = 0;
        if (!info.Empty()) {
            const char* url = info.AsCharPtr() + sizeof(TFullArchiveDocHeader) - URL_MAX;
            if (!NormalizeUrl(szNormalUrl, URL_MAX, url)) {
                strlcpy(szNormalUrl, url, URL_MAX - 1);
            }
        }
        return szNormalUrl;
    }
};

TString GetDocumentUrl(const TSearchArchive& archive, const TSearchFullArchive& fullArchive, ui32 arcDocId);
