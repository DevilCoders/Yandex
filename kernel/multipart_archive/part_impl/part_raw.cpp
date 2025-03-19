#include "mutex_owner.h"

#include <kernel/multipart_archive/abstract/part.h>

#include <library/cpp/logger/global/global.h>
#include <library/cpp/streams/special/position.h>
#include <util/generic/buffer.h>
#include <util/stream/buffered.h>
#include <util/system/fs.h>

namespace NRTYArchive {

    class TArchivePartRaw : public IArchivePart {
    private:
        class TDataAccessorInputStream : public IInputStream {
        public:
            TDataAccessorInputStream(const IDataAccessor::TPtr dataAccessor)
                : DataAccessor(dataAccessor)
                , Offset(0)
            {}

        protected:
            size_t DoRead(void* buf, size_t len) override {
                TBlob data = DataAccessor->Read(len, Offset);
                Offset += data.Size();
                CHECK_WITH_LOG(data.Size() <= len);
                memcpy(buf, data.Data(), data.Size());
                return data.Size();
            }

            size_t DoSkip(size_t len) override {
                if (!len)
                    return 0;
                size_t size = DataAccessor->GetSize();
                if (Offset >= size)
                    return 0;
                size_t toSkip = Min<size_t>(len, size - Offset);
                Offset += toSkip;
                return toSkip;
            }

        private:
            IDataAccessor::TPtr DataAccessor;
            IDataAccessor::TOffset Offset;
        };

        class TIterator : public IArchivePart::IIterator {
        public:
            TIterator(const TArchivePartRaw& archive)
                : Archive(archive)
                , Input(new TBuffered<TDataAccessorInputStream>(1 << 17, Archive.File))
                , Offset(0)
                , Loaded(false)
            {}

            TBlob GetDocument() const override {
                if (!Loaded) {
                    Buffer = Archive.ReadDocumentFromStream(Input, Archive.WithoutSizes);
                    Loaded = true;
                }
                return Buffer;
            }

            bool SkipTo(TOffset offset) override {
                if (offset < Offset)
                    return false;
                if (Offset == offset)
                    return true;
                Loaded = false;
                auto g = Archive.Lock.ReadGuard();
                Offset = Input.SkipTo(offset);
                return Offset == offset;
            }

            IDataAccessor::TOffset SkipNext() override {
                ui64 dataLen = GetDocument().Size();

                if (dataLen != 0 && !Archive.WithoutSizes) {
                    dataLen += sizeof(ui32);
                }

                SkipTo(Offset + dataLen);
                return Offset;
            }

            bool CheckOffsetNext(TOffset offset) override {
                ui64 dataLen = GetDocument().Size();

                if (dataLen != 0 && !Archive.WithoutSizes) {
                    dataLen += sizeof(ui32);
                }

                return (dataLen != 0) && (offset == Offset + dataLen);
            }

        private:
            const TArchivePartRaw& Archive;
            mutable TPositionStream Input;
            IDataAccessor::TOffset Offset;
            mutable TBlob Buffer;
            mutable bool Loaded;
        };

    public:
        TArchivePartRaw(const TFsPath& path, const IPolicy& policy)
            : IArchivePart(policy.IsWritable())
            , WritableFile(nullptr)
            , Path(path)
            , Lock(!policy.IsWritable())
            , WithoutSizes(policy.GetContext().WithoutSizes)
            , Dropped(false)
            , Policy(policy)
        {
            if (!Policy.IsWritable()) {
                CHECK_WITH_LOG(path.Exists());
                File = IDataAccessor::Create(path, Policy.GetContext().DataAccessorContext);
            } else {
                IWritableDataAccessor::TPtr wf(IWritableDataAccessor::Create(path, Policy.GetContext().GetWritableContext().DataAccessorContext));
                WritableFile = wf.Get();
                File = wf.Release();
            }
        }

        ~TArchivePartRaw() override {
            File.Drop();
            if (Dropped) {
                VERIFY_WITH_LOG(NFs::Remove(Path), "cannot remove %s", Path.GetPath().data());
            }
        }

