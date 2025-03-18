package ru.yandex.ci.client.ci;

import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi;
import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.api.storage.StorageApi;

public interface CiClient extends AutoCloseable {
    StorageApi.GetLastValidConfigResponse getLastValidConfig(
            StorageApi.GetLastValidConfigRequest request
    );

    StorageApi.GetLastValidPrefixedConfigResponse getLastValidConfigBatch(
            StorageApi.GetLastValidPrefixedConfigRequest request
    );

    FrontendOnCommitFlowLaunchApi.StartFlowResponse startFlow(
            StorageApi.ExtendedStartFlowRequest request
    );

    FrontendOnCommitFlowLaunchApi.StartFlowResponse startFlow(
            FrontendOnCommitFlowLaunchApi.StartFlowRequest request
    );

    FrontendFlowApi.LaunchJobResponse launchJob(
            FrontendFlowApi.LaunchJobRequest request
    );

    Common.LaunchStatus getFlowStatus(
            FrontendOnCommitFlowLaunchApi.GetFlowLaunchRequest request
    );

    StorageApi.CancelFlowResponse cancelFlow(
            StorageApi.CancelFlowRequest request
    );

    StorageApi.MarkDiscoveredCommitResponse markDiscoveredCommit(
            StorageApi.MarkDiscoveredCommitRequest request
    );
}
