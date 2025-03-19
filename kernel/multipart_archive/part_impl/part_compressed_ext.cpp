#include "part_compressed_ext.h"

#include "mutex_owner.h"

#include <kernel/multipart_archive/abstract/part.h>
#include <kernel/multipart_archive/abstract/position.h>

#include <library/cpp/codecs/codecs.h>
#include <library/cpp/logger/global/global.h>

#include <util/stream/buffer.h>


namespace NRTYArchive {
    namespace {
        const TStringBuf FallbackCodec = "lz4";

        constexpr ui64 BlockNumberBits = 12;
        constexpr ui64 BlockNumberMask = (1 << BlockNumberBits) - 1;
        constexpr ui64 OffsetBits = TPosition::OffsetBits - BlockNumberBits;
        constexpr ui64 MaxSlaveOffset = (1 << OffsetBits) - 1;

        constexpr ui64 GetBlockNumber(IDataAccessor::TOffset offset) {
            return offset & BlockNumberMask;
        }

        constexpr IDataAccessor::TOffset GetSlaveOffset(IDataAccessor::TOffset offset) {
            return offset >> BlockNumberBits;
        }

        constexpr ui64 MakeOffset(IDataAccessor::TOffset slaveOffset, ui64 blockNumber) {
            return (slaveOffset << BlockNumberBits) | blockNumber;
        }

        inline TStringBuf BlobAsStrBuf(const TBlob& blob) {
            return {blob.AsCharPtr(), blob.Size()};
        }
    }

    TBlobVector::TBlobVector(const NCodecs::ICodec& codec)
        : Codec(codec)
    {
    }

    TBlobVector::TBlobVector(const NCodecs::ICodec& codec, const TBlob& packedBlob)
        : Codec(codec)
    {
        LoadFromBlob(packedBlob);
    }

    void TBlobVector::LoadFromBlob(const TBlob& packedBlob) {
        UnpackBuffer.Clear();
        Offsets.clear();
        UpperBound = 0;
        if (packedBlob.Empty()) {
            return;
        }
        try {
            Codec.Decode(BlobAsStrBuf(packedBlob), UnpackBuffer);
        } catch (...) {
            CHECK_WITH_LOG(false) << CurrentExceptionMessage();
        }
        CHECK_WITH_LOG(UnpackBuffer.Size() >= sizeof(ui32));
        ui32 count;
        memcpy(&count, UnpackBuffer.Data() + UnpackBuffer.Size() - sizeof(ui32), sizeof(ui32));
        size_t tailSize = count * sizeof(ui32);
        CHECK_WITH_LOG(tailSize <= UnpackBuffer.Size());
        UpperBound = UnpackBuffer.Size() - tailSize;
        if (count > 1) {
            Offsets.resize(count - 1);
            memcpy(Offsets.data(), UnpackBuffer.Data() + UpperBound, tailSize - sizeof(ui32));
        }
    }

    size_t TBlobVector::GetCount() const {
        return UpperBound == UnpackBuffer.Size() ? 0 : Offsets.size() + 1;
    }

    std::pair<size_t, size_t> GetRange(size_t index, const ui32* offsets, size_t offsetsCount, size_t upperBound) {
        if (index > offsetsCount) {
            return {0, 0};
        }
        const size_t begin = index ? offsets[index - 1] : 0;
        const size_t end = index < offsetsCount ? offsets[index] : upperBound;
        return {begin, end};
    }

    TBlob TBlobVector::GetCopy(size_t index) const {
        if (Y_UNLIKELY(UpperBound == UnpackBuffer.Size())) {
            return {};
        }
        auto range = GetRange(index, Offsets.data(), Offsets.size(), UpperBound);
        if (range.first == range.second) {
            return {};
        }
        return TBlob::Copy(UnpackBuffer.Data() + range.first, range.second - range.first);
    }

    TBlob TBlobVector::Take(size_t index) {
        if (Y_UNLIKELY(UpperBound == UnpackBuffer.Size())) {
            return {};
        }
        auto range = GetRange(index, Offsets.data(), Offsets.size(), UpperBound);
        if (range.first == range.second) {
            return {};
        }
        auto fullBlob = TBlob::FromBuffer(UnpackBuffer);
        Offsets.clear();
        UpperBound = 0;
        return fullBlob.SubBlob(range.first, range.second);
    }

