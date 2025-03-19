#pragma once

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/system/mlock.h>
#include <util/system/rwlock.h>
#include <util/system/sysstat.h>
#include <util/thread/factory.h>


template<class TReloadableFile>
class TFileReloader : public IThreadFactory::IThreadAble {
public:
    typedef TAtomicSharedPtr<TReloadableFile> TReloadableFilePtr;

    TFileReloader(TReloadableFilePtr& reloadableFilePtr,
                  TRWMutex& fileLock,
                  const TDuration checkInterval = TDuration::Seconds(5))
        : FilePtr_(reloadableFilePtr)
        , FileLock_(fileLock)
        , CheckInterval_(checkInterval)
        , Stop_(false)
    {
        struct stat fileStat;
        if (stat(FilePtr_->GetFileName().data(), &fileStat) == 0) {
            InodeNumber_ = fileStat.st_ino;
        } else {
            // Can't get status of file.
            // Failing here seems to be overkill, just ignore the error.
            InodeNumber_ = 0;
        }
        ThreadRef_ = SystemThreadFactory()->Run(this);
    }

    ~TFileReloader() override {
        Stop_ = true;
        ThreadRef_->Join();
    }

private:
    void DoExecute() override {
        // TODO(rdna@): Avoid polling.
        while (!Stop_) {
            if (IsFileChanged(FilePtr_->GetFileName())) {
                TReloadableFilePtr newFilePtr;
                newFilePtr.Reset(new TReloadableFile());
                newFilePtr->Map(FilePtr_->GetFileName().data(),
                                         FilePtr_->GetPrechargeMode());
                {
                    // Reload file.
                    TWriteGuard writeGuard(FileLock_);
                    FilePtr_.Swap(newFilePtr);
                }
            }
            sleep(CheckInterval_.Seconds());
        }
    }

    bool IsFileChanged(const TString fileName) {
        struct stat fileStat;
        if (stat(fileName.data(), &fileStat) != 0) {
            // There is some error. Ignore it.
            return false;
        }
        if (!InodeNumber_) {
            // This is the first successful check.
            InodeNumber_ = fileStat.st_ino;
            return false;
        } else if (fileStat.st_ino != InodeNumber_) {
            fprintf(stderr, "===> File %s has been changed.\n", fileName.data());
            InodeNumber_ = fileStat.st_ino;
            return true;
        }
        // File is the same.
        return false;
    }

    TReloadableFilePtr&             FilePtr_;
    TRWMutex&                       FileLock_;
    TDuration                       CheckInterval_;
    ino_t                           InodeNumber_;
    volatile bool                   Stop_;
    TAutoPtr<IThreadFactory::IThread>  ThreadRef_;
};
