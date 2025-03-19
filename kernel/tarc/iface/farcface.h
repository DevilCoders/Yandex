#pragma once

#include "arcface.h"
#include "tarcio.h"
#include "fulldoc.h"

#include <util/stream/buffer.h>

struct TBlockForFullArchiveWriter {
    const void* BlockText;
    size_t BlockTextSize;
    EFullArchiveBlockType BlockType;
    TBlockForFullArchiveWriter()
        : BlockText(nullptr)
        , BlockTextSize(0)
        , BlockType(FABT_ORIGINAL)
    {}
};

class TFullArchiveWriter
{
private:
    IOutputStream& Output;
    TBufferOutput Buf;
public:
    TFullArchiveWriter(IOutputStream& o, size_t initBufSize = 0xA00000);
    ~TFullArchiveWriter();

    ui64 WriteDocToFullArchive(ui32 docId, const TFullArchiveDocHeader& hdr,
                           const TBlockForFullArchiveWriter* blocks, size_t blocksCount,
                           const void* extInfo = nullptr, size_t extInfoLen = 0);
};

class TFullArchiveIterator : public IArchiveIterator {
private:
    TArchiveIterator Iter;
    TArchiveHeader* CurHeader;
    TBuffer CurDocText;
    TFullArchiveDocHeader CurFullHeader;
public:
    TFullArchiveIterator(size_t bufSize = 32 << 20)
        : Iter(bufSize)
        , CurHeader(nullptr)
    {
    }

    void Open(const char *name, EOpenMode mode = 0) override {
        Iter.Open(name, mode);
    }

private:
    TArchiveHeader* SeekToDocInt(TArchiveHeader* hdr) {
        CurHeader = hdr;
        Reset();
        return CurHeader;
    }

public:
    TArchiveHeader* SeekToDoc(ui32 docid) override {
        return SeekToDocInt(Iter.SeekToDoc(docid));
    }

    TArchiveHeader* SeekToDoc(ui32 docid, TArcErrorPtr& err) override {
        TArchiveHeader* hdr = Iter.SeekToDoc(docid, err);
        if (!hdr)
            return nullptr;
        return SeekToDocInt(hdr);
    }

    void OpenTdr(const char *name = "tdr") {
        Iter.OpenDir(name);
        CurHeader = nullptr;
    }

    ui32 GetDocLen() const {
        Y_ASSERT(CurHeader);
        return CurHeader->DocLen;
    }

    ui32 GetDocId() const {
        Y_ASSERT(CurHeader);
        return CurHeader->DocId;
    }

    TFullArchiveDocHeader* GetFullHeader() {
        return &CurFullHeader;
    }

    const char* GetUrl() const {
        return CurFullHeader.Url;
    }

    const char* GetDocText() const {
        return CurDocText.Size() == 0 ? nullptr : CurDocText.Data();
    }

    size_t GetDocTextSize() const {
        return CurDocText.Size() == 0 ? 0 : CurDocText.Size() - 1;
    }

    TBlob GetExtInfo() const;

    bool Next() {
        CurHeader = Iter.NextAuto();
        if (CurHeader) {
            Reset();
            return true;
        }
        return false;
    }

    TArchiveHeader* NextAuto() override {
        if (Next()) {
            return CurHeader;
        }
        return nullptr;
    }

    TArchiveHeader* Current() {
        return CurHeader;
    }

    ui32 Size() {
        return Iter.Size();
    }

    const char* MakeDocText();

    typedef TVector<ui32> TDocIds;

    void OrderDocs(TDocIds* result) const {
        Iter.OrderDocs(result);
    }

    void GetDocTextPart(EFullArchiveBlockType type, TBuffer* toWrite, bool clearBuffer = true);
    ui32 FindPackedLength() const;
private:
    void Reset();
};

void GetDocTextPart(const TBlob& data, EFullArchiveBlockType type, TBuffer* toWrite);
size_t CalculateFullHeaderLenght(const TBlob& fullExtBlob);
void MakeFullArchiveDocHeader(TFullArchiveDocHeader&, const TArchiveIterator&, const TArchiveHeader*);
TBlob GetExtInfo(const TBlob& fullExtBlob);
