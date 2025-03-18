package ru.yandex.ci.tms.task.autocheck.degradation.postcommit.task;

import java.time.Duration;
import java.time.OffsetDateTime;
import java.time.ZoneOffset;
import java.util.List;
import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.sandbox.TasksFilter;
import ru.yandex.ci.client.sandbox.TimeRange;
import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.tms.data.autocheck.SemaphoreId;
import ru.yandex.commune.bazinga.BazingaTaskManager;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;

public class AutocheckDegradationPostcommitTasksRestartSchedulerTask extends
        AbstractOnetimeTask<AutocheckDegradationPostcommitTasksRestartSchedulerTask.Parameters> {
    private static final Logger logger = LoggerFactory.getLogger(
            AutocheckDegradationPostcommitTasksRestartSchedulerTask.class);

    public static final int MAX_BATCH_OPERATIONS_LIMIT = 100;

    private SandboxClient sandboxClient;
    private BazingaTaskManager bazingaTaskManager;

    public AutocheckDegradationPostcommitTasksRestartSchedulerTask(
            SandboxClient sandboxClient,
            BazingaTaskManager bazingaTaskManager) {
        super(Parameters.class);

        this.sandboxClient = sandboxClient;
        this.bazingaTaskManager = bazingaTaskManager;
    }

    public AutocheckDegradationPostcommitTasksRestartSchedulerTask(Parameters parameters) {
        super(parameters);
    }

    @Override
    protected void execute(Parameters params, ExecutionContext context) {
        var now = OffsetDateTime.now(ZoneOffset.UTC);

        var filter = TasksFilter.builder()
                .created(TimeRange.of(now.minusWeeks(2), now))
                .type("AUTOCHECK_BUILD_YA_2")
                .type("AUTOCHECK_BUILD_PARENT_2")
                .status(SandboxTaskStatus.ASSIGNED)
                .status(SandboxTaskStatus.EXECUTING)
                .status(SandboxTaskStatus.WAIT_TASK)
                .status(SandboxTaskStatus.PREPARING)
                .children(true)
                .hidden(true)
                .tag("TESTENV-AUTOCHECK-JOB")
                .allTags(true)
                .semaphoreAcquirers(Integer.parseInt(params.getAcquiredSemaphore().getSandboxId()))
                .build();

        int total = (int) sandboxClient.getTotalFilteredTasks(filter);
        List<Long> tasksIds = sandboxClient.getTasksIds(filter.toBuilder().limit(total).build());

        logger.info("Sandbox tasks ids for restart received successfully");

        for (int i = 0; i < tasksIds.size(); i += MAX_BATCH_OPERATIONS_LIMIT) {
            bazingaTaskManager.schedule(new AutocheckDegradationPostcommitTasksRestartTask(
                    new AutocheckDegradationPostcommitTasksBaseTask.Parameters(
                            Set.copyOf(tasksIds.subList(i, Math.min(tasksIds.size(), i + MAX_BATCH_OPERATIONS_LIMIT)))
                    ))
            );
        }
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofMinutes(5);
    }

    @BenderBindAllFields
    public static class Parameters {
        private final SemaphoreId acquiredSemaphore;

        public Parameters(SemaphoreId acquiredSemaphore) {
            this.acquiredSemaphore = acquiredSemaphore;
        }

        public SemaphoreId getAcquiredSemaphore() {
            return acquiredSemaphore;
        }
    }
}
