package ru.yandex.ci.engine.launch.auto;

import java.time.Clock;
import java.time.Duration;
import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.Optional;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.Supplier;

import javax.annotation.Nullable;

import io.micrometer.core.instrument.Gauge;
import io.micrometer.core.instrument.MeterRegistry;
import lombok.Builder;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlView;
import ru.yandex.ci.common.ydb.YqlPredicateCi;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.discovery.CommitDiscoveryProgress;
import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.ci.core.discovery.GraphDiscoveryTask;
import ru.yandex.ci.engine.discovery.tier0.GraphDiscoveryService;
import ru.yandex.lang.NonNullApi;

import static ru.yandex.ci.core.discovery.GraphDiscoveryTask.Status.GRAPH_EVALUATION_FAILED;
import static ru.yandex.ci.core.discovery.GraphDiscoveryTask.Status.GRAPH_EVALUATION_STARTED;
import static ru.yandex.ci.core.discovery.GraphDiscoveryTask.Status.GRAPH_EVALUATION_SUCCESS;
import static ru.yandex.ci.core.discovery.GraphDiscoveryTask.Status.GRAPH_RESULTS_PROCESSING;

@Slf4j
@NonNullApi
public class AutoReleaseMetrics {

    public static final String METRICS_GROUP = "auto_release";

    private final AtomicReference<Statistics> statistics;
    private final ArcService arcService;
    private final CiMainDb db;
    private final DiscoveryProgressChecker discoveryProgressChecker;
    private final GraphDiscoveryService graphDiscoveryService;
    private final Clock clock;

    public AutoReleaseMetrics(
            ArcService arcService,
            CiMainDb db,
            DiscoveryProgressChecker discoveryProgressChecker,
            GraphDiscoveryService graphDiscoveryService,
            Clock clock,
            MeterRegistry meterRegistry
    ) {
        this.arcService = arcService;
        this.statistics = new AtomicReference<>(Statistics.builder().build());
        this.db = db;
        this.discoveryProgressChecker = discoveryProgressChecker;
        this.graphDiscoveryService = graphDiscoveryService;
        this.clock = clock;
        registerStatistics(meterRegistry, this.statistics);
    }

    public void run() {
        log.info("Updating auto release metrics...");
        db.currentOrReadOnly(() -> {
            var stats = statistics.get().toBuilder();

            stats.tasksInStatusGraphEvaluationFailed = db.graphDiscoveryTaskTable().count(
                    YqlPredicate.where("status").eq(GRAPH_EVALUATION_FAILED),
                    YqlView.index(GraphDiscoveryTask.IDX_STATUS_AND_SANDBOX_TASK_ID)
            );

            var runningSandboxTasksStat = new RunningSandboxTasksStatistics();
            graphDiscoveryService.runActionForTasksInStatus(
                    GRAPH_EVALUATION_STARTED, tasks -> {
                        Instant now = clock.instant();
                        tasks.forEach(task -> runningSandboxTasksStat.collect(task, now));
                    }
            );
            stats.with(runningSandboxTasksStat);

            ArcRevision headRevision = arcService.getLastRevisionInBranch(ArcBranch.trunk());
            ArcCommit headCommit = arcService.getCommit(headRevision);
            Instant now = clock.instant();

            log.info("Head commit is {}", headCommit);

            stats.dirDiscoveryDelay = DiscoveryDelayStatistics.create(now, headCommit,
                    arcService, db, discoveryProgressChecker, stats.dirDiscoveryDelay);

            stats.graphDiscoveryDelay = DiscoveryDelayStatistics.create(now, headCommit,
                    arcService, db, discoveryProgressChecker, stats.graphDiscoveryDelay);

            stats.storageDiscoveryDelay = DiscoveryDelayStatistics.create(now, headCommit,
                    arcService, db, discoveryProgressChecker, stats.storageDiscoveryDelay);

            // counters for throughput graph
            stats.startedTaskCount = graphDiscoveryService.getStartedTaskCount().get();
            stats.finishedTaskCount = graphDiscoveryService.getProcessedResultCount().get();
            stats.inProgressTaskCount = db.graphDiscoveryTaskTable().count(
                    YqlPredicateCi.in("status",
                            GRAPH_EVALUATION_STARTED, GRAPH_EVALUATION_FAILED, GRAPH_EVALUATION_SUCCESS,
                            GRAPH_RESULTS_PROCESSING
                    ),
                    YqlView.index(GraphDiscoveryTask.IDX_STATUS_AND_SANDBOX_TASK_ID)
            );

            statistics.set(stats.build());
        });

    }

