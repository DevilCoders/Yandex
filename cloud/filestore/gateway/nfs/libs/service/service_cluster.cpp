#include "service.h"

namespace NCloud::NFileStore::NGateway {

////////////////////////////////////////////////////////////////////////////////

int TFileStoreService::ReadNodes(yfs_readnodes_cb* cb)
{
    STORAGE_TRACE("ReadNodes");

    auto callContext = PrepareCallContext();

    auto request = CreateRequest<NProto::TListClusterNodesRequest>();

    auto future = Session->ListClusterNodes(
        std::move(callContext),
        std::move(request));

    const auto& response = future.GetValue(Config->GetRequestTimeout());
    int retval = CheckResponse(response);
    if (retval < 0) {
        return retval;
    }

    for (const auto& node: response.GetNodes()) {
        yfs_cluster_node n = {
            .node_id = node.GetNodeId().c_str(),
            .flags = node.GetFlags(),
            .num_clients = node.GetClients(),
        };
        (void) YFS_CALL(invoke, cb, &n);
    }

    return 0;
}

int TFileStoreService::AddClient(
    TString nodeId,
    const yfs_cluster_client* client)
{
    auto clientId = TString(client->client_id);
    auto opaque = TString(client->opaque, client->opaque_len);

    STORAGE_TRACE("AddClient"
        << " node:" << nodeId.Quote()
        << " client:"<< clientId.Quote());

    auto callContext = PrepareCallContext();

    auto request = CreateRequest<NProto::TAddClusterClientsRequest>();
    request->SetNodeId(std::move(nodeId));

    auto& record = *request->AddClients();
    record.SetClientId(std::move(clientId));
    record.SetOpaque(std::move(opaque));

    auto future = Session->AddClusterClients(
        std::move(callContext),
        std::move(request));

    const auto& response = future.GetValue(Config->GetRequestTimeout());
    int retval = CheckResponse(response);
    if (retval < 0) {
        return retval;
    }

    return 0;
}

int TFileStoreService::RemoveClient(TString nodeId, TString clientId)
{
    STORAGE_TRACE("RemoveClient"
        << " node:" << nodeId.Quote()
        << " client:" << clientId.Quote());

    auto callContext = PrepareCallContext();

    auto request = CreateRequest<NProto::TRemoveClusterClientsRequest>();
    request->SetNodeId(std::move(nodeId));

    *request->AddClientIds() = std::move(clientId);

    auto future = Session->RemoveClusterClients(
        std::move(callContext),
        std::move(request));

    const auto& response = future.GetValue(Config->GetRequestTimeout());
    int retval = CheckResponse(response);
    if (retval < 0) {
        return retval;
    }

    return 0;
}

int TFileStoreService::ReadClients(TString nodeId, yfs_readclients_cb* cb)
{
    STORAGE_TRACE("ReadClients"
        << " node:" << nodeId.Quote());

    auto callContext = PrepareCallContext();

    auto request = CreateRequest<NProto::TListClusterClientsRequest>();
    request->SetNodeId(std::move(nodeId));

    auto future = Session->ListClusterClients(
        std::move(callContext),
        std::move(request));

    const auto& response = future.GetValue(Config->GetRequestTimeout());
    int retval = CheckResponse(response);
    if (retval < 0) {
        return retval;
    }

    for (const auto& client: response.GetClients()) {
        yfs_cluster_client c = {
            .client_id = client.GetClientId().c_str(),
            .opaque = client.GetOpaque().data(),
            .opaque_len = static_cast<ui32>(client.GetOpaque().size()),
        };
        (void) YFS_CALL(invoke, cb, &c);
    }

    return 0;
}

int TFileStoreService::UpdateCluster(TString nodeId, ui32 update)
{
    STORAGE_TRACE("UpdateCluster"
        << " node:" << nodeId.Quote()
        << " update:" << update);

    auto callContext = PrepareCallContext();

    auto request = CreateRequest<NProto::TUpdateClusterRequest>();
    request->SetNodeId(std::move(nodeId));
    request->SetUpdate(update);

    auto future = Session->UpdateCluster(
        std::move(callContext),
        std::move(request));

    const auto& response = future.GetValue(Config->GetRequestTimeout());
    int retval = CheckResponse(response);
    if (retval < 0) {
        return retval;
    }

    return 0;
}

}   // namespace NCloud::NFileStore::NGateway
