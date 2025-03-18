package ru.yandex.ci.api.controllers.storage;

import java.nio.file.Path;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.gson.JsonParser;
import io.grpc.stub.StreamObserver;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.api.controllers.frontend.OnCommitFlowController;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi.StartFlowResponse;
import ru.yandex.ci.api.storage.StorageApi;
import ru.yandex.ci.api.storage.StorageApi.ExtendedStartFlowRequest;
import ru.yandex.ci.api.storage.StorageFlowServiceGrpc;
import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.config.ConfigStatus;
import ru.yandex.ci.core.config.VirtualCiProcessId;
import ru.yandex.ci.core.config.a.AffectedAYamlsFinder;
import ru.yandex.ci.core.config.a.model.CiConfig;
import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.launch.LaunchService;
import ru.yandex.ci.engine.launch.LaunchService.DelegatedSecurity;
import ru.yandex.ci.engine.launch.LaunchService.LaunchMode;
import ru.yandex.ci.engine.launch.auto.DiscoveryProgressService;
import ru.yandex.ci.engine.proto.ProtoMappers;
import ru.yandex.ci.flow.db.CiDb;

@Slf4j
@RequiredArgsConstructor
public class StorageFlowController extends StorageFlowServiceGrpc.StorageFlowServiceImplBase {

    @Nonnull
    private final OnCommitFlowController standardController;
    @Nonnull
    private final ConfigurationService configurationService;
    @Nonnull
    private final CiDb db;
    @Nonnull
    private final String robotLogin;
    @Nonnull
    private final LaunchService launchService;
    @Nonnull
    private final DiscoveryProgressService discoveryProgressService;
    @Nonnull
    private final ConfigBundleCollectionService configBundleCollectionService;

    @Override
    public void startFlow(
            ExtendedStartFlowRequest request,
            StreamObserver<StartFlowResponse> responseObserver) {
        var flowVars = !request.getFlowVars().isEmpty()
                ? JsonParser.parseString(request.getFlowVars()).getAsJsonObject()
                : null;

        var delegatedSecurity = lookupDelegatedSecurity(request.getRequest(), request.getDelegatedConfig());

        var launchMode = request.getPostponed()
                ? LaunchMode.POSTPONE
                : LaunchMode.NORMAL;

        var response = standardController.startFlow(
                request.getRequest(),
                robotLogin,
                flowVars,
                request.getFlowTitle(),
                delegatedSecurity,
                true,
                launchMode
        );
        responseObserver.onNext(response);
        responseObserver.onCompleted();
    }

    @Override
    public void cancelFlow(
            StorageApi.CancelFlowRequest request,
            StreamObserver<StorageApi.CancelFlowResponse> responseObserver
    ) {
        var processId = ProtoMappers.toCiProcessId(request.getFlowProcessId());
        var launchId = LaunchId.of(processId, request.getNumber());

        launchService.cancel(launchId, robotLogin, "Canceled by Storage");

        responseObserver.onNext(StorageApi.CancelFlowResponse.newBuilder().build());
        responseObserver.onCompleted();
    }

    @Override
    public void getLastValidConfig(
            StorageApi.GetLastValidConfigRequest request,
            StreamObserver<StorageApi.GetLastValidConfigResponse> responseObserver) {

        var path = Path.of(request.getDir(), AffectedAYamlsFinder.CONFIG_FILE_NAME);
        var branch = ArcBranch.ofString(request.getBranch());

        var lastValidConfig = configurationService.getLastValidConfig(path, branch);

        var response = StorageApi.GetLastValidConfigResponse.newBuilder()
                .setConfigRevision(ProtoMappers.toProtoOrderedArcRevision(lastValidConfig.getRevision()))
                .build();
        responseObserver.onNext(response);
        responseObserver.onCompleted();
    }

    @Override
    public void getLastValidPrefixedConfig(
            StorageApi.GetLastValidPrefixedConfigRequest request,
            StreamObserver<StorageApi.GetLastValidPrefixedConfigResponse> responseObserver) {

        var revision = request.getRevision();
        if (revision <= 0) {
            throw GrpcUtils.failedPreconditionException(
                    "Revision is required to find prefixed configs, got " + revision);
        }

        var response = StorageApi.GetLastValidPrefixedConfigResponse.newBuilder();
        configBundleCollectionService.getLastValidConfigs(revision, request.getPrefixDirList())
                .forEach(dir ->
                        response.addResponsesBuilder()
                                .setPrefixDir(dir.getPrefix())
                                .setPath(dir.getPath())
                                .setConfigRevision(ProtoMappers.toProtoOrderedArcRevision(dir.getConfigRevision()))
                );
        responseObserver.onNext(response.build());
        responseObserver.onCompleted();
    }


    @Override
    public void markDiscoveredCommit(
            StorageApi.MarkDiscoveredCommitRequest request,
            StreamObserver<StorageApi.MarkDiscoveredCommitResponse> responseObserver) {

        var commit = db.currentOrReadOnly(() -> db.arcCommit().get(request.getRevision().getCommitId()));
        var revision = commit.toOrderedTrunkArcRevision();
        discoveryProgressService.markAsDiscovered(revision, DiscoveryType.STORAGE);

        responseObserver.onNext(StorageApi.MarkDiscoveredCommitResponse.getDefaultInstance());
        responseObserver.onCompleted();
    }

    @Nullable
    private DelegatedSecurity lookupDelegatedSecurity(
            FrontendOnCommitFlowLaunchApi.StartFlowRequest request,
            StorageApi.DelegatedConfig delegatedConfig
    ) {
        var path = delegatedConfig.getPath();
        if (path.isEmpty() || !ProtoMappers.hasOrderedArcRevision(delegatedConfig.getRevision())) {
            return null;
        }

        log.info("Looking for delegated security config: {}", delegatedConfig);

        var revision = ProtoMappers.toOrderedArcRevision(delegatedConfig.getRevision());
        var config = configurationService.getConfig(Path.of(path), revision);

        Preconditions.checkState(config.getStatus() == ConfigStatus.READY,
                "Config %s at %s must be valid, found %s", path, revision, config.getStatus());

        var securityState = config.getConfigEntity().getSecurityState();
        Preconditions.checkState(securityState.getYavTokenUuid() != null,
                "Internal error. Security token not found in %s", path);

        var aYamlConfig = config.getValidAYamlConfig();

        var virtualCiProcessId = VirtualCiProcessId.of(ProtoMappers.toCiProcessId(request.getFlowProcessId()));
        var sandboxOwner = lookupSandboxOwner(virtualCiProcessId, aYamlConfig.getCi());

        return new DelegatedSecurity(aYamlConfig.getService(), securityState.getYavTokenUuid(), sandboxOwner);
    }

    private String lookupSandboxOwner(VirtualCiProcessId virtualCiProcessId, CiConfig ciConfig) {
        var virtualType = virtualCiProcessId.getVirtualType();
        if (virtualType == VirtualCiProcessId.VirtualType.VIRTUAL_LARGE_TEST) {
            var autocheck = ciConfig.getAutocheck();
            if (autocheck != null) {
                if (autocheck.getLargeSandboxOwner() != null) {
                    return autocheck.getLargeSandboxOwner();
                }
            }
        }
        if (virtualType == VirtualCiProcessId.VirtualType.VIRTUAL_NATIVE_BUILD) {
            var autocheck = ciConfig.getAutocheck();
            if (autocheck != null) {
                if (autocheck.getNativeSandboxOwner() != null) {
                    return autocheck.getNativeSandboxOwner();
                }
            }
        }
        return ciConfig.getRuntime().getSandbox().getOwner();
    }

}
