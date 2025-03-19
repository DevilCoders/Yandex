#include <kernel/multipart_archive/abstract/data_accessor.h>
#include <library/cpp/logger/global/global.h>
#include <util/generic/buffer.h>
#include <util/stream/buffer.h>

namespace NRTYArchive {

    class TMemory : public IWritableDataAccessor {
    public:
        TMemory(const TFsPath& /*path*/, const TConstructContext& /*context*/, bool readOnly) {
            if (!readOnly)
                Output.Reset(new TBufferOutput(Data));
        }

        TSize GetSize() const override {
            return Data.size();
        }

        TBlob Read(TSize size, TOffset offset) override {
            CHECK_WITH_LOG(offset <= Data.size());
            TSize toRead = Min<TSize>(size, Data.size() - offset);
            return TBlob::NoCopy(Data.data() + offset, toRead);
        }

        TOffset Write(const void* buffer, TSize size) override {
            TOffset result = Data.size();
            Output->Write(buffer, size);
            return result;
        }

        bool IsBuffered() const override {
            return true;
        }

        void Flush() override {
        }

        static IDataAccessor::TFactory::TRegistrator<TMemory> Registrator;

    private:
        TBuffer Data;
        THolder<IOutputStream> Output;
    };

    IDataAccessor::TFactory::TRegistrator<TMemory> TMemory::Registrator(IDataAccessor::MEMORY);

}
