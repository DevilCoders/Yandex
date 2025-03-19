#include "node.h"

#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>

#include <ydb/core/base/event_filter.h>
#include <ydb/core/base/location.h>
#include <ydb/core/protos/config.pb.h>
#include <ydb/public/lib/deprecated/kicli/kicli.h>

#include <library/cpp/actors/core/actor.h>
#include <library/cpp/actors/core/event.h>

#include <util/generic/vector.h>
#include <util/network/address.h>
#include <util/network/socket.h>
#include <util/random/shuffle.h>
#include <util/string/join.h>
#include <util/system/hostname.h>

namespace NCloud::NFileStore::NStorage {

using namespace NActors;

using namespace NKikimr;

namespace {

////////////////////////////////////////////////////////////////////////////////

TString GetNetworkAddress(const TString& host)
{
    TNetworkAddress addr(host, 0);

    // prefer ipv6
    for (auto it = addr.Begin(), end = addr.End(); it != end; ++it) {
        if (it->ai_family == AF_INET6) {
            return NAddr::PrintHost(NAddr::TAddrInfo(&*it));
        }
    }

    // fallback to ipv4
    for (auto it = addr.Begin(), end = addr.End(); it != end; ++it) {
        if (it->ai_family == AF_INET) {
            return NAddr::PrintHost(NAddr::TAddrInfo(&*it));
        }
    }

    ythrow TServiceError(E_FAIL)
        << "could not resolve address for host: " << host;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

TRegisterDynamicNodeResult RegisterDynamicNode(
    NKikimrConfig::TAppConfigPtr appConfig,
    const TRegisterDynamicNodeOptions& options,
    TLog& Log)
{
    auto& nsConfig = *appConfig->MutableNameserviceConfig();
    auto& dnConfig = *appConfig->MutableDynamicNodeConfig();

    auto hostName = FQDNHostName();
    auto hostAddress = GetNetworkAddress(hostName);

    TVector<TString> addrs;
    if (options.NodeBrokerAddress) {
        addrs.push_back(options.NodeBrokerAddress);
    } else if (options.NodeBrokerPort) {
        for (const auto& node: nsConfig.GetNode()) {
            addrs.emplace_back(Join(":", node.GetHost(), options.NodeBrokerPort));
        }
        Shuffle(addrs.begin(), addrs.end());
    }

    if (!addrs) {
        ythrow TServiceError(E_FAIL)
            << "cannot register dynamic node: "
            << "neither NodeBrokerAddress nor NodeBrokerPort specified";
    }

    static constexpr int MaxAttempts = 10;
    static constexpr TDuration ErrorTimeout = TDuration::Seconds(1);
    static constexpr TDuration RegistrationTimeout = TDuration::Seconds(5);

    NActorsInterconnect::TNodeLocation proto;
    proto.SetDataCenter(options.DataCenter);
    proto.SetRack(options.Rack);
    proto.SetUnit(ToString(options.Body));
    const TNodeLocation location(proto);

    int attempts = 0;
    for (;;) {
        const auto& nodeBrokerAddress = addrs[attempts++ % addrs.size()];

        NGRpcProxy::TGRpcClientConfig config(nodeBrokerAddress, RegistrationTimeout);
        NClient::TKikimr kikimr(config);

        STORAGE_INFO("Trying to register dynamic node with NodeBrokerAddress: " << nodeBrokerAddress);

        auto registrant = kikimr.GetNodeRegistrant();
        auto result = registrant.SyncRegisterNode(
            options.Domain,
            hostName,
            options.InterconnectPort,
            hostAddress,
            hostName,
            location,
            false,
            options.SchemeShardDir);

        if (!result.IsSuccess()) {
            if (attempts == MaxAttempts) {
                ythrow TServiceError(E_FAIL)
                    << "cannot register dynamic node: " << result.GetErrorMessage();
            }
            STORAGE_WARN("Failed to register dynamic node with NodeBrokerAddress: " << nodeBrokerAddress);
            Sleep(ErrorTimeout);
            continue;
        }

        nsConfig.ClearNode();
        for (const auto& node: result.Record().GetNodes()) {
            if (node.GetNodeId() == result.GetNodeId()) {
                dnConfig.MutableNodeInfo()->CopyFrom(node);
            } else {
                auto& info = *nsConfig.AddNode();
                info.SetNodeId(node.GetNodeId());
                info.SetAddress(node.GetAddress());
                info.SetPort(node.GetPort());
                info.SetHost(node.GetHost());
                info.SetInterconnectHost(node.GetResolveHost());
                info.MutableLocation()->CopyFrom(node.GetLocation());
            }
        }

        STORAGE_INFO("Registered dynamic node with NodeBrokerAddress: " << nodeBrokerAddress);

        return { result.GetNodeId(), result.GetScopeId() };
    }
}

}   // namespace NCloud::NFileStore::NStorage
