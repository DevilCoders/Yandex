#include "itsworker.h"
#include "server.h"

#include <library/cpp/digest/md5/md5.h>

#include <kernel/searchlog/errorlog.h>

#include <util/folder/path.h>

namespace NServer {
    TITSWorker::TITSWorker(ui64 ratePerMinute,
                           TServer* parentWebdaemon,
                           const TString& itsConfigPath)
        : ItsConfigBlob(SharedItsConfigBlob.AtomicLoad())
        , ParentWebdaemon(parentWebdaemon)
        , MinWaitDuration(TDuration::Minutes(1) / ratePerMinute)
        , ItsConfigPath(itsConfigPath)
    {
        Y_ASSERT(ParentWebdaemon);
        Y_ENSURE(ParentWebdaemon, "no parent webdaemon passed");
        Y_ASSERT(ratePerMinute);
        Y_ENSURE(ratePerMinute, "rate could not be zero");
    }

    void TITSWorker::DoExecute() {
        TThread::SetCurrentThreadName("ITSWorker");

        THolder<TITSWorker> harakiri(this);

        SEARCH_INFO << "[TITSWorker] worker started";
        while (ParentWebdaemon->IsRun()) {
            TInstant started = TInstant::Now();
            CheckDynamicConfig();
            TInstant completed = TInstant::Now();
            const TDuration elapsed = completed - started;
            if (elapsed < MinWaitDuration) {
                usleep((MinWaitDuration - elapsed).MicroSeconds());
            }
        }
        SEARCH_INFO << "[TITSWorker] worker stopped";
    }

    void TITSWorker::CheckDynamicConfig() {
        const TFsPath dynamicConfigFile(ItsConfigPath);
        if (dynamicConfigFile.IsFile()) {
            const TBlob content = TBlob::FromFileContentSingleThreaded(dynamicConfigFile.c_str());
            const TString curSum = MD5::Calc(TStringBuf(content.AsCharPtr(), content.Size()));
            if (LastConfigChecksum == curSum) {
                return;
            }

            SEARCH_INFO << "[TITSWorker] dynamic config " << dynamicConfigFile.c_str()
                        << ", prev " << LastConfigChecksum
                        << ", new " << curSum;

            UpdateConfig(content);
            LastConfigChecksum = curSum;
        }
    }

    void TITSWorker::UpdateConfig(const TBlob& content) {
        SharedItsConfigBlob.AtomicStore(new TRefCountBlobHolder(content));
    }

} // namespace NServer
