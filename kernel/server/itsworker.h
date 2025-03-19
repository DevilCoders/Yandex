#pragma once

#include "config.h"

#include <util/datetime/base.h>
#include <util/memory/blob.h>
#include <util/thread/factory.h>

namespace NServer {
    class TServer;

    class TITSWorker: public IThreadFactory::IThreadAble {
    public:
        TITSWorker(ui64 ratePerMinute,
                   TServer* parentWebdaemon,
                   const TString& itsConfigPath);
        ~TITSWorker() override = default;

    private:
        void DoExecute() override;
        void CheckDynamicConfig();

    private:
        virtual void UpdateConfig(const TBlob& content);

    private:
        TString LastConfigChecksum;
        TSharedItsConfigBlob::TPtr ItsConfigBlob;
        const TServer* ParentWebdaemon;
        const TDuration MinWaitDuration;
        const TString ItsConfigPath;
    };

} // namespace NServer
