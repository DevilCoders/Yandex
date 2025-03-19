#include <kernel/multipart_archive/abstract/data_accessor.h>

#include <library/cpp/logger/global/global.h>

#include <util/stream/file.h>
#include <util/system/filemap.h>
#include <util/system/getpid.h>
#include <util/system/madvise.h>
#include <util/system/mlock.h>

namespace {
    class TMappedFileBase : public NRTYArchive::IDataAccessor {
    public:
        TMappedFileBase(const TFsPath& path, TFileMap::EOpenMode om = TFileMap::EOpenModeFlag::oRdOnly)
            : File(path, om)
        {
            File.Map(0, File.Length());
        }
        ~TMappedFileBase() = default;

        TSize GetSize() const override {
            return File.Length();
        }

        TBlob Read(TSize size, TOffset offset) override {
            ui64 len = File.Length();
            CHECK_WITH_LOG(offset <= len) << offset << "/" << len;
            TSize toRead = Min<TSize>(size, len - offset);
            return TBlob::NoCopy((const char*)File.Ptr() + offset, toRead);
        }

        bool IsBuffered() const override {
            return true;
        }

    protected:
        ::TFileMap File;
    };
}

namespace NRTYArchive {
    class TMappedFile : public TMappedFileBase {
    public:
        TMappedFile(const TFsPath& path, const TConstructContext&, bool readOnly)
            : TMappedFileBase(path)
        {
            VERIFY_WITH_LOG(readOnly, "invalid usage");
        }
        ~TMappedFile() = default;

    private:
        using TRegistrator = IDataAccessor::TFactory::TRegistrator<TMappedFile>;
        static TRegistrator Registrator;
    };

    TMappedFile::TRegistrator TMappedFile::Registrator(IDataAccessor::MEMORY_MAP);


    class TLockedMappedFile : public TMappedFileBase {
    public:
        TLockedMappedFile(const TFsPath& path, const TConstructContext&, bool readOnly)
            : TMappedFileBase(path)
        {
            VERIFY_WITH_LOG(readOnly, "invalid usage");
            try {
                ::LockMemory(File.Ptr(), File.MappedSize());
            } catch (const yexception& e) {
                TString status = TFileInput("/proc/" + ToString(GetPID()) + "/status").ReadAll();

                FATAL_LOG << "Can't lock mapped file " << path << " with size " << File.MappedSize()
                    << "(" << e.what() << ")" << Endl << status;
                throw e;
            }
        }

        ~TLockedMappedFile() override {
            try {
                ::UnlockMemory(File.Ptr(), File.MappedSize());
            } catch (const yexception& e) {
                ERROR_LOG << "Can't unlock mmaped region " << File.Ptr() << ": " << e.what() << Endl;
            }
        }

    private:
        using TRegistrator = IDataAccessor::TFactory::TRegistrator<TLockedMappedFile>;
        static TRegistrator Registrator;
    };

    TLockedMappedFile::TRegistrator TLockedMappedFile::Registrator(IDataAccessor::MEMORY_LOCKED_MAP);


    class TPrechargedMappedFile : public TMappedFileBase {
    public:
        TPrechargedMappedFile(const TFsPath& path, const TConstructContext&, bool readOnly)
            : TMappedFileBase(path, TFileMap::EOpenModeFlag::oRdOnly | TFileMap::EOpenModeFlag::oPrecharge)
        {
            VERIFY_WITH_LOG(readOnly, "invalid usage");
        }
        ~TPrechargedMappedFile() = default;

    private:
        using TRegistrator = IDataAccessor::TFactory::TRegistrator<TPrechargedMappedFile>;
        static TRegistrator Registrator;
    };

    TPrechargedMappedFile::TRegistrator TPrechargedMappedFile::Registrator(IDataAccessor::MEMORY_PRECHARGED_MAP);


    class TRandomAccessMappedFile : public TMappedFileBase {
    public:
        TRandomAccessMappedFile(const TFsPath& path, const TConstructContext&, bool readOnly)
            : TMappedFileBase(path)
        {
            VERIFY_WITH_LOG(readOnly, "invalid usage");
            try {
                ::MadviseRandomAccess(File.Ptr(), File.MappedSize());
            } catch (const yexception& e) {
                FATAL_LOG << "Error in MadviseRandomAccess for mapped file " << path
                          << " with size " << File.MappedSize() << "(" << e.what() << ")" << Endl;
                throw e;
            }
        }

    private:
        using TRegistrator = IDataAccessor::TFactory::TRegistrator<TRandomAccessMappedFile>;
        static TRegistrator Registrator;
    };

    TRandomAccessMappedFile::TRegistrator TRandomAccessMappedFile::Registrator(IDataAccessor::MEMORY_RANDOM_ACCESS_MAP);
}
