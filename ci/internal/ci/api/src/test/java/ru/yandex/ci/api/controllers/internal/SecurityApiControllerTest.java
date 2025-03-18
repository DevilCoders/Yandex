package ru.yandex.ci.api.controllers.internal;

import io.grpc.ManagedChannel;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.api.ControllerTestBase;
import ru.yandex.ci.api.internal.security.DelegatePrTokenRequest;
import ru.yandex.ci.api.internal.security.DelegatePrTokenResponse;
import ru.yandex.ci.api.internal.security.DelegateTokenRequest;
import ru.yandex.ci.api.internal.security.DelegateTokenResponse;
import ru.yandex.ci.api.internal.security.SecurityApiGrpc;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.pr.CreatePrCommentTask;
import ru.yandex.ci.engine.proto.ProtoMappers;

import static org.assertj.core.api.Assertions.assertThat;

class SecurityApiControllerTest extends ControllerTestBase<SecurityApiGrpc.SecurityApiBlockingStub> {

    @Override
    protected SecurityApiGrpc.SecurityApiBlockingStub createStub(ManagedChannel channel) {
        return SecurityApiGrpc.newBlockingStub(channel);
    }

    @Test
    void delegatePrToken() {
        CiProcessId processId = CiProcessId.ofFlow(TestData.CONFIG_PATH_PR_NEW, "sawmill");

        initDiffSets();
        arcanumTestServer.mockGetReviewRequestData(42, "arcanum_responses/getReviewRequestData-pr42-ds1.json");
        discoveryToR4();
        discoveryServicePullRequests.processDiffSet(TestData.DIFF_SET_1);

        mockSandboxDelegationAny();
        mockYavAny();

        var response = grpcService.delegatePrToken(DelegatePrTokenRequest.newBuilder()
                .setPullRequestId(TestData.DIFF_SET_1.getPullRequestId())
                .setConfigDir(processId.getDir())
                .build());

        assertThat(response.getStatus()).isEqualTo(DelegatePrTokenResponse.Status.OK);

        executeBazingaTasks(CreatePrCommentTask.class);
        arcanumTestServer.verifyCreateReviewRequestComment(
                42,
                "Token for configuration **pr/new/a.yaml** successfully delegated by @user42"
        );
    }

    @Test
    void delegateToken() {
        var processId = CiProcessId.ofFlow(TestData.CONFIG_PATH_ABC, "sawmill");
        discoveryToR2();

        mockSandboxDelegationAny();
        mockYavAny();

        var response = grpcService.delegateToken(DelegateTokenRequest.newBuilder()
                .setRevision(ProtoMappers.toProtoOrderedArcRevision(TestData.TRUNK_R2))
                .setConfigDir(processId.getDir())
                .build());
        assertThat(response)
                .isEqualTo(DelegateTokenResponse.getDefaultInstance());

        verifyYavDelegated("sec-01dy7t26dyht1bj4w3yn94fsa", 2); // Same secret in both autocheck/a.yaml and this one
    }

    @Test
    void delegateTokenLatest() {
        var processId = CiProcessId.ofFlow(TestData.CONFIG_PATH_ABC, "sawmill");
        discoveryToR2();

        mockSandboxDelegationAny();
        mockYavAny();

        var response = grpcService.delegateToken(DelegateTokenRequest.newBuilder()
                .setConfigDir(processId.getDir())
                .build());
        assertThat(response)
                .isEqualTo(DelegateTokenResponse.getDefaultInstance());

        verifyYavDelegated("sec-01dy7t26dyht1bj4w3yn94fsa", 2); // Same secret in both autocheck/a.yaml and this one
    }
}
