package ru.yandex.ci.api.controllers.internal;

import java.nio.file.Path;
import java.util.Optional;

import com.google.common.base.Preconditions;
import io.grpc.stub.StreamObserver;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.api.internal.security.DelegatePrTokenRequest;
import ru.yandex.ci.api.internal.security.DelegatePrTokenResponse;
import ru.yandex.ci.api.internal.security.DelegateTokenRequest;
import ru.yandex.ci.api.internal.security.DelegateTokenResponse;
import ru.yandex.ci.api.internal.security.SecurityApiGrpc;
import ru.yandex.ci.api.misc.AuthUtils;
import ru.yandex.ci.client.tvm.grpc.AuthenticatedUser;
import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.config.a.AYamlService;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.flow.SecurityDelegationService;
import ru.yandex.ci.engine.flow.YavDelegationException;
import ru.yandex.ci.engine.launch.LaunchService;
import ru.yandex.ci.engine.proto.ProtoMappers;

@Slf4j
@RequiredArgsConstructor
public class SecurityApiController extends SecurityApiGrpc.SecurityApiImplBase {

    private final SecurityDelegationService securityDelegationService;
    private final ConfigurationService configurationService;
    private final LaunchService launchService;

    @Override
    public void delegateToken(DelegateTokenRequest request, StreamObserver<DelegateTokenResponse> responseObserver) {
        AuthenticatedUser apiUser = AuthUtils.getUser();
        if (apiUser.getTvmUserTicket() == null) {
            throw GrpcUtils.unauthenticated("TVM-User-Ticket is required");
        }

        Path configPath = AYamlService.dirToConfigPath(request.getConfigDir());

        ConfigBundle configBundle = ProtoMappers.hasOrderedArcRevision(request.getRevision())
                ? configurationService.getConfig(configPath, ProtoMappers.toOrderedArcRevision(request.getRevision()))
                : configurationService.getLastConfig(configPath, ArcBranch.trunk()); // Latest from trunk

        try {
            securityDelegationService.delegateYavTokens(
                    configBundle,
                    apiUser.getTvmUserTicket(),
                    apiUser.getLogin()
            );
        } catch (YavDelegationException e) {
            throw GrpcUtils.permissionDeniedException(e);
        }

        launchService.startDelayedLaunches(configPath, configBundle.getRevision().toRevision());

        responseObserver.onNext(DelegateTokenResponse.newBuilder().build());
        responseObserver.onCompleted();
    }

    @Override
    public void delegatePrToken(DelegatePrTokenRequest request,
                                StreamObserver<DelegatePrTokenResponse> responseObserver) {
        AuthenticatedUser apiUser = AuthUtils.getUser();
        if (apiUser.getTvmUserTicket() == null) {
            throw GrpcUtils.unauthenticated("TVM-User-Ticket is required");
        }

        Preconditions.checkArgument(request.getPullRequestId() > 0,
                "pull_request_id is required, got %s", request.getPullRequestId());

        Path configPath = AYamlService.dirToConfigPath(request.getConfigDir());

        ArcBranch branch = ArcBranch.ofPullRequest(request.getPullRequestId());

        Optional<ConfigBundle> configBundle = configurationService.findLastValidConfig(configPath, branch);
        if (configBundle.isEmpty()) {
            if (branch.isTrunk()) {
                throw new RuntimeException(String.format("Unable to find valid config %s in branch %s",
                        configPath, branch));
            } else {
                log.info("Config not found in branch {}, looking in trunk", branch);
                configBundle = configurationService.findLastValidConfig(configPath, ArcBranch.trunk());
                if (configBundle.isEmpty()) {
                    throw new RuntimeException(String.format("Unable to find valid config %s in branch %s or %s",
                            configPath, branch, ArcBranch.trunk()));
                }
            }
        }

        try {
            securityDelegationService.delegateYavTokens(
                    configBundle.get(),
                    apiUser.getTvmUserTicket(),
                    apiUser.getLogin()
            );
        } catch (YavDelegationException e) {
            throw GrpcUtils.permissionDeniedException(e);
        }

        launchService.startDelayedLaunches(configPath, configBundle.get().getRevision().toRevision());

        responseObserver.onNext(
                DelegatePrTokenResponse.newBuilder()
                        .setStatus(DelegatePrTokenResponse.Status.OK)
                        .build()
        );
        responseObserver.onCompleted();

    }
}
