#include "server.h"
#include <kernel/common_server/solutions/server_template/src/permissions/object.h>
namespace NCS {
    namespace NServerTemplate {
        TServer::TServer(const TServerConfig& config)
            : TBase(config)
            , Config(config) {
            Y_UNUSED(Config);
        }


        void TServer::DoStop(ui32 rigidStopLevel, const TCgiParameters* cgiParams) {
            TBase::DoStop(rigidStopLevel, cgiParams);
        }

        void TServer::DoRun() {
            TBase::DoRun();
        }

        void TServer::InitConstants(NFrontend::TConstantsInfoReport& cReport, TAtomicSharedPtr<IUserPermissions> permissions) const {
            TBase::InitConstants(cReport, permissions);
        }

        IUserPermissions::TPtr TServer::BuildPermissionFromItems(const TVector<TItemPermissionContainer>& items, const TString& userId) const {
            auto result = MakeHolder<TUserPermissions>(userId);
            result->AddAbilities(items);
            return result.Release();
        }
    }
}
