#pragma once

#include "arcface.h"
#include "reciter.h"
#include "settings.h"

#include <util/memory/blob.h>
#include <util/stream/output.h>
#include <util/stream/str.h>
#include <util/generic/buffer.h>
#include <util/generic/fwd.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/system/filemap.h>
struct TDocAbsentException : public yexception {};

class IArchiveIterator {
public:
    virtual ~IArchiveIterator() {};
    virtual void Open(const char *name, EOpenMode mode = 0) = 0;

    enum EArcError {
        NoError,
        DocAbsent,
        DocIdMismatch,
    };

    struct TArcError {
        EArcError Error;
        TStringStream Message;
        TArcError()
            : Error(NoError)
        {}
    };
    typedef TAutoPtr<TArcError> TArcErrorPtr;

    // throws
    virtual TArchiveHeader* SeekToDoc(ui32 docid) = 0;
    /// returns error information in @c err
    virtual TArchiveHeader* SeekToDoc(ui32 docid, TArcErrorPtr& err) = 0;
    virtual TArchiveHeader* NextAuto() = 0;

};

inline
TBlob GetArchiveExtInfo(const TArchiveHeader* ctx, TArchiveVersion archiveVersion = ARCVERSION) {
    if (ctx->ExtLen) {
        const char* data = (const char*)ctx;
        data += sizeof(TArchiveHeader);
        TBlob rawExtInfo = TBlob::NoCopy(data, ctx->ExtLen);
        return UnpackRawExtInfo(rawExtInfo, archiveVersion);
    }
    return TBlob();
}

inline
TBlob GetArchiveDocText(const TArchiveHeader* ctx) {
    ui32 textLen = ctx->GetTextLen();
    if (textLen) {
        const char* data = (const char*)ctx;
        data += sizeof(TArchiveHeader) + ctx->ExtLen;
        return TBlob::NoCopy(data, textLen);
    }
    return TBlob();
}

class TArchiveIterator :    public TFileRecordIterator<TArchiveHeader>,
                            public IArchiveIterator
{
private:
    TFileMappedArray<ui64> DirReader;
    TArchiveVersion ArchiveVersion;

public:
    TArchiveIterator(size_t bufSize = 32 << 20)
        : TFileRecordIterator<TArchiveHeader>(bufSize)
        , ArchiveVersion(0)
    {
    }

    void Open(const char *name, EOpenMode mode = 0) override {
        TFileRecordIterator<TArchiveHeader>::Open(name, *this, mode);
    }

    void LoadHeader(IInputStream& in) {
        CheckTextArchiveHeader(in, ArchiveVersion);
    }

    void OpenDir(const char *name = nullptr) {
        TString dirname(name);
        if (!dirname) {
            dirname = GetFileName();
        if (dirname.EndsWith(TStringBuf("tag")))
            dirname.replace(dirname.size()-3, 3, "tdr");
        else
            dirname.replace(dirname.size()-3, 3, "dir");
        }
        DirReader.Init(dirname.data());
    }

    ui32 Size() {
        if (DirReader.Empty())
            OpenDir();
        return (ui32)DirReader.Size();
    }

    TArchiveHeader* SeekToDoc(ui32 docid, TArcErrorPtr& err) override {
        if (DirReader.Empty())
            OpenDir();
        Y_ENSURE(docid < DirReader.Size(),
            "DocId " << docid << " too big for " << GetFileName().Quote() << ". ");

        TArchiveHeader* hdr(nullptr);
        i64 offset = DirReader[docid];
        if ((ui64)offset == FAIL_DOC_OFFSET) {
            err.Reset(new TArcError);
            err->Error = DocAbsent;
            err->Message << "Document with " << docid << " id is absent in directory. ";
            return nullptr;
        }
        Seek(offset);
        hdr = Current();

        Y_ASSERT(hdr);
        Y_ENSURE(hdr, "Bad docId " << docid << " on " << offset << ". ");

        if (hdr->DocId != docid && Singleton<TArchiveSettings>()->DocIdCheckerEnabled) {
            err.Reset(new TArcError);
                err->Error = DocIdMismatch;
            // FIXME: GetFileName may be wrong, better to get name from FileReadArray
            err->Message << "Unrecognized data in file " << GetFileName().Quote() << ". Got id " <<
                hdr->DocId << " have to be " << docid << ". ";
            // TDocAbsentException was here
            return nullptr;
        }

        return hdr;
    }

    TArchiveHeader* SeekToDoc(ui32 docid) override {
        TArcErrorPtr err;
        TArchiveHeader* hdr = SeekToDoc(docid, err);
        if (hdr)
            return hdr;

        Y_ASSERT(!!err);
        Y_ASSERT(err->Error != NoError);
        // shit happen, err should contains error information

        // FIXME: Indeed, corrupted data is not a missing document
        // To preserve old behaviour, we will still throw TDocAbsentException.
        // If you need precise error handling, use extended SeekToDoc version
        ythrow TDocAbsentException() << err->Message.Str();
    }

    TArchiveHeader* NextAuto() override {
        TArchiveHeader* val = Next();
        if (val || !RefillBuffer())
            return val;
        return Next();
    }

    TBlob GetExtInfo(const TArchiveHeader* ctx) const {
        Y_ASSERT(IsValidPtr(ctx));
        return GetArchiveExtInfo(ctx, ArchiveVersion);
    }

    TBlob GetDocText(const TArchiveHeader* ctx) const {
        Y_ASSERT(IsValidPtr(ctx));
        return GetArchiveDocText(ctx);
    }

    TArchiveVersion GetArchiveVersion() const {
        return ArchiveVersion;
    }

    typedef TVector<ui32> TDocIds;

    void OrderDocs(TDocIds* result) const;
};

void MakeArchiveDir(const TString& arcname, const TString& dirname);

class IArchiveBlobReplacer
{
protected:
    virtual ~IArchiveBlobReplacer() {
    }
public:
    enum EReplaceResult {
        RR_SKIP,     // one need use the old blob from curBlob
        RR_UPDATE,   // one need replace blob with the data in 'toWrite'
        RR_REMOVE,   // one need discard the document
        RR_INSERT,   // one need insert the new document with empty text
    };
    virtual EReplaceResult ReplaceBlob(ui32 curDocId, const TBlob& curBlob, TBuffer* toWriteNewBlob,
        const TBlob& curText, TBuffer* toWriteNewText) = 0;
};

void ReplaceArchiveBlobs(TArchiveIterator& iter, IOutputStream* out, IArchiveBlobReplacer* replacer);

class TArchiveDirBuilder {
public:
    TArchiveDirBuilder(IOutputStream& out);
    TArchiveDirBuilder(THolder<IOutputStream>&& out);
    ~TArchiveDirBuilder();

    void AddEntry(const TArchiveHeader& header);
    void Finish();

private:
    TVector<ui64> DirEntryList;
    THolder<IOutputStream> OutHolder;
    IOutputStream& Out;
    ui64 Offset = ARCHIVE_FILE_HEADER_SIZE;
    bool Finished = false;
};

void VerifyArchiveVersion(const TArchiveIterator& iterator, TArchiveVersion expectedVersion);
