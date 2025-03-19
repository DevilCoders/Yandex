#include "archive_file.h"

#include <util/string/util.h>
#include <util/system/env.h>

// We assume that we have only one writer
void TMemoryArchiveFile::TArchiveFileData::Init(size_t maxDocs, const TString& workDir, size_t suffix, size_t fileNo, size_t subFileNo) {
    TString fileName = Sprintf("%s/yandex-%lu-%lu-%lu.tmp", workDir.data(),
        (long unsigned)suffix, (long unsigned)fileNo, (long unsigned)subFileNo);
    File = TFile(fileName, OpenAlways | RdWr);
    Y_VERIFY(File.IsOpen(), "Cannot open temporary file %s for archive\n", fileName.data());
    DocArchiveInfos.resize(maxDocs * 2);
    Reset();
}

void TMemoryArchiveFile::TArchiveFileData::Reset() {
    DocIdsNum = 0;
    RemovedDocIdsNum = 0;
}

void TMemoryArchiveFile::TArchiveFileData::Append(const void* data, size_t size, ui64& currentOffset) {
    if (!size)
        return;
    File.Write(data, size);
    currentOffset += size;
    Y_ASSERT(DocIdsNum);
    DocArchiveInfos[DocIdsNum - 1].Length += (ui32)size;
}

void TMemoryArchiveFile::TArchiveFileData::AddOne(ui32 docId, const TBlob& blob, TMemoryArchiveFile& owner) {
    Y_VERIFY(DocIdsNum < DocArchiveInfos.size(), "Error in file archive system! Too many documents in one subfile.\n");
    DocArchiveInfos[DocIdsNum] = TDocArchiveInfo(owner.CurrentOffset, (ui32)blob.Size(), docId);
    Append(blob.Data(), blob.Size(), owner.CurrentOffset);
    owner.CurrentPlace.DocNumberInFile = DocIdsNum;
    DocIdsNum++;
    TDocPlace oldPlace = owner.DocPlaces[docId];
    // Mark new copy of document as current in global registry.
    owner.DocPlaces[docId] = owner.CurrentPlace;
    // Mark old copy of document as old in file (if there is old copy).
    if (oldPlace.DocNumberInFile != TDocPlace().DocNumberInFile)
        owner.Files[oldPlace.FileNo].EraseDoc(oldPlace);
}

TMemoryArchiveFile::TArchiveFileData::TDocArchiveInfo* TMemoryArchiveFile::TArchiveFileData::RemoveOne(TBlob& blob) {
    TDocArchiveInfo* ret = nullptr;
    while (RemovedDocIdsNum < DocIdsNum && ((!ret) || ret->DocId == (ui32)-1 || ret->Length == 0)) {
        ret = &DocArchiveInfos[RemovedDocIdsNum];
        RemovedDocIdsNum++;
        if (RemovedDocIdsNum == DocIdsNum)
            File.Seek(0, sSet);
    }
    if (!ret)
        return nullptr;
    Y_VERIFY(ret->Offset != TDocArchiveInfo().Offset, "Incorrect offset specified for docid=%lu\n", (long unsigned)ret->DocId);
    Y_VERIFY(ret->Length, "Incorrect length specified for docid=%lu\n", (long unsigned)ret->DocId);
    blob = TBlob::FromFileContent(File, ret->Offset, ret->Length);
    return ret;
}

bool TMemoryArchiveFile::TArchiveFileData::CopyOne(
    TMemoryArchiveFile::TArchiveFileData& mirror,
    TMemoryArchiveFile& owner)
{
    TBlob blob;
    TDocArchiveInfo* ret = mirror.RemoveOne(blob);
    if (!ret)
        return false;
    ui32 docId = ret->DocId;
    // Assuming that RemoveOne has filtered all old documents.
    Y_ASSERT(docId != (ui32)-1);
    AddOne(docId, blob, owner);
    ret->DocId = (ui32)-1;
    return true;
}

bool TMemoryArchiveFile::TArchiveFileData::AddDoc(ui32 maxDocs, ui32 docId, TArchiveFileData& mirror, TMemoryArchiveFile& owner) {
    for (size_t i = 0; i < 2; i++)
        if (!CopyOne(mirror, owner))
            break;
    if (maxDocs <= DocIdsNum) {
        // Coping all left documents of previous dump.
        while (CopyOne(mirror, owner)) {
        }
        return false;
    }
    AddOne(docId, TBlob(), owner);
    return true;
}

