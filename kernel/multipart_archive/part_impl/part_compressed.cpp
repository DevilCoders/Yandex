#include "mutex_owner.h"

#include <kernel/multipart_archive/abstract/part.h>
#include <kernel/multipart_archive/abstract/position.h>
#include <kernel/multipart_archive/compressor/compressor.h>
#include <library/cpp/logger/global/global.h>
#include <util/generic/buffer.h>
#include <util/stream/buffer.h>
#include <util/stream/mem.h>

namespace NRTYArchive {
    namespace {
        constexpr ui64 BLOCK_NUMBER_BITS = 10;
        constexpr ui64 BLOCK_OFFSET_BITS = TPosition::OffsetBits - BLOCK_NUMBER_BITS;
        constexpr ui64 BLOCK_NUMBER_SHIFT = 0;
        constexpr ui64 BLOCK_OFFSET_SHIFT = BLOCK_NUMBER_BITS;
    }

    #define FIELD_MASK(FIELD) ((1 << BLOCK_##FIELD##_BITS) - 1)
    #define UNPACK_FIELD(i, FIELD) ((i >> BLOCK_##FIELD##_SHIFT) & FIELD_MASK(FIELD))
    #define PACK_FIELD(i, FIELD) (i << BLOCK_##FIELD##_SHIFT)
    #define WRITE_BUFFER_LIMIT (1 << 15)
    #define BLOCK_NUMBER_LIMIT FIELD_MASK(NUMBER)

    class TArchivePartCompressed : public IArchivePart {
    private:
        struct TPosition {
            inline TPosition(IDataAccessor::TOffset offset, ui16 blockNum)
                : DataOffset(offset)
                , BlockNumber(blockNum)
            {}

            inline TPosition(IArchivePart::TOffset offset)
                : DataOffset(UNPACK_FIELD(offset, OFFSET))
                , BlockNumber(UNPACK_FIELD(offset, NUMBER))
            {}

            inline operator IArchivePart::TOffset() const {
                return PACK_FIELD(DataOffset, OFFSET) | PACK_FIELD(BlockNumber, NUMBER);
            }

            IDataAccessor::TOffset DataOffset;
            ui16 BlockNumber;
        };

        class TCompresedBlock {
        public:
            TCompresedBlock(const TCompressor& compressor)
                : Compressor(compressor)
            {}

            bool Empty() const {
                return Data.Empty();
            }

            inline ui16 GetDocsCount() const {
                return Offsets.size() + 1;
            }

            void Clear() {
                Offsets.clear();
                Data.Clear();
            }

            void Serialize(TBuffer& out) const {
                TBufferOutput bo(out);
                ui16 offsetsSize = Offsets.size();
                size_t dataSize = Data.size() + offsetsSize * sizeof(ui16) + sizeof(offsetsSize);
                THolder<IOutputStream> comp(Compressor.CreateCompressStream(bo, dataSize));
                comp->Write(&offsetsSize, sizeof(offsetsSize));
                if (offsetsSize)
                    comp->Write(Offsets.data(), offsetsSize * sizeof(ui16));
                comp->Write(Data.data(), Data.size());
                comp->Finish();
            }

            void Deserialize(const TBlob& data) {
                Clear();
                if (data.Empty())
                    return;
                try {
                    TMemoryInput mi(data.AsCharPtr(), data.Size());
                    THolder<IInputStream> dec(Compressor.CreateDecompressStream(mi));
                    ui16 offsetsSize;
                    if (dec->Load(&offsetsSize, sizeof(offsetsSize)) < sizeof(offsetsSize))
                        ythrow yexception() << "corrupted";
                    Offsets.resize(offsetsSize);
                    if (offsetsSize && dec->Load(Offsets.data(), offsetsSize * sizeof(ui16)) < offsetsSize * sizeof(ui16))
                        ythrow yexception() << "corrupted";
                    Data.Clear();
                    TBufferOutput bo(Data);
                    dec->ReadAll(bo);
                } catch (...) {
                    ERROR_LOG << CurrentExceptionMessage() << Endl;
                }
                for (size_t i = 0; i < Offsets.size(); ++i)
                    if (Offsets[i] >= Data.Size()) {
                        Offsets.resize(i);
                        return;
                    }
            }

            TBlob GetDocument(ui16 index) const {
                if (index > Offsets.size())
                    return TBlob();
                const size_t begin = index ? Offsets[index - 1] : 0;
                const size_t end = index < Offsets.size() ? Offsets[index] : Data.size();
                CHECK_WITH_LOG(end >= begin);
                CHECK_WITH_LOG(end <= Data.Size());
                const size_t size = end - begin;
                if (!size) {
                    return TBlob();
                }
                return TBlob::Copy(Data.Data() + begin, size);
            }

            bool PutDocument(const TBlob& document, ui16& index) {
                index = Offsets.size();
                Data.Append(document.AsCharPtr(), document.Size());
                if (Data.size() >= WRITE_BUFFER_LIMIT || Offsets.size() >= BLOCK_NUMBER_LIMIT - 1)
                    return true;
                VERIFY_WITH_LOG(Data.size() < Max<ui16>(), "data to0 big: %lu", Data.size());
                Offsets.push_back(Data.size());
                return false;
            }

        private:
            const TCompressor& Compressor;
            TVector<ui16> Offsets;
            TBuffer Data;
        };

        class TIterator : public IArchivePart::IIterator {
        public:
            TIterator(const TArchivePartCompressed& archive)
                : Archive(archive)
                , Slave(Archive.Slave->CreateIterator())
                , CBlock(Archive.Compressor)
                , CBlockLoaded(false)
                , DocLoaded(false)
                , CurrentPosition(FIELD_MASK(OFFSET), 0)
            {}