    TBlobVectorBuilder::TBlobVectorBuilder(const NCodecs::ICodec& codec, size_t sizeLimit, size_t countLimit)
        : Codec(codec)
        , SizeLimit(sizeLimit)
        , CountLimit(countLimit)
    {
    }

    bool TBlobVectorBuilder::Empty() const {
        return Count == 0;
    }

    size_t TBlobVectorBuilder::GetCount() const {
        return Count;
    }

    bool TBlobVectorBuilder::IsFull() const {
        return Buffer.Size() >= SizeLimit || Count >= CountLimit;
    }

    void TBlobVectorBuilder::Clear() {
        Buffer.Clear();
        Offsets.clear();
        Count = 0;
    }

    TBlob TBlobVectorBuilder::GetCopy(size_t index) const {
        auto range = GetRange(index, Offsets.data(), Offsets.size(), Buffer.Size());
        if (range.first == range.second) {
            return {};
        }
        return TBlob::Copy(Buffer.Data() + range.first, range.second - range.first);
    }

    size_t TBlobVectorBuilder::Put(const TBlob& blob) {
        if (Count > 0) {
            Offsets.push_back(Buffer.Size());
        }
        Buffer.Append(blob.AsCharPtr(), blob.Size());
        return Count++;
    }

    void TBlobVectorBuilder::SaveToBuffer(TBuffer& out) const {
        size_t BufferSize = Buffer.Size();
        CHECK_WITH_LOG(Count <= Max<ui32>());
        if (!Offsets.empty()) {
            Buffer.Append(reinterpret_cast<const char*>(Offsets.data()), Offsets.size() * sizeof(ui32));
        }
        ui32 count = Count;
        Buffer.Append(reinterpret_cast<const char*>(&count), sizeof(ui32));
        Codec.Encode(TStringBuf{Buffer.Data(), Buffer.Size()}, out);
        Buffer.Resize(BufferSize);
    }

    TFirstBlock::TFirstBlock(const TBlob& blob) {
        CHECK_WITH_LOG(blob.Size() > sizeof(ui32)) << blob.Size() << " <= " << sizeof(ui32);
        ui32 codecBlobSize;
        memcpy(&codecBlobSize, blob.Data(), sizeof(ui32));
        const size_t blobsStart = sizeof(ui32) + codecBlobSize;
        CHECK_WITH_LOG(blobsStart <= blob.Size()) << blobsStart << " > " << blob.Size();
        Codec = NCodecs::ICodec::RestoreFromString(TStringBuf{blob.AsCharPtr() + sizeof(ui32), codecBlobSize});
        CHECK_WITH_LOG(!!Codec);
        Blobs.ConstructInPlace(*Codec, TBlob::NoCopy(blob.AsCharPtr() + blobsStart, blob.Size() - blobsStart));
        CHECK_WITH_LOG(Blobs.Defined());
    }

    NCodecs::TCodecPtr TFirstBlock::GetCodec() const {
        return Codec;
    }

    TBlob TFirstBlock::GetBlobCopy(size_t index) const {
        return Blobs->GetCopy(index);
    }

    size_t TFirstBlock::GetCount() const {
        return Blobs->GetCount();
    }

    const size_t TCodecLearnQueue::MinimalLearnSize = 2048;

    TCodecLearnQueue::TCodecLearnQueue(NCodecs::TCodecPtr codec, size_t learnSize, size_t countLimit)
        : LearnSize(std::max(MinimalLearnSize, learnSize))
        , CountLimit(countLimit)
        , Codec(std::move(codec))
    {
    }

    TBlob TCodecLearnQueue::GetBlobCopy(size_t index) const {
        if (index >= Blobs.size()) {
            return {};
        }
        return Blobs[index];
    }

    size_t TCodecLearnQueue::PutBlob(const TBlob& blob) {
        TotalSize += blob.Size();
        Blobs.emplace_back(blob.DeepCopy());
        return Blobs.size() - 1;
    }

    bool TCodecLearnQueue::IsFull() const {
        return Codec->AlreadyTrained() || TotalSize >= LearnSize || Blobs.size() >= CountLimit;
    }

