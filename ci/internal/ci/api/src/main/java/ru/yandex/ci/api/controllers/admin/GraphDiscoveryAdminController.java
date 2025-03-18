package ru.yandex.ci.api.controllers.admin;

import io.grpc.stub.StreamObserver;

import ru.yandex.ci.api.admin.GraphDiscoveryAdmin.ScheduleGraphDiscoveryRequest;
import ru.yandex.ci.api.admin.GraphDiscoveryAdmin.ScheduleGraphDiscoveryResponse;
import ru.yandex.ci.api.admin.GraphDiscoveryAdminServiceGrpc;
import ru.yandex.ci.api.misc.AuthUtils;
import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcCommitUtils;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.security.AccessService;
import ru.yandex.ci.engine.discovery.tier0.GraphDiscoveryService;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public class GraphDiscoveryAdminController extends GraphDiscoveryAdminServiceGrpc.GraphDiscoveryAdminServiceImplBase {

    private final AccessService accessService;
    private final GraphDiscoveryService graphDiscoveryService;
    private final ArcService arcService;

    public GraphDiscoveryAdminController(
            AccessService accessService,
            GraphDiscoveryService graphDiscoveryService,
            ArcService arcService
    ) {
        this.accessService = accessService;
        this.graphDiscoveryService = graphDiscoveryService;
        this.arcService = arcService;
    }

    @Override
    public void scheduleGraphDiscovery(ScheduleGraphDiscoveryRequest request,
                                       StreamObserver<ScheduleGraphDiscoveryResponse> responseObserver) {

        accessService.checkAccess(AuthUtils.getUsername(), "ci");

        ArcBranch arcBranch = ArcBranch.ofBranchName(request.getBranch());
        if (!arcBranch.isTrunk()) {
            throw GrpcUtils.invalidArgumentException("branch is not 'trunk'");
        }

        ArcRevision revision = ArcRevision.of(request.getRightHash());
        ArcCommit commit = arcService.getCommit(revision);
        ArcCommit parentCommit = ArcCommitUtils.firstParentArcRevision(commit)
                .map(arcService::getCommit)
                .orElse(null);

        graphDiscoveryService.scheduleGraphDiscovery(parentCommit, commit, commit.toOrderedTrunkArcRevision());
        var response = ScheduleGraphDiscoveryResponse.getDefaultInstance();
        responseObserver.onNext(response);
        responseObserver.onCompleted();
    }
}
