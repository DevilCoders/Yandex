#pragma once

#include "wad_text_archive.h"
#include "searcharc.h"

#include <util/generic/variant.h>

using TChunkElement = std::variant<TSearchArchive, TWadTextArchiveManager>;

class TChunkManager {
public:
    TChunkManager() = default;

    TChunkManager(THolder<TChunkElement> data)
        : Data(std::move(data))
    {
        IsWadTextArchive = std::holds_alternative<TWadTextArchiveManager>(*Data.Get());
    }

    void Open(const TString& indexPath,
        EArchiveOpenMode mode,
        EArchiveType arcType,
        size_t cacheSize,
        size_t replica,
        size_t repCoef,
        const NRTYArchive::TMultipartConfig& config = Default<NRTYArchive::TMultipartConfig>(),
        bool useMapping = false,
        bool lockMemory = false)
    {
        Y_ENSURE(!IsWadTextArchive);
        std::get<TSearchArchive>(*Data.Get()).Open(indexPath, mode, arcType, cacheSize, replica, repCoef, config, useMapping, lockMemory);
    }

    void Open(const TString& indexPath) {
        if (IsWadTextArchive) {
            std::get<TWadTextArchiveManager>(*Data.Get()).Open(indexPath, EDataType::DocText);
        } else {
            Open(indexPath, AOM_FILE, AT_FLAT, 0, 0, 1, Default<NRTYArchive::TMultipartConfig>(), 0, 0);
        }
    }

    int ObtainExtInfo(ui32 docId, TDocArchive &dA) const {
        if (IsWadTextArchive) {
            return std::get<TWadTextArchiveManager>(*Data.Get()).ObtainExtInfo(docId, dA);
        } else {
            return std::get<TSearchArchive>(*Data.Get()).FindDocBlob(docId, dA);
        }
    }

    TString GetDocumentUrl(ui32 docId, const TSearchFullArchive* fullArchive) const {
        if (IsWadTextArchive) {
            return TSearchFullArchive::NormArcUrl(std::get<TWadTextArchiveManager>(*Data.Get()).GetExtInfo(docId)->UncompressBlob());
        } else {
            return ::GetDocumentUrl(std::get<TSearchArchive>(*Data.Get()), *fullArchive, docId);
        }
    }

    TBlob GetFullBlob(ui32 docId) const {
        if (IsWadTextArchive) {
            return std::get<TWadTextArchiveManager>(*Data.Get()).GetDocFullInfo(docId)->UncompressBlob();
        } else {
            return std::get<TSearchArchive>(*Data.Get()).GetDocFullInfo(docId);
        }
    }

    TBlob GetDocText(ui32 docId) const {
        if (IsWadTextArchive) {
            return std::get<TWadTextArchiveManager>(*Data.Get()).GetDocText(docId)->UncompressBlob();
        } else {
            return std::get<TSearchArchive>(*Data.Get()).GetDocText(docId);
        }
    }

    TBlob GetExtInfo(ui32 docId) const {
        if (IsWadTextArchive) {
            return std::get<TWadTextArchiveManager>(*Data.Get()).GetExtInfo(docId)->UncompressBlob();
        } else {
            return std::get<TSearchArchive>(*Data.Get()).GetExtInfo(docId);
        }
    }

    TBlob GetBertEmbedding(ui32 docId) const {
        if (IsWadTextArchive) {
            return std::get<TWadTextArchiveManager>(*Data.Get()).GetBertEmbedding(docId)->UncompressBlob();
        } else {
            return std::get<TSearchArchive>(*Data.Get()).GetBertEmbedding(docId);
        }
    }

    TBlob GetBertEmbeddingV2(ui32 docId) const {
        if (IsWadTextArchive) {
            return std::get<TWadTextArchiveManager>(*Data.Get()).GetBertEmbeddingV2(docId)->UncompressBlob();
        } else {
            return std::get<TSearchArchive>(*Data.Get()).GetBertEmbedding(docId);
        }
    }


    ui32 GetDocCount() const {
        if (IsWadTextArchive) {
            return std::get<TWadTextArchiveManager>(*Data.Get()).GetDocCount();
        } else {
            return std::get<TSearchArchive>(*Data.Get()).GetMaxHndl() + 1;
        }
    }

    bool IsInArchive(ui32 docId) const {
        if (IsWadTextArchive) {
            return std::get<TWadTextArchiveManager>(*Data.Get()).IsInArchive(docId);
        } else {
            return std::get<TSearchArchive>(*Data.Get()).IsInArchive(docId);
        }
    }

    int UnpackDoc(ui32 docId, TBuffer* out) const {
        if (IsWadTextArchive) {
            return std::get<TWadTextArchiveManager>(*Data.Get()).UnpackDoc(docId, out);
        } else {
            return std::get<TSearchArchive>(*Data.Get()).UnpackDoc(docId, out);
        }
    }

private:
    THolder<TChunkElement> Data;
    bool IsWadTextArchive = true;
};
