package ru.yandex.ci.storage.tests;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi;
import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.api.storage.StorageApi;
import ru.yandex.ci.client.ci.CiClient;

public class TestCiClient implements CiClient {
    private final List<StorageApi.CancelFlowRequest> cancelRequests = new ArrayList<>();
    private final List<StorageApi.MarkDiscoveredCommitRequest> markDiscoveredCommitRequests = new ArrayList<>();

    public void reset() {
        cancelRequests.clear();
        markDiscoveredCommitRequests.clear();
    }

    @Override
    public StorageApi.GetLastValidConfigResponse getLastValidConfig(
            StorageApi.GetLastValidConfigRequest request
    ) {
        return StorageApi.GetLastValidConfigResponse.newBuilder()
                .setConfigRevision(Common.OrderedArcRevision.newBuilder()
                        .setHash("head")
                        .setBranch("trunk")
                        .build())
                .build();
    }

    @Override
    public StorageApi.GetLastValidPrefixedConfigResponse getLastValidConfigBatch(
            StorageApi.GetLastValidPrefixedConfigRequest request
    ) {
        return StorageApi.GetLastValidPrefixedConfigResponse.newBuilder()
                .build();
    }

    @Override
    public FrontendOnCommitFlowLaunchApi.StartFlowResponse startFlow(
            StorageApi.ExtendedStartFlowRequest request
    ) {
        return FrontendOnCommitFlowLaunchApi.StartFlowResponse.getDefaultInstance();
    }

    @Override
    public FrontendOnCommitFlowLaunchApi.StartFlowResponse startFlow(
            FrontendOnCommitFlowLaunchApi.StartFlowRequest request
    ) {
        throw new UnsupportedOperationException();
    }

    @Override
    public FrontendFlowApi.LaunchJobResponse launchJob(
            FrontendFlowApi.LaunchJobRequest request
    ) {
        throw new UnsupportedOperationException();
    }

    @Override
    public Common.LaunchStatus getFlowStatus(
            FrontendOnCommitFlowLaunchApi.GetFlowLaunchRequest request
    ) {
        return Common.LaunchStatus.FAILURE;
    }

    @Override
    public StorageApi.CancelFlowResponse cancelFlow(
            StorageApi.CancelFlowRequest request
    ) {
        cancelRequests.add(request);
        return StorageApi.CancelFlowResponse.newBuilder().build();
    }

    @Override
    public StorageApi.MarkDiscoveredCommitResponse markDiscoveredCommit(
            StorageApi.MarkDiscoveredCommitRequest request
    ) {
        markDiscoveredCommitRequests.add(request);
        return StorageApi.MarkDiscoveredCommitResponse.getDefaultInstance();
    }

    @Override
    public void close() {
    }

    public List<StorageApi.CancelFlowRequest> getCancelRequests() {
        return Collections.unmodifiableList(cancelRequests);
    }

    public List<StorageApi.MarkDiscoveredCommitRequest> getMarkDiscoveredCommitRequests() {
        return Collections.unmodifiableList(markDiscoveredCommitRequests);
    }
}