    size_t TCodecLearnQueue::GetCount() const {
        return Blobs.size();
    }

    struct TBlobDequeReader : NCodecs::ISequenceReader {
        explicit TBlobDequeReader(const TDeque<TBlob>& blobs)
            : Current(blobs.cbegin())
            , End(blobs.cend())
        {
        }

        bool NextRegion(TStringBuf& out) override {
            if (Current == End) {
                return false;
            }
            out = BlobAsStrBuf(*Current);
            ++Current;
            return true;
        }

    private:
        TDeque<TBlob>::const_iterator Current;
        TDeque<TBlob>::const_iterator End;
    };

    NCodecs::TCodecPtr TCodecLearnQueue::TrainCodec(TBuffer& out) {
        if (!Codec->AlreadyTrained()) {
            if (TotalSize < MinimalLearnSize) {
                NOTICE_LOG << "Learning sample is too small (" << TotalSize << "/" << MinimalLearnSize << "), fall back to " << FallbackCodec << Endl;
                Codec = NCodecs::ICodec::GetInstance(FallbackCodec);
            } else {
                TBlobDequeReader blobsReader(Blobs);
                if (!Codec->TryToLearn(blobsReader)) {
                    WARNING_LOG << "Unable to train codec " << Codec->GetName() << ", sample size: " << TotalSize << " bytes, " << Blobs.size() << " docs. Fall back to " << FallbackCodec << Endl;
                    Codec = NCodecs::ICodec::GetInstance(FallbackCodec);
                }
            }
        }
        CHECK_WITH_LOG(!!Codec);
        out.Resize(sizeof(ui32));
        TBufferOutput output(out);
        NCodecs::ICodec::Store(&output, Codec);
        CHECK_WITH_LOG(out.Size() <= Max<ui32>() + sizeof(ui32));
        ui32 codecBlobSize = static_cast<ui32>(out.Size() - sizeof(ui32));
        memcpy(out.Data(), &codecBlobSize, sizeof(ui32));

        TBlobVectorBuilder builder(*Codec, Max<ui32>(), Max<ui32>());
        for (auto& blob : Blobs) {
            builder.Put(blob);
        }
        TBuffer buffer;
        builder.SaveToBuffer(buffer);
        output.Write(buffer.Data(), buffer.Size());
        return Codec;
    }

    struct TArchivePartCompressedExt : IArchivePart {
        struct TIterator : IArchivePart::IIterator {
            TIterator(const TArchivePartCompressedExt& archive)
                : Archive(archive)
                , Slave(Archive.Slave->CreateIterator())
                , Block(*Archive.Codec)
            {
            }

            TBlob GetDocument() const override {
                if (!BlockLoaded) {
                    auto g = Archive.Lock.ReadGuard();
                    if (!Archive.IsClosed()) {
                        if (!Archive.Codec->AlreadyTrained()) {
                            if (SlaveOffset != 0) {
                                return {};
                            }
                            return Archive.LearnQueue->GetBlobCopy(BlockNumber);
                        }
                        if (SlaveOffset == Archive.Slave->GetWritePosition()) {
                            return Archive.BlockBuilder->GetCopy(BlockNumber);
                        }
                    }
                    if (SlaveOffset == 0) {
                        return Archive.FirstBlock->GetBlobCopy(BlockNumber);
                    }
                    g.Reset(nullptr);
                    Block.LoadFromBlob(Slave->GetDocument());
                }
                if (!DocLoaded) {
                    Doc = Block.GetCopy(BlockNumber);
                    DocLoaded = true;
                }
                return Doc;
            }

            bool SkipTo(IArchivePart::TOffset offset) override {
                auto newSlaveOffset = GetSlaveOffset(offset);
                auto newBlockNumber = GetBlockNumber(offset);
                if (newSlaveOffset == SlaveOffset && newBlockNumber == BlockNumber) {
                    return true;
                }
                DocLoaded = false;
                if (newSlaveOffset != SlaveOffset) {
                    if (!Slave->SkipTo(newSlaveOffset)) {
                        return false;
                    }
                    BlockLoaded = false;
                }
                SlaveOffset = newSlaveOffset;
                BlockNumber = newBlockNumber;
                return true;
            }