        TBlob GetDocument(TOffset offset) const override {
            auto g = Lock.ReadGuard();
            const ui32 sizeLen = sizeof(ui32);

            TBlob sizeBlob = File->Read(sizeLen, offset);
            if (sizeBlob.Size() != sizeLen)
                return TBlob();

            ui32 size;
            memcpy(&size, sizeBlob.Data(), sizeLen);

            TBlob dataBlob(File->Read(size, WithoutSizes ? offset : (offset + sizeLen)));

            if (dataBlob.Size() != size)
                return TBlob();

            return dataBlob;
        }

        bool HardLinkOrCopyTo(const TFsPath& path) const override {
            auto g = Lock.WriteGuard();
            if (!!WritableFile)
                WritableFile->Flush();

            if (Dropped)
                return false;

            NFs::HardLinkOrCopy(Path, path);
            return true;
        }

        IIterator::TPtr CreateIterator() const override {
            auto g = Lock.ReadGuard();
            return new TIterator(*this);
        }

        void Drop() override {
            auto g = Lock.WriteGuard();
            WritableFile = nullptr;
            Dropped = true;
        }

        TOffset TryPutDocument(const TBlob& document) override {
            auto g = Lock.WriteGuard();
            if (IsFullUnsafe()) {
                return TPosition::InvalidOffset;
            }
            VERIFY_WITH_LOG(!Dropped, "Part was dropped %s", GetPath().GetPath().data());
            VERIFY_WITH_LOG(WritableFile, "cannot write to readonly archive part");
            return WriteDocumentBlob(document);
        }

        bool IsFullUnsafe() const {
            return File->GetSize() >= Policy.GetContext().SizeLimit;
        }

        bool IsFull() const override {
            auto g = Lock.ReadGuard();
            return IsFullUnsafe();
        }

        ui64 GetSizeInBytes() const override {
            auto g = Lock.ReadGuard();
            return !!File ? File->GetSize() : 0;
        }

        void DoClose() override {
            TConstructContext context = Policy.GetContext();
            auto g = Lock.WriteGuard();
            VERIFY_WITH_LOG(!Dropped, "Part was dropped %s", GetPath().GetPath().data());
            WritableFile->Flush();
            File.Drop();
            CHECK_WITH_LOG(Path.Exists());
            File.Reset(IDataAccessor::Create(Path, context.DataAccessorContext));
            WritableFile = nullptr;
            Lock.SetReadOnly();
        }

        TOffset GetWritePosition() const override {
            auto g = Lock.ReadGuard();
            return File->GetSize();
        }

        const TFsPath& GetPath() const override {
            return Path;
        }

        static IArchivePart::TFactory::TRegistrator<TArchivePartRaw> Registrator;
    private:
        inline static TBlob ReadDocumentFromStream(IInputStream& in, bool withoutSizes = false) {
            ui32 size;
            TSize readed = in.Load(&size, sizeof(size));
            if (readed != sizeof(size))
                return TBlob();
            TBuffer buf;
            buf.Resize(size);
            ui32 sizeLen = withoutSizes ? sizeof(size) : 0;
            if (withoutSizes) {
                VERIFY_WITH_LOG(size >= 4, "Incorrect data size: archive broken");
                *reinterpret_cast<ui32*>(buf.data()) = size;
            }
            readed = in.Load(buf.data() + sizeLen, size - sizeLen);
            if (readed < size - sizeLen)
                return TBlob();
            return TBlob::FromBuffer(buf);
        }

        TOffset WriteDocumentBlob(const TBlob& document) {
            if (WithoutSizes) {
                VERIFY_WITH_LOG(document.Size() >= sizeof(ui32), "Trying to write broken data");
                return WritableFile->Write(document.AsCharPtr(), document.Size());
            } else {
                ui32 size = document.Size();
                TOffset res = WritableFile->Write(&size, sizeof(size));
                WritableFile->Write(document.AsCharPtr(), document.Size());
                return res;
            }
        }

        IDataAccessor::TPtr File;
        IWritableDataAccessor* WritableFile;
        TFsPath Path;
        TMutexOwner Lock;
        bool WithoutSizes;
        bool Dropped;
        const IArchivePart::IPolicy& Policy;
    };

    IArchivePart::TFactory::TRegistrator<TArchivePartRaw> TArchivePartRaw::Registrator(IArchivePart::RAW);
}
