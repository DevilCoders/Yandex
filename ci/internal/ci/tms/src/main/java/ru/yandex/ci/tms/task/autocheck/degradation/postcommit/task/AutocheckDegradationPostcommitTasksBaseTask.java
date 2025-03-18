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
import ru.yandex.ci.client.sandbox.api.BatchResult;
import ru.yandex.ci.client.sandbox.api.BatchStatus;
import ru.yandex.ci.client.sandbox.api.SandboxBatchAction;
import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;

public abstract class AutocheckDegradationPostcommitTasksBaseTask extends
        AbstractOnetimeTask<AutocheckDegradationPostcommitTasksBaseTask.Parameters> {
    private static final Logger logger = LoggerFactory.getLogger(AutocheckDegradationPostcommitTasksBaseTask.class);

    protected boolean dryRun;
    protected SandboxClient sandboxClient;

    protected AutocheckDegradationPostcommitTasksBaseTask(boolean dryRun, SandboxClient sandboxClient) {
        super(Parameters.class);

        this.dryRun = dryRun;
        this.sandboxClient = sandboxClient;
    }

    protected AutocheckDegradationPostcommitTasksBaseTask(Parameters parameters) {
        super(parameters);
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofMinutes(5);
    }

    protected void executeTaskAction(Set<Long> sandboxTasksIds, SandboxBatchAction action) throws InterruptedException {
        logger.info("Sandbox tasks ids: {}", sandboxTasksIds);
        var now = OffsetDateTime.now(ZoneOffset.UTC);
        var filter = TasksFilter.builder()
                .ids(sandboxTasksIds)
                .created(TimeRange.of(now.minusWeeks(2), now))
                .statuses(getPreProcessedStatuses())
                .children(true)
                .hidden(true)
                .build();

        List<BatchResult> results = null;
        if (!dryRun) {
            results = sandboxClient.executeBatchTaskAction(
                    action,
                    sandboxTasksIds,
                    "Autocheck postcommits degradation");
            Thread.sleep(5000);

            if (results.stream().anyMatch(r -> r.getStatus() == BatchStatus.ERROR)
                    || sandboxClient.getTotalFilteredTasks(filter) > 0) {
                throw new RuntimeException(String.format("Unable to \"%s\" some tasks, results: %s", action, results));
            }
        }

        logger.info("Batch tasks action \"{}\" executed successfully, batch results: {}", action, results);
    }

    protected abstract List<SandboxTaskStatus> getPreProcessedStatuses();

    @BenderBindAllFields
    public static class Parameters {
        private final Set<Long> sandboxTasksIds;

        public Parameters(Set<Long> sandboxTasksIds) {
            this.sandboxTasksIds = sandboxTasksIds;
        }

        public Set<Long> getSandboxTasksIds() {
            return sandboxTasksIds;
        }
    }
}
