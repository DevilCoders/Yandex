package ru.yandex.ci.tms.autorelease;

import java.time.Clock;
import java.time.Duration;
import java.time.Instant;
import java.util.Objects;
import java.util.function.Consumer;

import javax.annotation.Nullable;

import lombok.extern.slf4j.Slf4j;
import org.apache.curator.framework.CuratorFramework;

import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.discovery.GraphDiscoveryTask;
import ru.yandex.ci.engine.common.CiEngineCronTask;
import ru.yandex.ci.engine.discovery.tier0.GraphDiscoveryRestartTask;
import ru.yandex.ci.engine.discovery.tier0.GraphDiscoveryResultProcessorTask;
import ru.yandex.ci.engine.discovery.tier0.GraphDiscoveryService;
import ru.yandex.commune.bazinga.BazingaTaskManager;
import ru.yandex.commune.bazinga.impl.FullJobId;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.lang.NonNullApi;

import static ru.yandex.ci.core.discovery.GraphDiscoveryTask.Status.GRAPH_EVALUATION_FAILED;
import static ru.yandex.ci.core.discovery.GraphDiscoveryTask.Status.GRAPH_EVALUATION_STARTED;
import static ru.yandex.ci.core.discovery.GraphDiscoveryTask.Status.GRAPH_EVALUATION_SUCCESS;
import static ru.yandex.ci.core.discovery.GraphDiscoveryTask.Status.GRAPH_RESULTS_PROCESSING;

@Slf4j
@NonNullApi
public class GraphDiscoveryResultReadinessCronTask extends CiEngineCronTask {

    private final CiMainDb db;
    private final GraphDiscoveryService graphDiscoveryService;
    private final BazingaTaskManager bazingaTaskManager;
    private final Clock clock;

    public GraphDiscoveryResultReadinessCronTask(
            Duration runDelay,
            Duration timeout,
            CiMainDb db,
            GraphDiscoveryService graphDiscoveryService,
            BazingaTaskManager bazingaTaskManager,
            Clock clock,
            @Nullable CuratorFramework curator
    ) {
        super(runDelay, timeout, curator);
        this.db = db;
        this.graphDiscoveryService = graphDiscoveryService;
        this.bazingaTaskManager = bazingaTaskManager;
        this.clock = clock;
    }


    @Override
    public void executeImpl(ExecutionContext executionContext) {
        doExecute();
    }

    void doExecute() {
        log.info("Checking sandbox task statuses started");
        graphDiscoveryService.updateStateOfRunningSandboxTasks();

        /* All graph discovery sandbox tasks should be finished successfully eventually.
           The method `updateStateOfFailedSandboxTasks` allows to start failed sandbox task via sandbox UI, for example
           when `GraphDiscoveryTask.nextAttemptAfter` is too far.

           If sandbox task can never be finished and will always fail, we should either udpate
           `GraphDiscoveryTask.status` to `GRAPH_RESULTS_PROCESSED` or delete this row from table. This action
           prevents App from infinite retries and asking sandbox task statuses. */
        graphDiscoveryService.updateStateOfFailedSandboxTasks();

        runForEachTaskInStatus(GRAPH_EVALUATION_SUCCESS, this::scheduleGraphProcessing);
        graphDiscoveryService.runActionForTasksInStatus(GRAPH_EVALUATION_FAILED, tasks -> {
            Instant now = clock.instant();
            for (GraphDiscoveryTask task : tasks) {
                scheduleRestartSandboxTask(task, now);
            }
        });
        log.info("Checking sandbox task statuses finished");
    }

    private void runForEachTaskInStatus(GraphDiscoveryTask.Status status, Consumer<GraphDiscoveryTask> action) {
        graphDiscoveryService.runActionForTasksInStatus(status, tasks -> tasks.forEach(action));
    }

    private void scheduleGraphProcessing(GraphDiscoveryTask task) {
        db.currentOrTx(() -> {
            var params = new GraphDiscoveryResultProcessorTask.Params(
                    task.getId().getCommitId(),
                    task.getLeftSvnRevision(), task.getRightSvnRevision(),
                    task.getSandboxTaskId(),
                    task.getId().getPlatforms()
            );
            FullJobId bazingaJobId = bazingaTaskManager.schedule(new GraphDiscoveryResultProcessorTask(params));
            log.info("Bazinga job scheduled: bazingaJobId {}, task {}", bazingaJobId, task.getId());
            GraphDiscoveryTask updatedTask = db.graphDiscoveryTaskTable().get(task.getId())
                    .toBuilder()
                    .status(GRAPH_RESULTS_PROCESSING)
                    .discoveryStartedAt(clock.instant())
                    .build();
            db.graphDiscoveryTaskTable().save(updatedTask);
            log.info("State updated to {} for {}", updatedTask.getStatus(), updatedTask.getId());
        });
    }

    private void scheduleRestartSandboxTask(GraphDiscoveryTask task, Instant now) {
        boolean readyForRestart = task.getNextAttemptAfter() == null || now.isAfter(task.getNextAttemptAfter());
        if (!readyForRestart) {
            return;
        }
        db.currentOrTx(() -> {
            var params = new GraphDiscoveryRestartTask.Params(
                    task.getSandboxTaskId(),
                    task.getId().getCommitId(),
                    task.getLeftSvnRevision(),
                    task.getRightSvnRevision(),
                    task.getId().getPlatforms()
            );
            FullJobId bazingaJobId = bazingaTaskManager.schedule(new GraphDiscoveryRestartTask(params));
            log.info("Bazinga job scheduled: bazingaJobId {}, task {}", bazingaJobId, task.getId());

            GraphDiscoveryTask foundTask = db.graphDiscoveryTaskTable().get(task.getId());
            GraphDiscoveryTask updatedTask = foundTask
                    .toBuilder()
                    .status(GRAPH_EVALUATION_STARTED)
                    .nextAttemptAfter(
                            Objects.requireNonNullElse(foundTask.getNextAttemptAfter(), now)
                    )
                    .graphEvaluationFinishedAt(null)
                    .build();
            db.graphDiscoveryTaskTable().save(updatedTask);
            log.info("State updated to {} for {}", updatedTask.getStatus(), updatedTask.getId());
        });

    }

}
