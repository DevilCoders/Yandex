package ru.yandex.ci.client.ci;

import java.util.function.Supplier;

import javax.annotation.Nonnull;

import ru.yandex.ci.api.internal.frontend.flow.FlowServiceGrpc;
import ru.yandex.ci.api.internal.frontend.flow.FrontendFlowApi;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.StartFlowResponse;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.OnCommitFlowServiceGrpc;
import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.api.storage.StorageApi;
import ru.yandex.ci.api.storage.StorageApi.ExtendedStartFlowRequest;
import ru.yandex.ci.api.storage.StorageApi.GetLastValidConfigRequest;
import ru.yandex.ci.api.storage.StorageApi.GetLastValidConfigResponse;
import ru.yandex.ci.api.storage.StorageFlowServiceGrpc;
import ru.yandex.ci.api.storage.StorageFlowServiceGrpc.StorageFlowServiceBlockingStub;
import ru.yandex.ci.common.grpc.GrpcClient;
import ru.yandex.ci.common.grpc.GrpcClientImpl;
import ru.yandex.ci.common.grpc.GrpcClientProperties;

public class CiClientImpl implements CiClient {

    private final GrpcClient grpcClient;
    private final Supplier<StorageFlowServiceBlockingStub> storageStub;
    private final Supplier<OnCommitFlowServiceGrpc.OnCommitFlowServiceBlockingStub> flowStub;
    private final Supplier<FlowServiceGrpc.FlowServiceBlockingStub> flowServiceStub;

    private CiClientImpl(GrpcClientProperties properties) {
        this.grpcClient = GrpcClientImpl.builder(properties, getClass()).build();
        this.storageStub = grpcClient.buildStub(StorageFlowServiceGrpc::newBlockingStub);
        this.flowStub = grpcClient.buildStub(OnCommitFlowServiceGrpc::newBlockingStub);
        this.flowServiceStub = grpcClient.buildStub(FlowServiceGrpc::newBlockingStub);
    }

    public static CiClientImpl create(GrpcClientProperties properties) {
        return new CiClientImpl(properties);
    }

    @Override
    public void close() throws Exception {
        grpcClient.close();
    }

    @Override
    public StartFlowResponse startFlow(@Nonnull ExtendedStartFlowRequest request) {
        return storageStub.get().startFlow(request);
    }

    @Override
    public FrontendFlowApi.LaunchJobResponse launchJob(@Nonnull FrontendFlowApi.LaunchJobRequest request) {
        return flowServiceStub.get().launchJob(request);
    }

    @Override
    public StartFlowResponse startFlow(@Nonnull FrontendOnCommitFlowLaunchApi.StartFlowRequest request) {
        return flowStub.get().startFlow(request);
    }

    @Override
    public Common.LaunchStatus getFlowStatus(@Nonnull FrontendOnCommitFlowLaunchApi.GetFlowLaunchRequest request) {
        return flowStub.get().getFlowLaunch(request).getLaunch().getStatus();
    }

    @Override
    public GetLastValidConfigResponse getLastValidConfig(
            @Nonnull GetLastValidConfigRequest request
    ) {
        return storageStub.get().getLastValidConfig(request);
    }

    @Override
    public StorageApi.GetLastValidPrefixedConfigResponse getLastValidConfigBatch(
            @Nonnull StorageApi.GetLastValidPrefixedConfigRequest request
    ) {
        return storageStub.get().getLastValidPrefixedConfig(request);
    }

    @Override
    public StorageApi.CancelFlowResponse cancelFlow(
            @Nonnull StorageApi.CancelFlowRequest request
    ) {
        return storageStub.get().cancelFlow(request);
    }

    @Override
    public StorageApi.MarkDiscoveredCommitResponse markDiscoveredCommit(
            @Nonnull StorageApi.MarkDiscoveredCommitRequest request
    ) {
        return storageStub.get().markDiscoveredCommit(request);
    }
}