    private static void registerStatistics(MeterRegistry meterRegistry, AtomicReference<Statistics> stats) {
        register(meterRegistry, () -> stats.get().dirDiscoveryDelay);
        register(meterRegistry, () -> stats.get().graphDiscoveryDelay);
        register(meterRegistry, () -> stats.get().storageDiscoveryDelay);

        Gauge.builder(
                        METRICS_GROUP + ".graph_discovery.tasks_in_status",
                        () -> stats.get().getTasksInStatusGraphEvaluationFailed()
                )
                .tag("status", GraphDiscoveryTask.Status.GRAPH_EVALUATION_FAILED.name().toLowerCase())
                .register(meterRegistry);

        Gauge.builder(
                        METRICS_GROUP + ".graph_discovery.running_sandbox_task",
                        () -> stats.get().getMaxAttemptOfRunningSandboxTask()
                )
                .tag("metric", "attempt_number")
                .tag("value", "max")
                .register(meterRegistry);

        Gauge.builder(
                        METRICS_GROUP + ".graph_discovery.running_sandbox_task",
                        () -> stats.get().maxDurationOfRunningSandboxTask.toSeconds()
                )
                .tag("metric", "max_duration_seconds")
                .tag("value", "max")
                .register(meterRegistry);

        Gauge.builder(
                        METRICS_GROUP + ".graph_discovery.tasks_started",
                        () -> stats.get().getStartedTaskCount()
                )
                .register(meterRegistry);

        Gauge.builder(
                        METRICS_GROUP + ".graph_discovery.tasks_finished",
                        () -> stats.get().getFinishedTaskCount()
                )
                .register(meterRegistry);

        Gauge.builder(
                        METRICS_GROUP + ".graph_discovery.tasks_in_status",
                        () -> stats.get().getInProgressTaskCount()
                )
                .tag("status", "in_progress")
                .register(meterRegistry);
    }

    static void register(MeterRegistry meterRegistry, Supplier<DiscoveryDelayStatistics> stats) {
        var name = stats.get().discoveryType.name().toLowerCase();
        log.info("Registering discovery type stats: {}", name);
        Gauge.builder(METRICS_GROUP + ".discovery.delay", () -> stats.get().getWaitingParentCommits())
                .tag("metric", "commits")
                .tag("discovery_type", name)
                .register(meterRegistry);
        Gauge.builder(METRICS_GROUP + ".discovery.delay", () -> stats.get().delay.toSeconds())
                .tag("metric", "seconds")
                .tag("discovery_type", name)
                .register(meterRegistry);
        Gauge.builder(METRICS_GROUP + ".discovery.last_processed", () -> stats.get().getLastProcessedSvnRevision())
                .tag("metric", "svn_revision")
                .tag("discovery_type", name)
                .register(meterRegistry);
    }

    @Value
    @Builder(toBuilder = true)
    @NonNullApi
    static class Statistics {

        DiscoveryDelayStatistics dirDiscoveryDelay;
        DiscoveryDelayStatistics graphDiscoveryDelay;
        DiscoveryDelayStatistics storageDiscoveryDelay;

        long tasksInStatusGraphEvaluationFailed;
        long maxAttemptOfRunningSandboxTask;
        Duration maxDurationOfRunningSandboxTask;
        long startedTaskCount;
        long finishedTaskCount;
        long inProgressTaskCount;

        static class Builder {
            {
                dirDiscoveryDelay = DiscoveryDelayStatistics.empty(DiscoveryType.DIR);
                graphDiscoveryDelay = DiscoveryDelayStatistics.empty(DiscoveryType.GRAPH);
                storageDiscoveryDelay = DiscoveryDelayStatistics.empty(DiscoveryType.STORAGE);
                tasksInStatusGraphEvaluationFailed = 0;
                maxAttemptOfRunningSandboxTask = 0;
                maxDurationOfRunningSandboxTask = Duration.ZERO;
                startedTaskCount = 0;
                finishedTaskCount = 0;
                inProgressTaskCount = 0;
            }

            public Builder with(RunningSandboxTasksStatistics stat) {
                maxAttemptOfRunningSandboxTask = stat.maxAttemptNumber;
                maxDurationOfRunningSandboxTask = stat.maxDuration;
                return this;
            }
        }
    }

    @NonNullApi
    static class RunningSandboxTasksStatistics {
        long maxAttemptNumber = 0;
        Duration maxDuration = Duration.ZERO;

        void collect(@Nullable GraphDiscoveryTask task, Instant now) {
            if (task != null) {
                int taskAttemptNumber = task.getAttemptNumber();
                maxAttemptNumber = Math.max(maxAttemptNumber, taskAttemptNumber);

                var taskDuration = Duration.between(task.getGraphEvaluationStartedAt(), now);
                maxDuration = maxDuration.toSeconds() >= taskDuration.toSeconds()
                        ? maxDuration : taskDuration;
            }
        }
    }

    @Value
    @NonNullApi
    static class DiscoveryDelayStatistics {
        DiscoveryType discoveryType;
        long waitingParentCommits;
        Duration delay;
        long lastProcessedSvnRevision;

