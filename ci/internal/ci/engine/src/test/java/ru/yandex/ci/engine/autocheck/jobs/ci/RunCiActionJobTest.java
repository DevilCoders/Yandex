package ru.yandex.ci.engine.autocheck.jobs.ci;

import java.nio.file.Path;
import java.time.Instant;
import java.util.Optional;

import ci.tasklets.run_ci_action.RunCiAction;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.boot.test.mock.mockito.MockBean;

import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.api.internal.frontend.flow.oncommit.FrontendOnCommitFlowLaunchApi;
import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.client.ci.CiClient;
import ru.yandex.ci.client.yav.model.YavSecret;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.launch.LaunchRuntimeInfo;
import ru.yandex.ci.core.launch.LaunchVcsInfo;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.core.security.YavToken;
import ru.yandex.ci.engine.flow.SecurityAccessService;
import ru.yandex.ci.flow.engine.runtime.helpers.TestJobContext;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.LaunchInfo;
import ru.yandex.ci.flow.engine.runtime.state.model.TaskBadge;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

public class RunCiActionJobTest extends CommonTestBase {

    @MockBean
    private SecurityAccessService securityAccessService;

    @MockBean
    private CiClient client;

    private RunCiActionJob job;

    @BeforeEach
    public void setup() {
        job = new RunCiActionJob(securityAccessService, "some-environment", client);

        YavSecret secret = mock(YavSecret.class);
        when(securityAccessService.getYavSecret(any(YavToken.Id.class))).thenReturn(secret);
        when(secret.getValueByKey(YavSecret.CI_TOKEN)).thenReturn(Optional.of("OAuthToken"));
        when(client.startFlow(any(FrontendOnCommitFlowLaunchApi.StartFlowRequest.class)))
                .thenReturn(FrontendOnCommitFlowLaunchApi.StartFlowResponse.newBuilder()
                        .setLaunch(FrontendOnCommitFlowLaunchApi.FlowLaunch.newBuilder()
                                .setFlowProcessId(Common.FlowProcessId.newBuilder()
                                        .setDir("dir")
                                        .setId("test-process")
                                        .build())
                                .build())
                        .build());
    }

    @Test
    public void shouldRespectEnvSettings() throws Exception {
        var config = RunCiAction.Config.newBuilder()
                .setEnvironment(RunCiAction.Config.CiEnvironment.TESTING)
                .setActionPath("ci/internal")
                .setActionId("some-action")
                .setExecutionTimeout("3s")
                .build();
        var context = getContext();
        context.registerConsumedResource(Resource.of(config));

        when(client.getFlowStatus(any(FrontendOnCommitFlowLaunchApi.GetFlowLaunchRequest.class)))
                .thenReturn(Common.LaunchStatus.FINISHED);
        job.execute(context);
        TaskBadge badge = context.progress().getTaskState("CI_BADGE");
        assertThat(badge.getStatus()).isEqualByComparingTo(TaskBadge.TaskStatus.SUCCESSFUL);
        assertThat(badge.getUrl()).contains("dir=dir");
        assertThat(badge.getUrl()).contains("id=test-process");
    }

    private TestJobContext getContext() {
        OrderedArcRevision revision = OrderedArcRevision.fromRevision(ArcRevision.of("123"), "abcd", 1L, 100500L);
        FlowLaunchEntity entity = FlowLaunchEntity.builder()
                .id(FlowLaunchEntity.Id.of("entity-id"))
                .processId("process:id:sample")
                .projectId("project-id")
                .launchInfo(LaunchInfo.of("launch-version"))
                .vcsInfo(LaunchVcsInfo.builder()
                        .revision(revision)
                        .build())
                .flowInfo(LaunchFlowInfo.builder()
                        .runtimeInfo(LaunchRuntimeInfo.builder()
                                .sandboxOwner("owner")
                                .build())
                        .flowId(FlowFullId.of(Path.of("a.yaml"), "flow-id"))
                        .configRevision(revision)
                        .build())
                .createdDate(Instant.now())
                .build();
        return new TestJobContext(entity);
    }
}
