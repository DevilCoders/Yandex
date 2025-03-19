#include "mutex_owner.h"

#include <kernel/multipart_archive/abstract/part.h>

#include <library/cpp/logger/global/global.h>
#include <util/folder/path.h>
#include <util/system/fs.h>

/* This part implementation treats the part file as a magic cookie. It carries the file around in HardLinkOrCopyTo
 * but never actually examines its contents. Dataless parts allow to perform a merge of multipart archives in the
 * absence of documents contents, that is, simply recalculate FATs.
 */

namespace NRTYArchive {

    class TArchivePartDataless : public IArchivePart {
    public:
        TArchivePartDataless(const TFsPath& path, const IPolicy& policy)
            : IArchivePart(policy.IsWritable())
            , Path(path)
        {
            if (policy.IsWritable()) {
                Path.Touch();
            } else {
                CHECK_WITH_LOG(path.Exists());
            }
        }

        ~TArchivePartDataless() override {
        }

        TBlob GetDocument(TOffset /*offset*/) const override {
            FAIL_LOG("Not supported in dataless parts");
        }

        bool HardLinkOrCopyTo(const TFsPath& path) const override {
            NFs::HardLinkOrCopy(Path, path);
            return true;
        }

        IIterator::TPtr CreateIterator() const override {
            FAIL_LOG("Not supported in dataless parts");
            return nullptr;
        }

        void Drop() override {
        }

        TOffset TryPutDocument(const TBlob& /*document*/) override {
            FAIL_LOG("Not supported in dataless parts");
            return 0;
        }

        bool IsFull() const override {
            return false;
        }

        ui64 GetSizeInBytes() const override {
            return 0;
        }

        void DoClose() override {
        }

        TOffset GetWritePosition() const override {
            FAIL_LOG("Not supported in dataless parts");
            return 0;
        }

        const TFsPath& GetPath() const override {
            return Path;
        }

        static IArchivePart::TFactory::TRegistrator<TArchivePartDataless> Registrator;
    private:
        TFsPath Path;
    };

    IArchivePart::TFactory::TRegistrator<TArchivePartDataless> TArchivePartDataless::Registrator(IArchivePart::DATALESS);
}
