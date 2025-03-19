#include "tarcio.h"

#include <util/draft/holder_vector.h>
#include <util/generic/algorithm.h>
#include <util/generic/string.h>

#include <utility>

struct TDocOffset {
    ui32 DocId;
    ui64 Offset;

    TDocOffset(ui32 docId, ui64 offset)
        : DocId(docId)
        , Offset(offset)
    {
    }

    bool operator<(const TDocOffset& docOffset) const {
        return Offset < docOffset.Offset;
    }
};

typedef TVector<TDocOffset> TDocOffsets;

void TArchiveIterator::OrderDocs(TDocIds* result) const {
    TDocOffsets offsets;
    offsets.reserve(DirReader.size());
    for (size_t i = 0; i < DirReader.size(); ++i)
        if (DirReader[i] != FAIL_DOC_OFFSET)
            offsets.push_back( TDocOffset(ui32(i), DirReader[i]) );
    Sort(offsets.begin(), offsets.end());
    result->resize(offsets.size());
    for (size_t i = 0; i < offsets.size(); ++i)
        (*result)[i] = offsets[i].DocId;
}

void MakeArchiveDir(const TString& arcname, const TString& dirname) {
    ui32 maxDocId = 0; // maximum docId within /arcname/

    {   // scan the archive to determine max docId
        TArchiveIterator arcIter;
        arcIter.Open(arcname.data());

        TArchiveHeader* header;
        while (header = arcIter.NextAuto()) {
            ui32 docId = header->DocId;
            maxDocId = Max(maxDocId, docId);
        }
    }

    TArrayHolder<ui64> dirBuffer; // buffered /dirname/
    dirBuffer.Reset(new ui64[maxDocId + 1]);
    memset(dirBuffer.Get(), 0xFF, (maxDocId + 1) * sizeof(ui64)); // fill up potential holes with 0xFF

    ui64 offset = ARCHIVE_FILE_HEADER_SIZE;

    TArchiveIterator arcIter;
    arcIter.Open(arcname.data());

    TArchiveHeader* header;
    while (header = arcIter.NextAuto()) {
        dirBuffer[header->DocId] = offset;
        offset += header->DocLen;
    }

    // write out /dirname/ buffer
    TFile dirFile(dirname, CreateAlways | WrOnly | Seq);
    dirFile.Write(dirBuffer.Get(), (maxDocId + 1) * sizeof(ui64));
    dirFile.Close();
}

void VerifyArchiveVersion(const TArchiveIterator& iterator, TArchiveVersion expectedVersion) {
    if (iterator.GetArchiveVersion() != expectedVersion) {
        ythrow yexception() << "archive " << iterator.GetFileName() << " has version " << iterator.GetArchiveVersion() << " expected " << expectedVersion;
    }
}

void ReplaceArchiveBlobs(TArchiveIterator& iter, IOutputStream* out, IArchiveBlobReplacer* replacer) {
    TBuffer newBlob(8192);
    TBuffer newText(8192);
    TArchiveHeader* curHdr = nullptr;
    ui32 newDocId;
    while ((curHdr = iter.NextAuto()) != nullptr) {
        TBlob curBlob = iter.GetExtInfo(curHdr);
        TBlob curText = iter.GetDocText(curHdr);
        newDocId = curHdr->DocId;
        newBlob.Clear();
        newText.Clear();
        IArchiveBlobReplacer::EReplaceResult res = replacer->ReplaceBlob(curHdr->DocId, curBlob, &newBlob,
             curText, &newText);
        while (res == IArchiveBlobReplacer::RR_INSERT) {
            WriteEmptyDoc(*out, newBlob.Data(), newBlob.Size(), newText.Data(), newText.Size(), newDocId);
            newDocId = curHdr->DocId;
            newBlob.Clear();
            newText.Clear();
            res = replacer->ReplaceBlob(curHdr->DocId, curBlob, &newBlob, curText, &newText);
        }
        if (res == IArchiveBlobReplacer::RR_UPDATE) {
            ui32 newExtLen = (ui32)newBlob.Size();
            Y_ASSERT(curHdr->ExtLen < curHdr->DocLen);
            curHdr->DocId = newDocId;
            curHdr->DocLen = curHdr->DocLen + newExtLen - curHdr->ExtLen;
            curHdr->ExtLen = newExtLen;
            if (newText.Size() == 0) {
                out->Write(curHdr, sizeof(TArchiveHeader));
                out->Write(newBlob.Data(), newBlob.Size());
                out->Write(curText.Data(), curText.Size());
            } else {
                curHdr->DocLen = ui32(curHdr->DocLen + newText.Size() - curText.Size());
                out->Write(curHdr, sizeof(TArchiveHeader));
                out->Write(newBlob.Data(), newBlob.Size());
                out->Write(newText.Data(), newText.Size());
            }
        } else if (res == IArchiveBlobReplacer::RR_SKIP) {
            out->Write(curHdr, curHdr->DocLen);
        }
    }
}

TArchiveDirBuilder::TArchiveDirBuilder(IOutputStream& out)
    : Out(out)
{}

TArchiveDirBuilder::TArchiveDirBuilder(THolder<IOutputStream>&& out)
    : OutHolder(std::move(out))
    , Out(*OutHolder)
{}

TArchiveDirBuilder::~TArchiveDirBuilder() {
    if (!Finished) {
        Finish();
    }
}

void TArchiveDirBuilder::AddEntry(const TArchiveHeader& header) {
    ui32 docId = header.DocId;
    if (DirEntryList.size() < docId + 1) {
        DirEntryList.resize(docId + 1, -1);
    }
    DirEntryList[docId] = Offset;
    Offset += header.DocLen;
}

void TArchiveDirBuilder::Finish() {
    const char* buffer = reinterpret_cast<const char*>(DirEntryList.data());
    size_t size = DirEntryList.size() * sizeof(decltype(DirEntryList)::value_type);
    Out.Write(buffer, size);
    Finished = true;
}