            IDataAccessor::TOffset SkipNext() override {
                GetDocument();
                if (!Archive.IsClosed()) {
                    if (!Archive.Codec->AlreadyTrained() || SlaveOffset == Archive.Slave->GetWritePosition()) {
                        BlockNumber++;
                        return MakeOffset(SlaveOffset, BlockNumber);
                    }
                }
                bool moveSlave = SlaveOffset == 0 && BlockNumber + 1 >= Archive.FirstBlock->GetCount() ||
                    SlaveOffset != 0 && BlockNumber + 1 >= Block.GetCount();
                if (!moveSlave) {
                    ++BlockNumber;
                } else {
                    SlaveOffset = Slave->SkipNext();
                    BlockNumber = 0;
                    BlockLoaded = false;
                }
                DocLoaded = false;
                return MakeOffset(SlaveOffset, BlockNumber);
            }

            bool CheckOffsetNext(TOffset offset) override {
                auto newSlaveOffset = GetSlaveOffset(offset);
                auto newBlockNumber = GetBlockNumber(offset);
                return SlaveOffset == newSlaveOffset && BlockNumber + 1 == newBlockNumber ||
                    Slave->CheckOffsetNext(newSlaveOffset) && BlockNumber == 0;
            }

        private:
            const TArchivePartCompressedExt& Archive;
            IArchivePart::IIterator::TPtr Slave;
            mutable TBlobVector Block;
            mutable TBlob Doc;
            mutable bool BlockLoaded = false;
            mutable bool DocLoaded = false;
            mutable TOffset SlaveOffset = MaxSlaveOffset;
            mutable ui32 BlockNumber = 0;
        };

        TArchivePartCompressedExt(const TFsPath& path, const IPolicy& policy)
            : IArchivePart(!policy.IsWritable())
            , BlockSizeLimit(policy.GetContext().Compression.ExtParams.BlockSize)
            , BlockCountLimit(BlockNumberMask)
            , Lock(!policy.IsWritable())
        {
            Slave = TFactory::Construct(RAW, path, policy);
            auto slaveIterator = Slave->CreateIterator();
            slaveIterator->SkipTo(0);
            auto blob = slaveIterator->GetDocument();
            if (!blob.Empty()) {
                FirstBlock.ConstructInPlace(blob);
                Codec = FirstBlock->GetCodec();
                CHECK_WITH_LOG(Codec->AlreadyTrained());
            } else {
                CHECK_WITH_LOG(!IsClosed()) << "the first block of a closed part should not be empty";
                auto codecName = policy.GetContext().Compression.ExtParams.CodecName;
                Codec = NCodecs::ICodec::GetInstance(codecName);
                CHECK_WITH_LOG(!!Codec);
                LearnQueue.ConstructInPlace(Codec, policy.GetContext().Compression.ExtParams.LearnSize, BlockCountLimit);
                if (Codec->AlreadyTrained()) {
                    CloseLearnQueue();
                    CHECK_WITH_LOG(!Slave->IsFull()) << "underlying RAW layer is already full with 0 documents written";
                }
            }
        }

        TBlob GetDocument(TOffset offset) const override {
            auto g = Lock.ReadGuard();
            auto slaveOffset = GetSlaveOffset(offset);
            auto blockNumber = GetBlockNumber(offset);
            if (!IsClosed()) {
                if (!Codec->AlreadyTrained()) {
                    if (slaveOffset != 0) {
                        return {};
                    }
                    return LearnQueue->GetBlobCopy(blockNumber);
                }
                if (slaveOffset == Slave->GetWritePosition()) {
                    return BlockBuilder->GetCopy(blockNumber);
                }
            }
            if (slaveOffset == 0) {
                return FirstBlock->GetBlobCopy(blockNumber);
            }
            TBlobVector block(*Codec, Slave->GetDocument(slaveOffset));
            return block.Take(blockNumber);
        }

        bool HardLinkOrCopyTo(const TFsPath& path) const override {
            auto g = Lock.WriteGuard();
            if (!IsClosed()) {
                CloseBuffers();
            }
            return Slave->HardLinkOrCopyTo(path);
        }

        IIterator::TPtr CreateIterator() const override {
            return new TIterator(*this);
        }

