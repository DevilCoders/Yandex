#pragma once
#include "config.h"
#include <kernel/common_server/server/server.h>
#include <kernel/common_server/solutions/server_template/src/abstract/server.h>

namespace NCS {
    namespace NServerTemplate {
        class TServer: public TBaseServer, public virtual IServerTemplate {
        private:
            using TBase = TBaseServer;
            const TServerConfig& Config;

        protected:
            virtual void DoRun() override;
            virtual void DoStop(ui32 rigidStopLevel, const TCgiParameters* cgiParams) override;

        public:
            using TConfig = TServerConfig;
            TServer(const TServerConfig& config);
            virtual void InitConstants(NFrontend::TConstantsInfoReport& cReport, TAtomicSharedPtr<IUserPermissions> permissions) const override;
            IUserPermissions::TPtr BuildPermissionFromItems(const TVector<TItemPermissionContainer>& items, const TString& userId) const override;
        };

    }
}
