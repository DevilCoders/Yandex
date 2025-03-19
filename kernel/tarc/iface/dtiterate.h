#pragma once

#include "tarcface.h"

#include <util/generic/vector.h>
#include <util/system/defaults.h>
#include <util/stream/zlib.h>
#include <util/stream/mem.h>
#include <util/ysaveload.h>

/*
class THandler
{
public:
    // return false to break the document iteration
    bool OnHeader(const TArchiveTextHeader* hdr) {
        return true;
    }

    // return false to break the document iteration
    bool OnMarkupInfo(const void* markupInfo, size_t markupInfoLen) {
        return true;
    }

    // return false to skip block
    bool OnBeginExtendedBlock(const TArchiveTextBlockInfo& b) {
        return true;
    }

    // return false to break the document iteration
    bool OnEndExtendedBlock() {
        return true;
    }

    // return false to skip block
    bool OnBeginBlock(ui16 prevSentCount, const TArchiveTextBlockInfo& b) {
        return true;
    }

    // return false to skip block
    bool OnWeightZones(TMemoryInput* ptr) {
        return true;
    }

    // return false to skip the rest of block
    bool OnSent(size_t sentNum, ui16 sentFlag, const void* sentBytes, size_t sentBytesLen) {
        return true;
    }

    // return false to break the document iteration
    bool OnEndBlock() {
        return true;
    }

    void OnEnd() {
    }
};
*/

template<class TDocTextHandler, bool SaveSentInfos = false>
void IterateArchiveDocText(const ui8* data, TDocTextHandler& handler) {
    TArchiveTextHeader* hdr = (TArchiveTextHeader*)data;
    if (!handler.OnHeader(hdr) || !hdr) {
        handler.OnEnd();
        return;
    }
    data += sizeof(TArchiveTextHeader);

    if (hdr->InfoLen && !handler.OnMarkupInfo(data, hdr->InfoLen)) {
        handler.OnEnd();
        return;
    }
    data += hdr->InfoLen;

    TArchiveTextBlockInfo* blockInfos = (TArchiveTextBlockInfo*)data;
    data += hdr->BlockCount * sizeof(TArchiveTextBlockInfo);

    ui32 offset = 0;
    ui16 sentCount = 0;
    TBufferOutput block;
    TVector<char> tempBuffer;
    TStringBuf sentInfos;

    for (ui16 i1 = 0; i1 < hdr->BlockCount; i1++) {
        block.Buffer().Clear();
        const TArchiveTextBlockInfo& b = blockInfos[i1];
        if (b.BlockFlag & BLOCK_IS_EXTENDED) {
            if (handler.OnBeginExtendedBlock(b)) {
                IterateArchiveDocText(data + offset, handler);
                if (!handler.OnEndExtendedBlock())
                    break;
            }
            offset = b.EndOffset;
            sentCount = sentCount + b.SentCount;
            continue;
        }
        if (handler.OnBeginBlock(sentCount, b)) {
            {
                TMemoryInput in(data + offset, b.EndOffset - offset);
                TZLibDecompress decompressor(&in);
                TransferData(&decompressor, &block);
            }

            if (block.Buffer().Size()) {
                TMemoryInput in(block.Buffer().Data(), block.Buffer().Size());
                {
                    const char* weightZonesBegin = in.Buf();
                    TArchiveWeightZones::Skip(&in);
                    const char* weightZonesEnd = in.Buf();
                    TMemoryInput weightZonesStream(
                                weightZonesBegin,
                                weightZonesEnd - weightZonesBegin);
                    if (!handler.OnWeightZones(&weightZonesStream)) {
                        continue;
                    }
                }

                LoadArchiveTextBlockSentInfos(&in, &sentInfos, &tempBuffer);
                Y_ASSERT(sentInfos.size() % sizeof(TArchiveTextBlockSentInfo) == 0);

                TConstArrayRef<TArchiveTextBlockSentInfo> sentInfoArray(
                    reinterpret_cast<const TArchiveTextBlockSentInfo*>(sentInfos.data()),
                    sentInfos.size() / sizeof(TArchiveTextBlockSentInfo));

                if constexpr (SaveSentInfos) {
                    if (!handler.OnSentInfos(sentInfoArray)) {
                        continue;
                    }
                }

                const char* sentBlob = in.Buf();
                const size_t sentBlobSize = in.Avail();
                in.Skip(sentBlobSize);
                Y_ASSERT(in.Exhausted());

                ui32 sOffset = 0;
                for (size_t i2 = 0; i2 < sentInfoArray.size(); ++i2) {
                    const TArchiveTextBlockSentInfo& si = sentInfoArray[i2];
                    Y_ASSERT(sOffset <= sentBlobSize);
                    Y_ASSERT(sOffset + (si.EndOffset - sOffset) <= sentBlobSize);
                    if (!handler.OnSent(
                                sentCount + i2 + 1,
                                si.Flag,
                                sentBlob + sOffset,
                                si.EndOffset - sOffset))
                    {
                        break;
                    }
                    sOffset = si.EndOffset;
                }
            }

            if (!handler.OnEndBlock())
                break;
        }

        offset = b.EndOffset;
        sentCount = sentCount + b.SentCount;
    }

    handler.OnEnd();
}
