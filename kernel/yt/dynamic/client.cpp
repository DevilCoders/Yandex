#include "client.h"

#include <yt/yt/core/net/address.h>
#include <yt/yt/client/api/rpc_proxy/connection.h>
#include <yt/yt/client/api/rpc_proxy/config.h>

#include <util/folder/dirut.h>
#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/string/strip.h>
#include <util/system/env.h>
#include <util/system/user.h>

namespace NYT {
    namespace NApi {
        namespace {
            TString LoadToken(const TString& token="") {
                if (token) {
                    return token;
                }

                TString envToken = GetEnv("YT_TOKEN");
                if (envToken) {
                    return envToken;
                }

                TString tokenPath = GetEnv("YT_TOKEN_PATH");
                if (!tokenPath) {
                    tokenPath = GetHomeDir() + "/.yt/token";
                }
                TFsPath path(tokenPath);
                return path.IsFile() ? Strip(TFileInput(ToString(path).c_str()).ReadAll()) : TString();
            }

            TString GetYtUser(const TString& user = "") {
                if (user) {
                    return user;
                } else if (TString envUser = GetEnv("YT_USER")) {
                    return envUser;
                } else {
                    return GetUsername();
                }
            }

            TString GetRpcProxyRole(const TString& rpcProxyRole = "") {
                if (rpcProxyRole) {
                    return rpcProxyRole;
                } else if (TString envRpcProxyRole = GetEnv("YT_RPC_PROXY_ROLE")) {
                    return envRpcProxyRole;
                }

                return TString{};
            }
        }

        void ConfigureAddressResolver() {
            auto addressResolverConfig = NYT::New<NYT::NNet::TAddressResolverConfig>();
            addressResolverConfig->EnableIPv6 = true;
            addressResolverConfig->EnableIPv4 = false;
            NYT::NNet::TAddressResolver::Get()->Configure(addressResolverConfig);
            // EnsureLocalHostName not only checks, but also resolves local host for further usage.
            NYT::NNet::TAddressResolver::Get()->EnsureLocalHostName();
        }

        NYT::NApi::NRpcProxy::TConnectionConfigPtr GetConnectionConfiguration(const TString& serverName, const TString& rpcProxyRole) {
            auto connConfPtr = NYT::New<NYT::NApi::NRpcProxy::TConnectionConfig>();
            connConfPtr->ClusterUrl = serverName;

            if (TString ytRpcProxyRole = GetRpcProxyRole(rpcProxyRole)) {
                connConfPtr->ProxyRole = ytRpcProxyRole;
            }

            return connConfPtr;
        }

        IClientPtr DoCreateClient(const TString& serverName, const TString& ytUser, const TString& token, const TString& rpcProxyRole) {
            NYT::NApi::NRpcProxy::TConnectionConfigPtr connConfPtr = GetConnectionConfiguration(serverName, rpcProxyRole);
            auto ytConnection = NYT::NApi::NRpcProxy::CreateConnection(connConfPtr);

            NYT::NApi::TClientOptions clientOps;
            if (!ytUser.empty()) {
                clientOps.User = ytUser;
            }
            clientOps.Token = LoadToken(token);
            return ytConnection->CreateClient(clientOps);
        }

        IClientPtr CreateClient(const TString& serverName, const TString& ytUser, const TString& token, const TString& rpcProxyRole) {
            const TString user = GetYtUser(ytUser);
            return DoCreateClient(serverName, user, token, rpcProxyRole);
        }

        IClientPtr CreateClientWithoutUser(const TString& serverName, const TString& token, const TString& rpcProxyRole) {
            return DoCreateClient(serverName, {}, token, rpcProxyRole);
        }
    }
}
