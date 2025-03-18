package ru.yandex.ci.flow.engine.runtime.state;

import java.nio.file.Path;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.UUID;

import io.github.benas.randombeans.api.EnhancedRandom;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.Mockito;
import org.springframework.boot.test.mock.mockito.MockBean;

import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.Job;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRefContainer;
import ru.yandex.ci.flow.engine.runtime.events.JobStateChangedEvent;
import ru.yandex.ci.flow.engine.runtime.helpers.TestJobContext;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.TaskBadge;
import ru.yandex.ci.test.random.TestRandomUtils;

public class JobProgressContextServiceTest extends CommonTestBase {

    private static final long SEED = -836825185L;
    private static final EnhancedRandom RANDOM = TestRandomUtils.enhancedRandom(SEED);

    private static final String TEAMCITY = "Teamcity";

    @MockBean
    private FlowStateService flowStateService;

    @MockBean
    private CiDb db;

    private JobProgressService jobProgressService;

    private TestJobContext jobContext;
    private JobLaunch firstLaunch;

    @BeforeEach
    public void before() {
        jobProgressService = new JobProgressService(flowStateService, db);

        Job job = new Job(FlowBuilder.create().withJob(TestJobExecutor.ID, "j1"), Collections.emptySet());

        firstLaunch = new JobLaunch(1, "me", Collections.emptyList(), Collections.emptyList());

        JobState jobState = new JobState(
                job,
                Collections.emptySet(),
                ResourceRefContainer.empty(),
                null
        );

        var flowLaunchEntity = RANDOM.nextObject(FlowLaunchEntity.class).toBuilder()
                .processId(CiProcessId.ofFlow(Path.of("ci/a.yaml"), "test").asString())
                .id(TestJobContext.DEFAULT_FLOW_LAUNCH_ID)
                .flowInfo(RANDOM.nextObject(LaunchFlowInfo.class).toBuilder()
                        .flowId(TestJobContext.DEFAULT_FLOW_ID)
                        .build())
                .jobs(Map.of())
                .build();

        jobState.addLaunch(firstLaunch);
        jobContext = new TestJobContext(flowLaunchEntity, jobState);

        Mockito.reset(flowStateService);
    }

    @Test
    public void newStatusTransitionCallsRecalcState() {
        jobProgressService.changeProgress(jobContext, "New status", 0.1f, List.of());

        Mockito.verify(flowStateService, Mockito.times(1))
                .recalc(Mockito.any(), Mockito.any(JobStateChangedEvent.class));
    }

    @Test
    public void sameStatusTransitionCallsIgnored() {
        firstLaunch.setStatusText("Current status");
        firstLaunch.setTotalProgress(0.1f);

        jobProgressService.changeProgress(jobContext, "Current status", 0.1f, List.of());

        Mockito.verify(flowStateService, Mockito.never())
                .recalc(Mockito.any(), Mockito.any(JobStateChangedEvent.class));
    }

    @Test
    public void globalStatusUpdateLeadToNewState() {
        firstLaunch.setStatusText("Current status");
        firstLaunch.setTotalProgress(0.1f);

        jobProgressService.changeProgress(jobContext, "New status", 0.1f, List.of());

        Mockito.verify(flowStateService, Mockito.times(1))
                .recalc(Mockito.any(), Mockito.any(JobStateChangedEvent.class));

    }

    @Test
    public void totalProgressUpdateLeadToNewState() {
        firstLaunch.setStatusText("Current status");
        firstLaunch.setTotalProgress(0.1f);

        jobProgressService.changeProgress(jobContext, "Current status", 0.2f, List.of());

        Mockito.verify(flowStateService, Mockito.times(1))
                .recalc(Mockito.any(), Mockito.any(JobStateChangedEvent.class));
    }

    @Test
    public void taskAdditionLeadsToNewState() {
        firstLaunch.setStatusText("Current status");
        firstLaunch.setTotalProgress(0.1f);

        jobProgressService.changeProgress(jobContext, "Current status", 0.1f,
                Collections.singletonList(
                        TaskBadge.of("teamcity", TEAMCITY, "http://localhost", TaskBadge.TaskStatus.SUCCESSFUL)));

        Mockito.verify(flowStateService, Mockito.times(1))
                .recalc(Mockito.any(), Mockito.any(JobStateChangedEvent.class));
    }

    @Test
    public void sameTaskStateIgnored() {
        firstLaunch.setStatusText("Current status");
        firstLaunch.setTotalProgress(0.1f);
        firstLaunch.setTaskStates(Collections.singletonList(
                TaskBadge.of("teamcity", TEAMCITY, "http://localhost", TaskBadge.TaskStatus.SUCCESSFUL)));

        jobProgressService.changeProgress(jobContext, "Current status", 0.1f,
                Collections.singletonList(
                        TaskBadge.of("teamcity", TEAMCITY, "http://localhost", TaskBadge.TaskStatus.SUCCESSFUL)));

        Mockito.verify(flowStateService, Mockito.never())
                .recalc(Mockito.any(), Mockito.any(JobStateChangedEvent.class));
    }

    @Test
    public void taskChangesLeadToNewState() {
        firstLaunch.setStatusText("Current status");
        firstLaunch.setTotalProgress(0.1f);
        firstLaunch.setTaskStates(Collections.singletonList(
                TaskBadge.of("teamcity", TEAMCITY, "http://localhost", TaskBadge.TaskStatus.RUNNING)));

        jobProgressService.changeProgress(jobContext, "Current status", 0.1f,
                Collections.singletonList(
                        TaskBadge.of("teamcity", TEAMCITY, "http://localhost", TaskBadge.TaskStatus.SUCCESSFUL)));

        Mockito.verify(flowStateService, Mockito.times(1))
                .recalc(Mockito.any(), Mockito.any(JobStateChangedEvent.class));
    }

    public static class TestJobExecutor implements JobExecutor {

        public static final UUID ID = UUID.fromString("b386baf5-3b28-4efc-9f9b-063cd629b624");

        @Override
        public UUID getSourceCodeId() {
            return ID;
        }

        @Override
        public void execute(JobContext context) throws Exception {
        }
    }
}