void TMemoryArchiveFile::TArchiveFileData::EraseDoc(const TDocPlace& place) {
    Y_ASSERT(place.DocNumberInFile < DocArchiveInfos.size());
    DocArchiveInfos[place.DocNumberInFile].DocId = (ui32)-1;
}

void TMemoryArchiveFile::TArchiveFilePair::Append(const TDocPlace& place, const void* data, size_t size, ui64& offset) {
    Files[place.SubFileNo].Append(data, size, offset);
}

void TMemoryArchiveFile::TArchiveFilePair::Init(size_t maxDocs, const TString& workDir, size_t suffix, size_t fileNo) {
    for (size_t i = 0; i < 2; i++)
        Files[i].Init(maxDocs, workDir, suffix, fileNo, i);
    CurrentSubFileNo = 0;
}

bool TMemoryArchiveFile::TArchiveFilePair::AddDoc(ui32 maxDocs, ui32 docId, TMemoryArchiveFile& owner) {
    owner.CurrentPlace.SubFileNo = CurrentSubFileNo;
    bool ret = Files[CurrentSubFileNo].AddDoc(maxDocs, docId, Files[(CurrentSubFileNo + 1) % 2], owner);
    if (!ret) {
        CurrentSubFileNo = (CurrentSubFileNo + 1) % 2;
        owner.CurrentOffset = 0;
        Files[CurrentSubFileNo].Reset();
    }
    return ret;
}

void TMemoryArchiveFile::TArchiveFilePair::EraseDoc(const TDocPlace& place) {
    Files[place.SubFileNo].EraseDoc(place);
}

TMemoryArchiveFile::TMemoryArchiveFile(const char* workDir, int suffix, ui32 maxDocs)
    : IMemoryArchive(maxDocs)
    , DocPlaces(MaxDocs)
    , CurrentPlace(0, 0, 0)
    , CurrentOffset(0)
    , CurrentFileNo(0)
{
    TString workDirStr = workDir;
    if (!workDir)
        workDirStr = GetEnv("TMPDIR");
    for (size_t i = 0; i < FILES_NUM; i++)
        Files[i].Init(MaxDocs, workDirStr.data(), suffix, i);
}

void TMemoryArchiveFile::DoWrite(const void* data, size_t size) {
    Files[CurrentPlace.FileNo].Append(CurrentPlace, data, size, CurrentOffset);
}

void TMemoryArchiveFile::AddDoc(ui32 docId) {
    bool ret = Files[CurrentPlace.FileNo = CurrentFileNo].AddDoc(MaxDocs, docId, *this);
    if (ret)
        return;
    CurrentFileNo = (CurrentFileNo + 1) % FILES_NUM;
    ret = Files[CurrentPlace.FileNo = CurrentFileNo].AddDoc(MaxDocs, docId, *this);
    Y_VERIFY(ret, "Both archive files are full! Impossible!\n");
}
void TMemoryArchiveFile::EraseDoc(ui32 docId) {
    TDocPlace currentPlace = DocPlaces[docId];
    bool correctPlace = currentPlace.DocNumberInFile != TDocPlace().DocNumberInFile;
    if (!correctPlace)
        return;
    Files[currentPlace.FileNo].EraseDoc(currentPlace);
    DocPlaces[docId] = TDocPlace();
}

void TMemoryArchiveFile::GetExtInfoAndDocText(ui32 docId, TBlob& extInfoBlob, TBlob& docTextBlob, bool getExtInfo, bool getDocText) const {
    TUnpackDocCtx ctx;
    TDocPlace currentPlace = DocPlaces[docId];
    bool correctPlace = currentPlace.DocNumberInFile != TDocPlace().DocNumberInFile;
    TCtxData ret = (correctPlace) ? Files[currentPlace.FileNo].GetFileAndOffset(currentPlace) : TCtxData(nullptr, FAIL_DOC_OFFSET);
    ctx.Set(*ret.first, ret.second);
    TArchive<const TFile&> archiveText(*ret.first, false);
    IMemoryArchive::GetExtInfoAndDocText(archiveText, ctx, extInfoBlob, docTextBlob, getExtInfo, getDocText);
}