            TBlob GetDocument() const override {
                if (!CBlockLoaded) {
                    auto g = Archive.Lock.ReadGuard();
                    if (!Archive.IsClosed() && CurrentPosition.DataOffset == Archive.Slave->GetWritePosition()) {
                        return Archive.WriteBuffer.GetDocument(CurrentPosition.BlockNumber);
                    }
                    CBlock.Deserialize(Slave->GetDocument());
                    CBlockLoaded = true;
                }
                if (!DocLoaded) {
                    Doc = CBlock.GetDocument(CurrentPosition.BlockNumber);
                    DocLoaded = true;
                }
                return Doc;
            }

            bool SkipTo(IArchivePart::TOffset offset) override {
                if (CurrentPosition == offset)
                    return true;
                TPosition pos(offset);
                DocLoaded = false;
                if (pos.DataOffset != CurrentPosition.DataOffset) {
                    if (!Slave->SkipTo(pos.DataOffset))
                        return false;
                    CBlockLoaded = false;
                }
                CurrentPosition = pos;
                return true;
            }

            IDataAccessor::TOffset SkipNext() override {
                GetDocument();

                if (!Archive.IsClosed() && CurrentPosition.DataOffset == Archive.Slave->GetWritePosition()) {
                    CurrentPosition.BlockNumber++;
                    return CurrentPosition;
                }

                if (CurrentPosition.BlockNumber + 1 >= CBlock.GetDocsCount()) {
                    CurrentPosition.DataOffset = Slave->SkipNext();
                    CurrentPosition.BlockNumber = 0;
                    CBlockLoaded = false;
                } else {
                    if (CurrentPosition.BlockNumber + 1 < CBlock.GetDocsCount())
                        CurrentPosition.BlockNumber++;
                }

                DocLoaded = false;
                return CurrentPosition;
            }

            bool CheckOffsetNext(TOffset offset) override {
                TPosition pos(offset);
                return (pos.DataOffset == CurrentPosition.DataOffset && pos.BlockNumber == CurrentPosition.BlockNumber + 1)
                       || ((Slave->CheckOffsetNext(pos.DataOffset) && pos.BlockNumber == 0));
            }

        private:
            const TArchivePartCompressed& Archive;
            IArchivePart::IIterator::TPtr Slave;
            mutable TCompresedBlock CBlock;
            mutable TBlob Doc;
            mutable bool CBlockLoaded;
            mutable bool DocLoaded;
            TPosition CurrentPosition;
        };

    public:
        TArchivePartCompressed(const TFsPath& path, const IPolicy& policy)
            : IArchivePart(!policy.IsWritable())
            , Compressor(policy.GetContext())
            , WriteBuffer(Compressor)
            , Lock(!policy.IsWritable())
        {
            Slave = TFactory::Construct(RAW, path, policy);
        }

        TBlob GetDocument(TOffset offset) const override {
            auto g = Lock.ReadGuard();
            TPosition pos(offset);
            if (!IsClosed() && pos.DataOffset == Slave->GetWritePosition())
                return WriteBuffer.GetDocument(pos.BlockNumber);
            TCompresedBlock cb(Compressor);
            cb.Deserialize(Slave->GetDocument(pos.DataOffset));
            return cb.GetDocument(pos.BlockNumber);
        }

        bool HardLinkOrCopyTo(const TFsPath& path) const override {
            auto g = Lock.WriteGuard();
            CloseWriteBuffer();
            return Slave->HardLinkOrCopyTo(path);
        }

        IIterator::TPtr CreateIterator() const override {
            return new TIterator(*this);
        }

        void Drop() override {
            auto g = Lock.WriteGuard();
            Slave->Drop();
            WriteBuffer.Clear();
        }

        TOffset TryPutDocument(const TBlob& document) override {
            auto g = Lock.WriteGuard();
            if (IsFullUnsafe()) {
                return InvalidOffset;
            }
            TPosition curPos(Slave->GetWritePosition(), 0);
            if (WriteBuffer.PutDocument(document, curPos.BlockNumber))
                CloseWriteBuffer();
            return curPos;
        }

        bool IsFullUnsafe() const {
            return Slave->IsFull() || Slave->GetWritePosition() >= FIELD_MASK(OFFSET);
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
            CloseWriteBuffer();
            Slave->Close();
            Lock.SetReadOnly();
        }

        TOffset GetWritePosition() const override {
            auto g = Lock.ReadGuard();
            return TPosition(Slave->GetWritePosition(), WriteBuffer.GetDocsCount());
        }

        const TFsPath& GetPath() const override {
            return Slave->GetPath();
        }

        static IArchivePart::TFactory::TRegistrator<TArchivePartCompressed> Registrator;

    private:
        void CloseWriteBuffer() const {
            if (WriteBuffer.Empty())
                return;
            TBuffer out;
            WriteBuffer.Serialize(out);
            if (Y_UNLIKELY(Slave->TryPutDocument(TBlob::FromBuffer(out)) == InvalidOffset)) {
                // should not be possible cause we check Slave->GetWritePosition() before accepting a document
                ERROR_LOG << "CloseWriteBuffer failed, write position = " << Slave->GetWritePosition() << Endl;
                Y_ASSERT(false);
                return;
            }
            WriteBuffer.Clear();
        }

        IArchivePart::TPtr Slave;
        TCompressor Compressor;
        mutable TCompresedBlock WriteBuffer;
        TMutexOwner Lock;
    };

    IArchivePart::TFactory::TRegistrator<TArchivePartCompressed> TArchivePartCompressed::Registrator(IArchivePart::COMPRESSED);

}