        Instant statisticsCreatedAt;

        static DiscoveryDelayStatistics empty(DiscoveryType discoveryType) {
            return new DiscoveryDelayStatistics(discoveryType, 0, Duration.ZERO, 0, Instant.MIN);
        }

        static DiscoveryDelayStatistics create(
                Instant now,
                ArcCommit headCommit,
                ArcService arcService,
                CiMainDb db,
                DiscoveryProgressChecker discoveryProgressChecker,
                DiscoveryDelayStatistics previousStat
        ) {
            return db.currentOrReadOnly(() -> {
                var discoveryType = previousStat.discoveryType;
                log.info("Processing delay for {}", discoveryType);
                long waitingParentCommits = countCommitWaitingDiscovery(discoveryType, db);

                Optional<ArcCommit> lastProcessedOptional = discoveryProgressChecker
                        .getLastProcessedCommitInTx(ArcBranch.trunk(), discoveryType)
                        .map(ArcRevision::of)
                        .map(arcService::getCommit);

                log.info("Last processed commit is {}", lastProcessedOptional.orElse(null));

                Duration delay = lastProcessedOptional
                        .map(lastProcessed -> computeDelay(now, headCommit, db, previousStat, lastProcessed))
                        .orElse(Duration.ZERO);
                log.info("Delay between head and last processed commit is {}", delay);

                long lastProcessedSvnRevision = lastProcessedOptional.map(ArcCommit::getSvnRevision).orElse(0L);
                return new DiscoveryDelayStatistics(
                        discoveryType,
                        waitingParentCommits,
                        delay,
                        lastProcessedSvnRevision,
                        now
                );
            });
        }

        private static Duration computeDelay(Instant now, ArcCommit headCommit, CiMainDb db,
                                             DiscoveryDelayStatistics previousStat,
                                             ArcCommit lastProcessed) {
            if (lastProcessed.getCommitId().equals(headCommit.getCommitId())) {
                // when lastCommitDiscovered == trunk head, then delay is zero
                return Duration.ZERO;
            }
            Instant childCommitTime = db.arcCommit().findChildCommitIds(lastProcessed.getCommitId())
                    .stream()
                    .map(child -> db.arcCommit().get(child.getCommitId()))
                    .filter(ArcCommit::isTrunk)
                    .findFirst()
                    .map(ArcCommit::getCreateTime)
                    .orElse(null);
            if (childCommitTime == null) {
                // When `child(lastProcessed)` is not found, let's compute delay as `head - lastProcessed`
                boolean zeroPreviousDelay = previousStat.delay
                        .truncatedTo(ChronoUnit.SECONDS)
                        .toSeconds() == 0;
                if (zeroPreviousDelay && previousStat.statisticsCreatedAt.isAfter(lastProcessed.getCreateTime())) {
                    /* Possible case:
                      - 00:00 (min:sec) created commit-1
                      - 10:00 gathered DiscoveryDelayStatistics
                      - 10:25 created commit-2
                      - 10:30 we are going to gather DiscoveryDelayStatistics, but PullCronTask will be launched
                        only at 10:32, so there is no commit-2 in ArcCommit table
                      In this case we want to return delay equal to 00:25 instead of 10:25.

                      The other way to process this case is fetching history from arc, in order to find commits
                      between commit-1 and commit-2. */
                    childCommitTime = previousStat.statisticsCreatedAt;
                } else {
                    childCommitTime = lastProcessed.getCreateTime();
                }
            }

            return Duration.between(childCommitTime, now);
        }

        private static long countCommitWaitingDiscovery(DiscoveryType discoveryType, CiMainDb db) {
            String targetIndex = switch (discoveryType) {
                case DIR -> CommitDiscoveryProgress.IDX_BY_DIR_DISCOVERY_FINISHED_FOR_PARENTS;
                case GRAPH -> CommitDiscoveryProgress.IDX_BY_GRAPH_DISCOVERY_FINISHED_FOR_PARENTS;
                case STORAGE -> CommitDiscoveryProgress.IDX_BY_STORAGE_DISCOVERY_FINISHED_FOR_PARENTS;
                case PCI_DSS -> CommitDiscoveryProgress.IDX_BY_PCI_DSS_DISCOVERY_FINISHED_FOR_PARENTS;
            };
            String targetColumn = switch (discoveryType) {
                case DIR -> "dirDiscoveryFinishedForParents";
                case GRAPH -> "graphDiscoveryFinishedForParents";
                case STORAGE -> "storageDiscoveryFinishedForParents";
                case PCI_DSS -> "pciDssDiscoveryFinishedForParents";
            };
            return db.currentOrReadOnly(() ->
                    db.commitDiscoveryProgress().count(
                            YqlPredicate.where(targetColumn).eq(false),
                            YqlView.index(targetIndex)
                    )
            );
        }

    }
}
