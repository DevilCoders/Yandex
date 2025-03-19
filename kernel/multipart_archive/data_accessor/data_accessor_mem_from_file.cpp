#include <kernel/multipart_archive/abstract/data_accessor.h>
#include <library/cpp/logger/global/global.h>
#include <util/memory/blob.h>

namespace NRTYArchive {

    class TMemoryFromFile : public IDataAccessor {
    public:
        TMemoryFromFile(const TFsPath& path, const TConstructContext& /*context*/, bool readOnly)
            : Data(TBlob::FromFileContent(path))
        {
            VERIFY_WITH_LOG(readOnly, "invalid usage");
        }

        TSize GetSize() const override {
            return Data.Size();
        }

        TBlob Read(TSize size, TOffset offset) override {
            CHECK_WITH_LOG(offset <= Data.Size());
            TSize toRead = Min<TSize>(size, Data.Size() - offset);
            return TBlob::NoCopy(Data.AsCharPtr() + offset, toRead);
        }

        bool IsBuffered() const override {
            return true;
        }

        static IDataAccessor::TFactory::TRegistrator<TMemoryFromFile> Registrator;

    private:
        TBlob Data;
    };

    IDataAccessor::TFactory::TRegistrator<TMemoryFromFile> TMemoryFromFile::Registrator(IDataAccessor::MEMORY_FROM_FILE);

}
