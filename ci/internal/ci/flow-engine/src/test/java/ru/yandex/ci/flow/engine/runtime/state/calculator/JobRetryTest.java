package ru.yandex.ci.flow.engine.runtime.state.calculator;

import java.time.Duration;
import java.util.ArrayList;
import java.util.List;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.config.a.model.Backoff;
import ru.yandex.ci.core.config.a.model.JobAttemptsConfig;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.events.JobExecutorSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobFailedEvent;
import ru.yandex.ci.flow.engine.runtime.events.JobRunningEvent;
import ru.yandex.ci.flow.engine.runtime.events.SubscribersSucceededEvent;
import ru.yandex.ci.flow.engine.runtime.helpers.DummyTmsTaskIdFactory;
import ru.yandex.ci.flow.engine.runtime.helpers.TestJobScheduler;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.test_data.common.jobs.ProduceRes1AndFail;

public class JobRetryTest extends FlowStateCalculatorTestBase {
    private static final String FIRST_JOB = "first_job";

    @Test
    public void schedulesRetry() {
        FlowLaunchId launchId = flowTester.activateLaunch(flow());

        flowTester.raiseJobExecuteEventsChain(launchId, FIRST_JOB);

        List<TestJobScheduler.TriggeredJob> queuedCommands = new ArrayList<>(testJobScheduler.getTriggeredJobs());

        Assertions.assertEquals(1, queuedCommands.size());
        Assertions.assertEquals(FIRST_JOB, queuedCommands.get(0).getJobLaunchId().getJobId());
    }

    @Test
    public void runsThreeTimes() {
        FlowLaunchId launchId = flowTester.runFlowToCompletion(flow());

        FlowLaunchEntity flowLaunch = flowLaunchGet(launchId);
        JobState jobState = flowLaunch.getJobState(FIRST_JOB);
        Assertions.assertEquals(3, jobState.getLaunches().size());
    }

    @Test
    public void hasScheduledRestartTime() {
        FlowLaunchId launchId = flowTester.activateLaunch(flow());

        flowStateService.recalc(launchId, new JobRunningEvent(FIRST_JOB, 1, DummyTmsTaskIdFactory.create()));
        flowStateService.recalc(launchId, new JobExecutorSucceededEvent(FIRST_JOB, 1));
        flowStateService.recalc(launchId, new SubscribersSucceededEvent(FIRST_JOB, 1));
        flowStateService.recalc(launchId, new JobFailedEvent(FIRST_JOB, 1));

        var jobLaunch = flowTester
                .getFlowLaunch(launchId)
                .getJobState(FIRST_JOB)
                .getLastLaunch();

        Assertions.assertNotNull(jobLaunch);
        Assertions.assertNotNull(jobLaunch.getScheduleTime());
    }


    static Flow flow() {
        FlowBuilder builder = FlowBuilder.create();

        builder.withJob(ProduceRes1AndFail.ID, FIRST_JOB)
                .withRetry(JobAttemptsConfig.builder()
                        .maxAttempts(3)
                        .backoff(Backoff.CONSTANT)
                        .initialBackoff(Duration.ofMinutes(1))
                        .build());

        return builder.build();
    }
}
