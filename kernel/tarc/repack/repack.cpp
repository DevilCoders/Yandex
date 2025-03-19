#include "repack.h"

#include <kernel/tarc/iface/dtiterate.h>

namespace NRepack {

template <class Coder>
class TRepacker : private Coder
{
public:
    template <class... Args>
    explicit TRepacker(Args&&... args) : Coder(std::forward<Args>(args)...) {
        HeaderAndBlockInfoBuffer_.Clear();
        RepackedBlocksBuffer_.Clear();
    }

    bool OnHeader(const TArchiveTextHeader* hdr) {
        HeaderAndBlockInfo_.Write(hdr, sizeof(TArchiveTextHeader));
        return true;
    }

    bool OnMarkupInfo(const void* markupInfo, size_t markupInfoLen) {
        TBufferOutput markupInfoUnpacked;
        TMemoryInput in(markupInfo, markupInfoLen);
        TZLibDecompress decompressor(&in);
        TransferData(&decompressor, &markupInfoUnpacked);

        return OnUnpackedMarkupInfo(markupInfoUnpacked.Buffer().Data(), markupInfoUnpacked.Buffer().Size());
    }

    bool OnUnpackedMarkupInfo(const void* markupInfo, size_t markupInfoLen) {
        Coder::OnUnpackedMarkupInfo(markupInfo, markupInfoLen, &HeaderAndBlockInfo_);
        reinterpret_cast<TArchiveTextHeader*>(HeaderAndBlockInfo_.Buffer().Data())->InfoLen = HeaderAndBlockInfo_.Buffer().Size() - sizeof(TArchiveTextHeader);
        return true;
    }

    bool OnBeginExtendedBlock(const TArchiveTextBlockInfo&) {
        Y_ENSURE(false, "Expected no extended blocks in text archive");
        return false;
    }

    bool OnEndExtendedBlock() { return false; }

    bool OnBeginBlock(ui16 /*prevSentCount*/, const TArchiveTextBlockInfo& b) {
        SentsBuffer_.Clear();
        LastBlockInfo_ = b;
        return true;
    }

    bool OnWeightZones(TMemoryInput* in) {
        Coder::OnWeightZones(in, &RepackedBlocks_);
        return true;
    }

    bool OnSentInfos(TConstArrayRef<TArchiveTextBlockSentInfo> infos) {
        Coder::OnSentInfos(infos, &RepackedBlocks_);
        return true;
    }

    bool OnSent(size_t, ui16, const void* sentBytes, size_t sentBytesLen) {
        Sents_.Write(reinterpret_cast<const char*>(sentBytes), sentBytesLen);
        return true;
    }

    bool OnEndBlock() {
        Coder::OnEndBlock(Sents_.Buffer(), &RepackedBlocks_);

        LastBlockInfo_.EndOffset = RepackedBlocks_.Buffer().Size();
        HeaderAndBlockInfo_.Write(&LastBlockInfo_, sizeof(TArchiveTextBlockInfo));

        return true;
    }

    void OnEnd() {
        const size_t headerSize = HeaderAndBlockInfo_.Buffer().Size();
        const size_t blocksSize = RepackedBlocks_.Buffer().Size();

        Repack_.Buffer().Reserve(headerSize + blocksSize);
        Repack_.Write(HeaderAndBlockInfo_.Buffer().Data(), headerSize);
        Repack_.Write(RepackedBlocks_.Buffer().Data(), blocksSize);
    }

    TBlob GetRepack() {
        return TBlob::FromBuffer(Repack_.Buffer());
    }

protected:
    thread_local static inline TBuffer HeaderAndBlockInfoBuffer_ = {}, RepackedBlocksBuffer_ = {}, SentsBuffer_ = {}; 

    TBufferOutput HeaderAndBlockInfo_{HeaderAndBlockInfoBuffer_};
    TBufferOutput RepackedBlocks_{RepackedBlocksBuffer_};

    TBufferOutput Sents_{SentsBuffer_};
    TArchiveTextBlockInfo LastBlockInfo_;

    TBufferOutput Repack_;
};

class TEncode {
public:
    TEncode(TCodec& codec) : Codec_(codec) {
    }

    void OnUnpackedMarkupInfo(const void* markupInfo, size_t markupInfoLen, IOutputStream* output) {
        TStringBuf buf(reinterpret_cast<const char*>(markupInfo), markupInfoLen);
        TBuffer out = Codec_.EncodeMarkup(buf);
        output->Write(out.Data(), out.Size());
    }

    void OnWeightZones(TMemoryInput* in, IOutputStream* output) {
        TStringBuf buf(in->Buf(), in->Avail());
        TBuffer out = Codec_.EncodeWeightZones(buf);

        ui32 size = out.Size();
        output->Write(reinterpret_cast<const char*>(&size), sizeof(size));
        output->Write(out.Data(), size);
    }

    void OnSentInfos(TConstArrayRef<TArchiveTextBlockSentInfo> infos, IOutputStream* output) {
        TBuffer out = Codec_.EncodeBlocks(TStringBuf(reinterpret_cast<const char*>(infos.data()), infos.size() * sizeof(TArchiveTextBlockSentInfo)));

        ui32 size = out.Size();
        output->Write(reinterpret_cast<const char*>(&size), sizeof(size));
        output->Write(out.Data(), out.Size());
    }

    void OnEndBlock(const TBuffer& sents, IOutputStream* output) {
        TStringBuf buf(sents.Data(), sents.Size());
        TBuffer out = Codec_.EncodeSentences(buf);

        output->Write(out.Data(), out.Size());
    }

private:
    TCodec& Codec_;
};

class TDecode {
public:
    TDecode() = default;
    TDecode(size_t zLibCompLevel) : ZLibCompLevel_(zLibCompLevel) {
    }

    void OnUnpackedMarkupInfo(const void* markupInfo, size_t markupInfoLen, IOutputStream* output) {
        TZLibCompress(output, ZLib::StreamType::Auto, ZLibCompLevel_).Write(markupInfo, markupInfoLen);
    }

    void OnWeightZones(TMemoryInput* in, TBufferOutput*) {
        TransferData(in, &RepackedBlock_);
    }

    void OnSentInfos(TConstArrayRef<TArchiveTextBlockSentInfo> infos, TBufferOutput*) {
        SaveArchiveTextBlockSentInfos(infos, &RepackedBlock_);
    }

    void OnEndBlock(const TBuffer& sents, TBufferOutput* output) {
        RepackedBlock_.Write(sents.Data(), sents.Size());
        TZLibCompress(output, ZLib::StreamType::Auto, ZLibCompLevel_).Write(RepackedBlock_.Buffer().Data(), RepackedBlock_.Buffer().Size());

        BlockBuffer_.Clear();
    }

private:
    thread_local static inline TBuffer BlockBuffer_ = {}; 

    TBufferOutput RepackedBlock_{BlockBuffer_};
    size_t ZLibCompLevel_ = 0;
};

TBlob RepackArchiveDocText(const TBlob& arc, TCodec& codec) {
    TRepacker<TEncode> repacker(codec);
    IterateArchiveDocText<decltype(repacker), true>(arc.AsUnsignedCharPtr(), repacker);
    return repacker.GetRepack();
}

TBlob RestoreArchiveDocText(const TBlob& repack, TCodec& codec, size_t zLibCompLevel) {
    TRepacker<TDecode> repacker(zLibCompLevel);
    IterateRepackedArchiveDocText<decltype(repacker), true>(repack.AsUnsignedCharPtr(), codec, repacker);
    return repacker.GetRepack();
}

}
