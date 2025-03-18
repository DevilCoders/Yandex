package ru.yandex.ci.flow.engine.runtime;

import java.util.UUID;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.flow.engine.FlowEngineTestBase;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.job.InterruptMethod;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.events.ExecutorInterruptingEvent;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;
import ru.yandex.ci.flow.engine.runtime.test_data.common.BadInterruptJob;
import ru.yandex.ci.flow.engine.runtime.test_data.common.SleepyJob;
import ru.yandex.ci.flow.engine.runtime.test_data.common.StuckJob;

public class JobInterruptTest extends FlowEngineTestBase {
    public static final String JOB_ID = "ID";

    @Autowired
    private JobInterruptionService jobInterruptionService;

    @Autowired
    private Semaphore semaphore;

    @Test
    public void terminatesJob() throws Exception {
        FlowLaunchId flowLaunchId = flowTester.activateLaunch(createFlow(SleepyJob.ID));

        Thread thread = flowTester.runScheduledJobsToCompletionAsync();
        tryAcquire(thread);

        flowTester.recalcFlowLaunch(flowLaunchId, new ExecutorInterruptingEvent(JOB_ID, 1, "me"));

        var fullJobId = new FullJobLaunchId(flowLaunchId, JOB_ID, 1);
        jobInterruptionService.notifyExecutor(fullJobId, InterruptMethod.INTERRUPT);

        thread.join();

        JobLaunch job = flowTester.getJobLastLaunch(flowLaunchId, JOB_ID);
        Assertions.assertEquals(StatusChangeType.INTERRUPTED, job.getLastStatusChange().getType());
        Assertions.assertEquals("Interrupting", job.getStatusText());
    }

    @Test
    public void killsJob() throws Exception {
        FlowLaunchId flowLaunchId = flowTester.activateLaunch(createFlow(SleepyJob.ID));

        Thread thread = flowTester.runScheduledJobsToCompletionAsync();
        tryAcquire(thread);

        flowTester.recalcFlowLaunch(flowLaunchId, new ExecutorInterruptingEvent(JOB_ID, 1, "me"));

        var fullJobId = new FullJobLaunchId(flowLaunchId, JOB_ID, 1);
        jobInterruptionService.notifyExecutor(fullJobId, InterruptMethod.KILL);

        thread.join();

        JobLaunch job = flowTester.getJobLastLaunch(flowLaunchId, JOB_ID);
        Assertions.assertEquals(StatusChangeType.KILLED, job.getLastStatusChange().getType());
        Assertions.assertEquals("Interrupting", job.getStatusText());
    }

    @Test
    public void jobWithBadInterruptContinues() throws Exception {
        FlowLaunchId flowLaunchId = flowTester.activateLaunch(createFlow(BadInterruptJob.ID));

        Thread thread = flowTester.runScheduledJobsToCompletionAsync();
        tryAcquire(thread);

        flowTester.recalcFlowLaunch(flowLaunchId, new ExecutorInterruptingEvent(JOB_ID, 1, "me"));

        var fullJobId = new FullJobLaunchId(flowLaunchId, JOB_ID, 1);
        jobInterruptionService.notifyExecutor(fullJobId, InterruptMethod.INTERRUPT);

        thread.join();

        JobLaunch job = flowTester.getJobLastLaunch(flowLaunchId, JOB_ID);
        Assertions.assertEquals(StatusChangeType.FAILED, job.getLastStatusChange().getType());
        Assertions.assertEquals("Woke up", job.getStatusText());
    }

    @Test
    public void interruptStuckJob() throws Exception {
        FlowLaunchId flowLaunchId = flowTester.activateLaunch(createFlow(StuckJob.ID));

        Thread thread = flowTester.runScheduledJobsToCompletionAsync();
        tryAcquire(thread);

        flowTester.recalcFlowLaunch(flowLaunchId, new ExecutorInterruptingEvent(JOB_ID, 1, "me"));

        var fullJobId = new FullJobLaunchId(flowLaunchId, JOB_ID, 1);
        jobInterruptionService.notifyExecutor(fullJobId, InterruptMethod.INTERRUPT);
        tryAcquire(thread);
        thread.join();

        JobLaunch job = flowTester.getJobLastLaunch(flowLaunchId, JOB_ID);
        Assertions.assertEquals(StatusChangeType.INTERRUPTED, job.getLastStatusChange().getType());
        Assertions.assertEquals("Interrupting", job.getStatusText());
    }

    private Flow createFlow(UUID executorId) {
        FlowBuilder builder = FlowBuilder.create();
        builder.withJob(executorId, JOB_ID);

        return builder.build();
    }

    private void tryAcquire(Thread thread) throws InterruptedException {
        boolean acquired = semaphore.tryAcquire(20, TimeUnit.SECONDS);
        if (!acquired) {
            if (thread.isAlive()) {
                throw new InterruptedException("acquire timeout");
            } else {
                throw new RuntimeException("Target thread exited before releasing semaphore");
            }
        }
    }
}
