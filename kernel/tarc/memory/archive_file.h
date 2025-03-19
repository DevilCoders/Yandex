#pragma once

#include "archive.h"

class TMemoryArchiveFile : public IMemoryArchive {
private:
    static const size_t FILES_NUM = 2;

    struct TDocPlace {
        union {
            struct {
                ui32 DocNumberInFile:30;
                ui32 FileNo:1;
                ui32 SubFileNo:1;
            };
            ui32 DW;
        };

        TDocPlace(ui32 docNumberInFile = (ui32)-1, ui32 fileNo = (ui32)-1, ui32 subFileNo = (ui32)-1)
            : DocNumberInFile(docNumberInFile)
            , FileNo(fileNo)
            , SubFileNo(subFileNo)
        { }

        TDocPlace(const TDocPlace&) noexcept = default;

        inline TDocPlace& operator=(const TDocPlace&) noexcept = default;
    };

    static_assert(sizeof(TDocPlace) == sizeof(ui32), "expect sizeof(TDocPlace) == sizeof(ui32)");
    static_assert(sizeof(TDocPlace) <= sizeof(void*), "expect sizeof(TDocPlace) <= sizeof(void*)");

    typedef TVector<TDocPlace> TDocPlaces;
    typedef std::pair<const TFile*, ui64> TCtxData;

    class TArchiveFileData {
    private:
        struct TDocArchiveInfo {
            ui64 Offset;
            ui32 Length;
            ui32 DocId;

            TDocArchiveInfo(ui64 offset = (ui64)-1, ui32 length = 0, ui32 docId = (ui32)-1)
                : Offset(offset)
                , Length(length)
                , DocId(docId)
            { }
        };

        TFile File;
        TVector<TDocArchiveInfo> DocArchiveInfos;
        ui32 DocIdsNum;
        ui32 RemovedDocIdsNum;

        void AddOne(ui32 docId, const TBlob& blob, TMemoryArchiveFile& owner);
        TDocArchiveInfo* RemoveOne(TBlob& blob);
        bool CopyOne(TArchiveFileData& mirror, TMemoryArchiveFile& owner);

    public:
        void Append(const void* data, size_t size, ui64& currentOffset);
        void Init(size_t maxDocs, const TString& workDir, size_t suffix, size_t fileNo, size_t subFileNo);
        void Reset();

        // Returns false when archive file is full.
        bool AddDoc(ui32 maxDocs, ui32 docId, TArchiveFileData& mirror, TMemoryArchiveFile& owner);
        void EraseDoc(const TDocPlace& place);
        TCtxData GetFileAndOffset(const TDocPlace& place) const {
            const TDocArchiveInfo* info = &DocArchiveInfos[place.DocNumberInFile];
            if (place.DocNumberInFile >= DocArchiveInfos.size() || !info->Length)
                return TCtxData(nullptr, FAIL_DOC_OFFSET);
            return TCtxData(&File, info->Offset);
        }
    };
    class TArchiveFilePair {
    private:
        TArchiveFileData Files[2];
        size_t CurrentSubFileNo;

    public:
        void Append(const TDocPlace& place, const void* data, size_t size, ui64& offset);
        void Init(size_t maxDocs, const TString& workDir, size_t suffix, size_t fileNo);
        bool AddDoc(ui32 maxDocs, ui32 docId, TMemoryArchiveFile& owner);
        void EraseDoc(const TDocPlace& place);
        TCtxData GetFileAndOffset(const TDocPlace& place) const {
            return Files[place.SubFileNo].GetFileAndOffset(place);
        }
    };

    TArchiveFilePair Files[FILES_NUM];
    TDocPlaces DocPlaces;
    TDocPlace CurrentPlace;
    ui64 CurrentOffset;
    size_t CurrentFileNo;

public:
    TMemoryArchiveFile(const char* workDir, int suffix, ui32 maxDocs);

    ~TMemoryArchiveFile() override {
    }

    void AddDoc(ui32 docId) override;
    void DoWrite(const void* data, size_t size) override;
    void EraseDoc(ui32 docId) override;
    bool HasDoc(ui32 docId) const override {
        return DocPlaces[docId].DocNumberInFile != TDocPlace().DocNumberInFile;
    }
    IMemoryArchiveDocGuardPtr GuardDoc(ui32 /*docId*/) const override {
        return nullptr;
    }

protected:
    void GetExtInfoAndDocText(ui32 docId, TBlob& extInfoBlob, TBlob& docTextBlob, bool getExtInfo, bool getDocText) const override;
};
