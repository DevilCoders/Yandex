package ru.yandex.ci.flow.engine.runtime;

import java.time.Instant;
import java.util.ArrayList;
import java.util.List;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.flow.engine.definition.DummyJob;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.builder.JobBuilder;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.events.DisableJobManualSwitchEvent;
import ru.yandex.ci.flow.engine.runtime.events.EnableJobManualSwitchEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobExecutorSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobRunningEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.events.SubscribersSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.helpers.DummyTmsTaskIdFactory;
import ru.yandex.ci.flow.engine.runtime.helpers.TestJobScheduler;
import ru.yandex.ci.flow.engine.runtime.state.calculator.FlowStateCalculatorTestBase;
import ru.yandex.ci.flow.test.TestFlowId;

public class ToggleJobManualSwitchEventTest extends FlowStateCalculatorTestBase {
    private static final String FIRST_JOB = "first";
    private static final String SECOND_JOB = "second";

    @Test
    public void blocksExecution() {
        var flowId = flowProvider.register(flow(false), TestFlowId.TEST_PATH);
        FlowLaunchId launchId = activateLaunch(flowId);

        flowStateService.recalc(launchId, new JobRunningEvent(FIRST_JOB, 1, DummyTmsTaskIdFactory.create()));
        flowStateService.recalc(launchId, new JobExecutorSucceededEvent(FIRST_JOB, 1));
        flowStateService.recalc(launchId, new SubscribersSucceededEvent(FIRST_JOB, 1));
        flowStateService.recalc(launchId, new EnableJobManualSwitchEvent(SECOND_JOB, USERNAME, Instant.now()));
        flowStateService.recalc(launchId, new JobSucceededEvent(FIRST_JOB, 1));

        List<TestJobScheduler.TriggeredJob> queuedCommands = new ArrayList<>(testJobScheduler.getTriggeredJobs());
        Assertions.assertEquals(1, queuedCommands.size());
    }

    @Test
    public void enablesExecution() {
        var flowId = flowProvider.register(flow(true), TestFlowId.TEST_PATH);
        FlowLaunchId launchId = activateLaunch(flowId);

        flowStateService.recalc(launchId, new JobRunningEvent(FIRST_JOB, 1, DummyTmsTaskIdFactory.create()));
        flowStateService.recalc(launchId, new JobExecutorSucceededEvent(FIRST_JOB, 1));
        flowStateService.recalc(launchId, new SubscribersSucceededEvent(FIRST_JOB, 1));
        flowStateService.recalc(launchId, new DisableJobManualSwitchEvent(SECOND_JOB, USERNAME, Instant.now()));
        // Multiple calls does nothing
        flowStateService.recalc(launchId, new DisableJobManualSwitchEvent(SECOND_JOB, USERNAME, Instant.now()));
        flowStateService.recalc(launchId, new DisableJobManualSwitchEvent(SECOND_JOB, USERNAME, Instant.now()));
        flowStateService.recalc(launchId, new JobSucceededEvent(FIRST_JOB, 1));

        List<TestJobScheduler.TriggeredJob> queuedCommands = new ArrayList<>(testJobScheduler.getTriggeredJobs());
        Assertions.assertEquals(2, queuedCommands.size());
    }

    static Flow flow(boolean withManualTrigger) {
        FlowBuilder builder = FlowBuilder.create();

        JobBuilder first = builder.withJob(DummyJob.ID, FIRST_JOB);

        JobBuilder second = builder.withJob(DummyJob.ID, SECOND_JOB)
                .withUpstreams(first);

        if (withManualTrigger) {
            second.withManualTrigger();
        }

        return builder.build();
    }

}
