#include "farcface.h"

#include <util/stream/zlib.h>
#include <util/generic/buffer.h>
#include <util/generic/cast.h>
#include <util/string/cast.h>

static const size_t FullHeaderPrefixLength = sizeof(TFullArchiveDocHeader) - URL_MAX;
static_assert(FullHeaderPrefixLength == 12, "expect FullHeaderPrefixLength == 12");

TFullArchiveWriter::TFullArchiveWriter(IOutputStream& o, size_t initBufSize)
    : Output(o)
    , Buf(initBufSize)
{
}

TFullArchiveWriter::~TFullArchiveWriter() {
}

ui64 TFullArchiveWriter::WriteDocToFullArchive(ui32 docId, const TFullArchiveDocHeader& hdr,
                       const TBlockForFullArchiveWriter* blocks, size_t blocksCount,
                       const void* extInfo /*= NULL*/, size_t extInfoLen /*= 0*/)
{
    TFullArchiveTextHeader txtHdr;
    txtHdr.BlockCount = IntegerCast<ui16>(blocksCount);

    TVector<TFullArchiveTextBlockInfo> blockInfos;
    ui64 len = 0;

    if (blocks && blocksCount) {
        blockInfos.reserve(txtHdr.BlockCount);
        for (size_t i = 0; i < txtHdr.BlockCount; i++) {
            const TBlockForFullArchiveWriter& block = blocks[i];
            TZLibCompress compressor(&Buf);
            compressor.Write(block.BlockText, block.BlockTextSize);
            compressor.Finish();

            TFullArchiveTextBlockInfo bInfo;
            bInfo.BlockType = (ui16)block.BlockType;
            bInfo.EndOffset = IntegerCast<ui32>(Buf.Buffer().Size());
            blockInfos.push_back(bInfo);

        }
    }

    TArchiveHeader arcHdr;
    arcHdr.DocId = docId;

    size_t tagsHdrLen = FullHeaderPrefixLength + strlen(hdr.Url) + 1;
    arcHdr.ExtLen = IntegerCast<ui32>(tagsHdrLen + extInfoLen);

    ui32 blocksInfoLen = IntegerCast<ui32>(blockInfos.size() * sizeof(TFullArchiveTextBlockInfo));
    ui32 textLen = IntegerCast<ui32>(sizeof(TFullArchiveTextHeader) + blocksInfoLen + Buf.Buffer().Size());

    arcHdr.DocLen = sizeof(TArchiveHeader) + arcHdr.ExtLen + textLen;

    Output.Write(&arcHdr, sizeof(TArchiveHeader));
    Output.Write(&hdr, tagsHdrLen);
    Output.Write(extInfo, extInfoLen);
    len += (ui64)(sizeof(TArchiveHeader) + tagsHdrLen + extInfoLen);

    Output.Write(&txtHdr, sizeof(TFullArchiveTextHeader));
    Output.Write(blockInfos.data(), blocksInfoLen);
    Output.Write(Buf.Buffer().Data(), Buf.Buffer().Size());
    len += (ui64)(sizeof(TFullArchiveTextHeader) + blocksInfoLen + Buf.Buffer().Size());

    Buf.Buffer().Clear();
    return len;
}

size_t CalculateFullHeaderLenght(const TBlob& fullExtBlob) {
    return FullHeaderPrefixLength + strlen(fullExtBlob.AsCharPtr() + FullHeaderPrefixLength) + 1;
}

TBlob GetExtInfo(const TBlob& fullExtBlob) {
    return fullExtBlob.SubBlob(CalculateFullHeaderLenght(fullExtBlob), fullExtBlob.Size());
}

void MakeFullArchiveDocHeader(TFullArchiveDocHeader& fh, const TArchiveIterator& iter, const TArchiveHeader* header) {
    Y_ASSERT(header);

    TBlob curExtBlob = iter.GetExtInfo(header);

    if (curExtBlob.Size() <= FullHeaderPrefixLength)
        ythrow yexception() << "bad data in file \"" <<  iter.GetFileName().data() << "\" at docId = " <<  header->DocId;

    size_t fullHdrLen = CalculateFullHeaderLenght(curExtBlob);

    if (fullHdrLen > curExtBlob.Size())
        ythrow yexception() << "bad data in file \"" <<  iter.GetFileName().data() << "\" at docId = " <<  header->DocId;

    memcpy(&fh, curExtBlob.Data(), fullHdrLen);
}

void TFullArchiveIterator::Reset() {
    Y_ASSERT(CurHeader);
    CurDocText.Clear();
    MakeFullArchiveDocHeader(CurFullHeader, Iter, CurHeader);
}

ui32 GetDocPackedLength(const TBlob& data) {
    const char* datap = data.AsCharPtr();
    const TFullArchiveTextHeader* hdr = (const TFullArchiveTextHeader*)datap;
    const TFullArchiveTextBlockInfo* blockInfo = (const TFullArchiveTextBlockInfo*)(datap + sizeof(*hdr));
    ui32 offset = 0;
    for (ui16 i = 0; i < hdr->BlockCount; i++, blockInfo++) {
        if (blockInfo->BlockType == FABT_ORIGINAL)
            return blockInfo->EndOffset - offset;
        offset = blockInfo->EndOffset;
    }
    return 0;
}

void GetDocTextPart(const TBlob& data, EFullArchiveBlockType type, TBuffer* toWrite) {
    const char* datap = data.AsCharPtr();

    const char* end = data.AsCharPtr() + data.Size();

    if (datap + sizeof(TFullArchiveTextHeader) > end)
        ythrow yexception() << "corrupted blob";
    TFullArchiveTextHeader* hdr = (TFullArchiveTextHeader*) datap;
    datap += sizeof(TFullArchiveTextHeader);

    if (datap + hdr->BlockCount * sizeof(TFullArchiveTextBlockInfo) > end)
        ythrow yexception() << "corrupted blob";
    TFullArchiveTextBlockInfo* blockInfos = (TFullArchiveTextBlockInfo*)datap;
    datap += hdr->BlockCount * sizeof(TFullArchiveTextBlockInfo);

    ui32 offset = 0;
    TBufferOutput block(*toWrite);

    for (ui16 i = 0; i < hdr->BlockCount; i++) {
        const TFullArchiveTextBlockInfo& b = blockInfos[i];
        if (b.BlockType == type) {
            if (datap + b.EndOffset > end)
                ythrow yexception() << "corrupted blob";
            TMemoryInput in(datap + offset, b.EndOffset - offset);
            TZLibDecompress decompressor(&in);
            TransferData(&decompressor, &block);
            break;
        }
        offset = b.EndOffset;
    }
}

void TFullArchiveIterator::GetDocTextPart(EFullArchiveBlockType type, TBuffer* toWrite, bool clearBuffer) {
    if (clearBuffer)
        toWrite->Clear();
    Y_ASSERT(CurHeader);
    TBlob curDocBlob = Iter.GetDocText(CurHeader);
    if (!curDocBlob.Empty())
        ::GetDocTextPart(curDocBlob, type, toWrite);
}

const char* TFullArchiveIterator::MakeDocText() {
    Y_ASSERT(CurDocText.Size() == 0);
    GetDocTextPart(FABT_ORIGINAL, &CurDocText);
    CurDocText.Append(0);
    return GetDocText();
}

TBlob TFullArchiveIterator::GetExtInfo() const {
    Y_ASSERT(CurHeader);
    return ::GetExtInfo(Iter.GetExtInfo(CurHeader));
}

ui32 TFullArchiveIterator::FindPackedLength() const {
    return GetDocPackedLength(Iter.GetDocText(CurHeader));
}
