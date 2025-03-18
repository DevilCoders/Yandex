package ru.yandex.ci.flow.engine.runtime;

import java.util.Queue;

import lombok.extern.slf4j.Slf4j;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.flow.engine.FlowEngineTestBase;
import ru.yandex.ci.flow.engine.definition.DummyJob;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.events.TriggerEvent;
import ru.yandex.ci.flow.engine.runtime.helpers.DummyTmsTaskIdFactory;
import ru.yandex.ci.flow.engine.runtime.helpers.FlowTestUtils;
import ru.yandex.ci.flow.engine.runtime.helpers.TestJobScheduler;
import ru.yandex.ci.flow.engine.runtime.state.FlowStateService;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;
import ru.yandex.ci.flow.test.TestFlowId;

@Slf4j
public class TripleTriggerTest extends FlowEngineTestBase {
    private static final String USERNAME = "user42";
    public static final String JOB_ID = "job";

    @Autowired
    private FlowStateService flowStateService;

    @Autowired
    private JobLauncher jobLauncher;

    @Autowired
    private TestJobScheduler testJobScheduler;

    @Test
    public void test() {
        var flow = getFlow();
        var flowId = flowTester.register(flow, TestFlowId.TEST_PATH);

        FlowLaunchId flowLaunchId = flowStateService.activateLaunch(
                FlowTestUtils.prepareLaunchParametersBuilder(flowId, flow)
        ).getFlowLaunchId();

        flowStateService.recalc(flowLaunchId, null);
        flowStateService.recalc(flowLaunchId, new TriggerEvent(JOB_ID, USERNAME, false));
        flowStateService.recalc(flowLaunchId, new TriggerEvent(JOB_ID, USERNAME, false));

        Queue<TestJobScheduler.TriggeredJob> triggerCommands = testJobScheduler.getTriggeredJobs();
        while (!triggerCommands.isEmpty()) {
            TestJobScheduler.TriggeredJob triggeredJob = triggerCommands.poll();
            log.info("Triggered job: {}", triggeredJob);
            jobLauncher.launchJob(triggeredJob.getJobLaunchId(), DummyTmsTaskIdFactory.create());
        }

        FlowLaunchEntity flowLaunch = flowLaunchGet(flowLaunchId);
        JobState jobState = flowLaunch.getJobState(JOB_ID);
        Assertions.assertEquals(1, jobState.getLaunches().size());

        Assertions.assertEquals(
                StatusChangeType.SUCCESSFUL,
                jobState.getLaunches().get(0).getLastStatusChange().getType()
        );
    }

    private Flow getFlow() {
        FlowBuilder builder = FlowBuilder.create();
        builder.withJob(DummyJob.ID, JOB_ID);
        return builder.build();
    }
}
