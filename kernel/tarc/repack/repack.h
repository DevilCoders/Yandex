#pragma once

#include <kernel/tarc/repack/codecs/codecs.h>
#include <kernel/tarc/iface/tarcface.h>

#include <util/generic/vector.h>
#include <util/system/defaults.h>
#include <util/stream/zlib.h>
#include <util/stream/mem.h>
#include <util/ysaveload.h>

namespace NRepack {

TBlob RepackArchiveDocText(const TBlob& arc, TCodec& codec);
TBlob RestoreArchiveDocText(const TBlob& repack, TCodec& codec, size_t zLibCompLevel = 0);

template<class TDocTextHandler, bool SaveSentInfos = false>
void IterateRepackedArchiveDocText(const ui8* data, TCodec& codec, TDocTextHandler& handler) {
    TArchiveTextHeader* hdr = (TArchiveTextHeader*)data;
    if (!handler.OnHeader(hdr) || !hdr) {
        handler.OnEnd();
        return;
    }
    data += sizeof(TArchiveTextHeader);

    if (hdr->InfoLen) {
        TStringBuf compressedMarkup(reinterpret_cast<const char*>(data), hdr->InfoLen);
        TBuffer decompressedMarkup = codec.DecodeMarkup(compressedMarkup);

        if (!handler.OnUnpackedMarkupInfo(decompressedMarkup.Data(), decompressedMarkup.Size())) {
            handler.OnEnd();
            return;
        }
    }
    data += hdr->InfoLen;

    TArchiveTextBlockInfo* blockInfos = (TArchiveTextBlockInfo*)data;
    data += hdr->BlockCount * sizeof(TArchiveTextBlockInfo);

    ui32 offset = 0;
    ui16 sentCount = 0;
    TBufferOutput block;

    for (ui16 i1 = 0; i1 < hdr->BlockCount; i1++) {
        block.Buffer().Clear();
        const TArchiveTextBlockInfo& b = blockInfos[i1];
        Y_ENSURE((b.BlockFlag & BLOCK_IS_EXTENDED) == 0); // a repack should not have extended blocks
        if (handler.OnBeginBlock(sentCount, b)) {
            TMemoryInput in(data + offset, b.EndOffset - offset);
            if (!in.Avail()) {
                if (!handler.OnEndBlock()) {
                    break;
                }
                else {
                    continue;
                }
            }            

            TStringBuf compressedZones(in.Buf() + sizeof(ui32), *reinterpret_cast<const ui32*>(in.Buf()));
            in.Skip(sizeof(ui32) + compressedZones.Size());

            TBuffer decompressedZones = codec.DecodeWeightZones(compressedZones);
            TMemoryInput weightZonesStream(decompressedZones.Data(), decompressedZones.Size());
            if (!handler.OnWeightZones(&weightZonesStream)) {
                continue;
            }

            TStringBuf encodedSentInfo(in.Buf() + sizeof(ui32), *reinterpret_cast<const ui32*>(in.Buf()));
            in.Skip(sizeof(ui32) + encodedSentInfo.size());

            TBuffer decodedSentInfos = codec.DecodeBlocks(encodedSentInfo);
            TConstArrayRef<TArchiveTextBlockSentInfo> sentInfos(
                reinterpret_cast<const TArchiveTextBlockSentInfo*>(decodedSentInfos.data()),
                decodedSentInfos.size() / sizeof(TArchiveTextBlockSentInfo));

            if constexpr (SaveSentInfos) {
                if (!handler.OnSentInfos(sentInfos)) {
                    continue;
                }
            }

            TStringBuf encodedSentBlob(in.Buf(), in.Avail());
            TBuffer decodedSentBlob = codec.DecodeSentences(encodedSentBlob);

            const char* sentBlob = decodedSentBlob.Data();
            const size_t sentBlobSize = decodedSentBlob.Size();

            ui32 sOffset = 0;
            for (size_t i2 = 0; i2 < sentInfos.size(); ++i2) {
                const TArchiveTextBlockSentInfo& si = sentInfos[i2];
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

            if (!handler.OnEndBlock())
                break;
        }

        offset = b.EndOffset;
        sentCount = sentCount + b.SentCount;
    }

    handler.OnEnd();
}

}
