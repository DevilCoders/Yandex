#include <kernel/multipart_archive/abstract/data_accessor.h>

#include <util/system/file.h>
#include <util/stream/direct_io.h>

#include <util/generic/size_literals.h>

#include <library/cpp/streams/special/throttled.h>
#include <library/cpp/logger/global/global.h>

namespace NRTYArchive {
    template <class TFileType>
    class TFileAccessorBase: public IWritableDataAccessor {
    public:
        TFileAccessorBase(const TFsPath& path, EOpenMode mode, TMaybe<ui64> sizeLimit, bool preallocationFlag)
            : File(path, mode)
            , Path(path)
            , Preallocated(false)
        {
            if ((mode & MaskRW) == RdWr && sizeLimit && preallocationFlag) {
                constexpr ui64 SAFE_SIZE = 2_GB;
                Y_ENSURE(sizeLimit <= SAFE_SIZE);
                File.FallocateNoResize(*sizeLimit.Get());
                DEBUG_LOG << "File " << Path << " preallocated" << Endl;
                Preallocated = true;
            }
        }

        TBlob Read(TSize size, TOffset offset) final {
            TSize result = 0;
            TBuffer buffer(size);
            buffer.Fill(0, size);

            while (size) {
                ui32 read = Min<TSize>(Max<ui32>(), size);
                read = File.Pread(buffer.Data() + result, read, offset);
                if (!read)
                    break;
                size -= read;
                result += read;
                offset += read;
            }

            buffer.Resize(result);
            return TBlob::FromBuffer(buffer);
        }

        ~TFileAccessorBase() {
            if (Preallocated) {
                try {
                    File.ShrinkToFit();
                    DEBUG_LOG << "File " << Path << " shrinked" << Endl;
                } catch (const yexception &e) {
                    ERROR_LOG << "Can not shrink file: " << e.what() << Endl;
                }
            }
        }

        bool IsBuffered() const final {
            return false;
        }

    protected:
        TFileType File;
        TFsPath Path;
        bool Preallocated;
    };

    class TFile : public TFileAccessorBase<::TFile> {
    public:
        TFile(const TFsPath& path, const TConstructContext& context, bool readOnly)
            : TFileAccessorBase(path, readOnly ? RdOnly : RdWr | CreateAlways, context.SizeLimit, context.PreallocationFlag)
        {
            WritePosition = File.GetLength();
        }

        TSize GetSize() const override {
            return WritePosition;
        }

        TOffset Write(const void* buffer, TSize size) override {
            TOffset oldPosition = WritePosition;
            File.Pwrite(buffer, size, WritePosition);
            WritePosition += size;
            return oldPosition;
        }

        void Flush() override {
            File.Flush();
        }

        static IDataAccessor::TFactory::TRegistrator<TFile> Registrator;
        static IWritableDataAccessor::TFactory::TRegistrator<TFile> RegistratorW;
    private:
        TOffset WritePosition = 0;
    };

    IDataAccessor::TFactory::TRegistrator<TFile> TFile::Registrator(IDataAccessor::FILE);
    IWritableDataAccessor::TFactory::TRegistrator<TFile> TFile::RegistratorW(IDataAccessor::FILE);


    class TDirectFile: public TFileAccessorBase<TDirectIOBufferedFile> {
    public:
        TDirectFile(const TFsPath& path, const TConstructContext& context, bool readOnly)
            : TFileAccessorBase(path, (readOnly ? RdOnly : (RdWr | OpenAlways)) | Direct, context.SizeLimit, context.PreallocationFlag)
        {
            if (!readOnly) {
                THolder<IOutputStream> directOut = MakeHolder<TRandomAccessFileOutput>(File);
                Output.Reset(context.WriteSpeedLimit ?
                             new TThrottledOutputStream(directOut.Release(), context.WriteSpeedLimit) :
                             directOut.Release()
                );
            }
        }

        TSize GetSize() const override {
            return File.GetWritePosition();
        }

        TOffset Write(const void* buffer, TSize size) override {
            TOffset result = File.GetWritePosition();
            Output->Write(buffer, size);
            return result;
        }

        void Flush() final {
            File.FlushData();
        }

        static IDataAccessor::TFactory::TRegistrator<TDirectFile> RegistratorR;
        static IWritableDataAccessor::TFactory::TRegistrator<TDirectFile> RegistratorW;
    private:
        THolder<IOutputStream> Output;
    };

    IDataAccessor::TFactory::TRegistrator<TDirectFile> TDirectFile::RegistratorR(IDataAccessor::DIRECT_FILE);
    IWritableDataAccessor::TFactory::TRegistrator<TDirectFile> TDirectFile::RegistratorW(IDataAccessor::DIRECT_FILE);
}