        void Drop() override {
            auto g = Lock.WriteGuard();
            Slave->Drop();
            LearnQueue.Clear();
            FirstBlock.Clear();
            BlockBuilder.Clear();
        }

        TOffset TryPutDocument(const TBlob& document) override {
            auto g = Lock.WriteGuard();
            if (IsFullUnsafe()) {
                return InvalidOffset;
            }
            if (Codec->AlreadyTrained()) {
                ui32 blockNumber = BlockBuilder->Put(document);
                TOffset offset = MakeOffset(Slave->GetWritePosition(), blockNumber);
                if (BlockBuilder->IsFull()) {
                    FlushBlockBuilder();
                }
                return offset;
            }
            ui32 blockNumber = LearnQueue->PutBlob(document);
            ui32 offset = MakeOffset(0, blockNumber);
            if (LearnQueue->IsFull()) {
                CloseLearnQueue();
            }
            return offset;
        }

        bool IsFullUnsafe() const {
            return Slave->IsFull() || Slave->GetWritePosition() >= MaxSlaveOffset;
        }

        bool IsFull() const override {
            auto g = Lock.ReadGuard();
            return IsFullUnsafe();
        }

        ui64 GetSizeInBytes() const override {
            auto g = Lock.ReadGuard();
            return Slave->GetSizeInBytes();
        }

        void DoClose() override {
            auto g = Lock.WriteGuard();
            CloseBuffers();
            Slave->Close();
            Lock.SetReadOnly();
        }

        TOffset GetWritePosition() const override {
            auto g = Lock.ReadGuard();
            if (Codec->AlreadyTrained()) {
                auto slaveOffset = Slave->GetWritePosition();
                return MakeOffset(slaveOffset, BlockBuilder->GetCount());
            }
            return MakeOffset(0, LearnQueue->GetCount());
        }

        const TFsPath& GetPath() const override {
            return Slave->GetPath();
        }

    private:
        void CloseBuffers() const {
            if (!Codec->AlreadyTrained()) {
                CloseLearnQueue();
                return;
            }
            if (!BlockBuilder.Defined() || BlockBuilder->Empty()) {
                return;
            }
            FlushBlockBuilder();
        }

        void CloseLearnQueue() const {
            BlockWriteBuffer.Clear();
            Codec = LearnQueue->TrainCodec(BlockWriteBuffer);
            auto firstBlockBlob = TBlob::NoCopy(BlockWriteBuffer.Data(), BlockWriteBuffer.Size());
            if (!IsClosed()) {
                // offset of the first block is always zero or something close to zero
                CHECK_WITH_LOG(Slave->TryPutDocument(firstBlockBlob) != InvalidOffset);
                LearnQueue.Clear();
                BlockBuilder.ConstructInPlace(*Codec, BlockSizeLimit, BlockCountLimit);
            }
            FirstBlock.ConstructInPlace(firstBlockBlob);
        }

        void FlushBlockBuilder() const {
            BlockWriteBuffer.Clear();
            BlockBuilder->SaveToBuffer(BlockWriteBuffer);
            auto writtenOffset = Slave->TryPutDocument(TBlob::NoCopy(BlockWriteBuffer.Data(), BlockWriteBuffer.Size()));
            if (Y_UNLIKELY(writtenOffset == InvalidOffset)) {
                // should not be possible cause we check Slave->GetWritePosition() before accepting a document
                ERROR_LOG << "FlushBlockBuilder failed, write position = " << Slave->GetWritePosition() << Endl;
                Y_ASSERT(false);
                return;
            }
            BlockBuilder->Clear();
        }

        const ui32 BlockSizeLimit;
        const ui32 BlockCountLimit;
        IArchivePart::TPtr Slave;
        // members below are mutable because HardLinkOrCopyTo is const
        mutable NCodecs::TCodecPtr Codec;
        mutable TMaybeFail<TCodecLearnQueue> LearnQueue;
        mutable TMaybeFail<TFirstBlock> FirstBlock;
        mutable TMaybeFail<TBlobVectorBuilder> BlockBuilder;
        mutable TBuffer BlockWriteBuffer;
        mutable TMutexOwner Lock;
    };

    namespace {
        IArchivePart::TFactory::TRegistrator<TArchivePartCompressedExt> Registrar(IArchivePart::COMPRESSED_EXT);
    }
}

