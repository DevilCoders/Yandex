#pragma once

#include <yt/yt/client/api/public.h>
#include <yt/yt/client/api/rpc_proxy/connection.h>

namespace NYT {
    namespace NApi {
        void ConfigureAddressResolver();
        NRpcProxy::TConnectionConfigPtr GetConnectionConfiguration(const TString& serverName, const TString& rpcProxyRole = "");
        /* This will specify YT user in one way or another */
        IClientPtr CreateClient(const TString& serverName, const TString& ytUser = "", const TString& token = "", const TString& rpcProxyRole = "");
        IClientPtr CreateClientWithoutUser(const TString& serverName, const TString& token = "", const TString& rpcProxyRole = "");
    }
}
