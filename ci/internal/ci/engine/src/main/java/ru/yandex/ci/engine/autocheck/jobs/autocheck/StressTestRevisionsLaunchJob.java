package ru.yandex.ci.engine.autocheck.jobs.autocheck;

import java.time.Clock;
import java.time.Duration;
import java.time.Instant;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Queue;
import java.util.UUID;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;

import javax.annotation.Nonnull;

import com.google.common.base.Preconditions;
import com.google.gson.JsonObject;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.springframework.scheduling.concurrent.ThreadPoolTaskExecutor;

import ru.yandex.ci.client.observer.CheckRevisionsDto;
import ru.yandex.ci.client.observer.ObserverClient;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.autocheck.AutocheckConstants;
import ru.yandex.ci.core.autocheck.FlowVars;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.discovery.DiscoveredCommitState;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchPullRequestInfo;
import ru.yandex.ci.core.pr.PullRequestDiffSet;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.launch.LaunchService;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.ExecutorInfo;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.util.UserUtils;

import static ru.yandex.ci.core.autocheck.AutocheckConstants.STRESS_TEST_TRUNK_PRECOMMIT_PROCESS_ID;

@RequiredArgsConstructor
@ExecutorInfo(
        title = "Launch precommit autocheck at specified revisions",
        description = "Internal job for launching precommit autocheck at specified revisions"
)
@Slf4j
public class StressTestRevisionsLaunchJob implements JobExecutor {

    public static final UUID ID = UUID.fromString("dc2cdb6c-708d-42e5-880c-a0bcf01d8e86");

    @Nonnull
    private final ObserverClient observerClient;
    @Nonnull
    private final LaunchService launchService;
    @Nonnull
    private final ConfigurationService configurationService;
    @Nonnull
    private final CiMainDb db;
    @Nonnull
    private final Clock clock;
    @Nonnull
    private final ThreadPoolTaskExecutor executor;

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        var flowVars = StressTestFlowVars.getFlowVars(context);
        var durationHours = StressTestFlowVars.getDurationHours(flowVars) + "h";
        var revisionsPerHours = StressTestFlowVars.getRevisionsPerHours(flowVars);
        var cacheNamespace = StressTestFlowVars.getNamespace(flowVars);

        log.info("CacheNamespace {}, revisionsPerHours {}, durationHours {}", cacheNamespace, revisionsPerHours,
                durationHours);

        var revision = context.getFlowLaunch().getVcsInfo().getRevision();
        var revisions = getNotUsedRevisions(durationHours, revisionsPerHours, cacheNamespace, revision);

        revisions = reverseList(revisions);

        var configBundle = configurationService.getLastValidConfig(AutocheckConstants.AUTOCHECK_A_YAML_PATH,
                ArcBranch.trunk());
        Preconditions.checkState(
                configBundle.isReadyForLaunch(),
                "Config bundle is not ready: %s", configBundle.getStatus()
        );

        var progressUpdater = new ProgressUpdater(revisions.size(), Duration.ofMinutes(1), context, clock);

        int intervalMillis = (3600 * 1000 / revisionsPerHours);
        log.info("Interval between launches: ms {}", intervalMillis);

        Queue<CompletableFuture<Launch>> futureLaunches = new ArrayDeque<>();
        while (!revisions.isEmpty()) {
            throwIfFailedFutureIsFound(futureLaunches);

            var targetRevision = revisions.remove(getLastIndex(revisions));

            logRevision("Starting autocheck", targetRevision);

            var futureLaunch = scheduleLaunchStart(targetRevision, cacheNamespace, configBundle, progressUpdater,
                    context.getFlowLaunch().getIdString());
            futureLaunches.add(futureLaunch);

            if (!revisions.isEmpty()) {
                TimeUnit.MILLISECONDS.sleep(intervalMillis);
            }
        }
    }

    @Nonnull
    private List<CheckRevisionsDto> getNotUsedRevisions(
            String durationHours,
            int revisionsPerHours,
            String cacheNamespace,
            OrderedArcRevision revision
    ) {
        var revisions = observerClient.getNotUsedRevisions(revision.getCommitId(), durationHours,
                revisionsPerHours, cacheNamespace);

        log.info("Autocheck queue size: {}", revisions.size());
        if (!revisions.isEmpty()) {
            logRevision("First revision in autocheck queue", revisions.get(0));
            logRevision("Last revision in autocheck queue", revisions.get(getLastIndex(revisions)));
        }

        return revisions;
    }

    private static int getLastIndex(List<CheckRevisionsDto> revisions) {
        return revisions.size() - 1;
    }

    private static void logRevision(String message, CheckRevisionsDto targetRevision) {
        log.info(
                "{}: left {} (r{}), right {} (diffSet {})",
                message,
                targetRevision.getLeft().getRevision(),
                targetRevision.getLeft().getRevisionNumber(),
                targetRevision.getRight().getRevision(),
                targetRevision.getDiffSetId()
        );
    }

    private static List<CheckRevisionsDto> reverseList(List<CheckRevisionsDto> revisions) {
        var result = new ArrayList<>(revisions);
        Collections.reverse(result);
        return result;
    }

    private static void throwIfFailedFutureIsFound(
            Queue<CompletableFuture<Launch>> futureLaunches
    ) throws InterruptedException, ExecutionException {
        var future = futureLaunches.peek();

        while (future != null && future.isDone()) {
            if (future.isCompletedExceptionally()) {
                log.info("Future completed exceptionally, getting result");
                future.get();
            } else {
                futureLaunches.poll();
                future = futureLaunches.peek();
            }
        }
    }

    private CompletableFuture<Launch> scheduleLaunchStart(
            CheckRevisionsDto targetRevision,
            String cacheNamespace,
            ConfigBundle configBundle,
            ProgressUpdater progressUpdater,
            String flowLaunchId
    ) {
        return CompletableFuture.supplyAsync(() -> {
            var branch = ArcBranch.ofString(targetRevision.getRight().getBranch());

            var diffSet = db.currentOrReadOnly(() -> db.pullRequestDiffSetTable().findById(
                    branch.getPullRequestId(), targetRevision.getDiffSetId()
            )).orElseThrow();

            var launchPullRequestInfo = getLaunchPullRequestInfo(diffSet);

            updateDiscoveredCommit(STRESS_TEST_TRUNK_PRECOMMIT_PROCESS_ID, diffSet.getOrderedMergeRevision());

            // TODO: use OnCommitLaunchService, now it's impossible, cause OnCommitLaunchService
            //  gets the last diffset in pullrequest
            var launch = launchService.startPrFlow(
                    STRESS_TEST_TRUNK_PRECOMMIT_PROCESS_ID,
                    diffSet.getOrderedMergeRevision(),
                    UserUtils.loginForInternalCiProcesses(),
                    configBundle,
                    launchPullRequestInfo,
                    LaunchService.LaunchMode.NORMAL,
                    false,
                    createLaunchFlowVars(cacheNamespace)
            );

            progressUpdater.update(targetRevision, launch.getLaunchId());

            observerClient.markAsUsed(
                    targetRevision.getRight().getRevision(),
                    targetRevision.getLeft().getRevision(),
                    cacheNamespace,
                    flowLaunchId
            );
            log.info("Revision {}, marked used for stress test", targetRevision);

            return launch;
        }, executor);
    }

    private JsonObject createLaunchFlowVars(String cacheNamespace) {
        var launchFlowVars = new JsonObject();
        launchFlowVars.addProperty(FlowVars.IS_STRESS_TEST, true);
        launchFlowVars.addProperty(FlowVars.CACHE_NAMESPACE, cacheNamespace);
        return launchFlowVars;
    }

    private LaunchPullRequestInfo getLaunchPullRequestInfo(PullRequestDiffSet diffSet) {
        return new LaunchPullRequestInfo(
                diffSet.getPullRequestId(),
                diffSet.getDiffSetId(),
                diffSet.getAuthor(),
                diffSet.getSummary(),
                diffSet.getDescription(),
                null,
                diffSet.getVcsInfo(),
                diffSet.getIssues(),
                diffSet.getLabels(),
                diffSet.getEventCreated()
        );
    }

    private void updateDiscoveredCommit(CiProcessId processId, OrderedArcRevision orderedRevision) {
        db.currentOrTx(() -> db.discoveredCommit().updateOrCreate(
                processId,
                orderedRevision,
                optionalState -> optionalState.orElseGet(
                        () -> DiscoveredCommitState.builder()
                                .manualDiscovery(true)
                                .build()
                )
        ));
    }

    private static class ProgressUpdater {

        @Nonnull
        private final JobContext context;
        @Nonnull
        private final Duration updatePeriod;
        @Nonnull
        private final Clock clock;
        private final int maxCountValue;

        @Nonnull
        private Instant nextUpdate;
        private int launchStartedCount = 0;

        ProgressUpdater(int maxCountValue,
                        @Nonnull Duration updatePeriod,
                        @Nonnull JobContext context,
                        Clock clock) {
            this.maxCountValue = maxCountValue;
            this.updatePeriod = updatePeriod;
            this.context = context;
            this.nextUpdate = clock.instant();
            this.clock = clock;
        }

        synchronized void update(CheckRevisionsDto revision, LaunchId launchId) {
            launchStartedCount++;

            log.info("Progress {}/{}, launch {}, revision {}", launchStartedCount, maxCountValue, launchId, revision);

            if (clock.instant().isAfter(nextUpdate) || launchStartedCount == maxCountValue) {
                var message = """
                        Progress %d/%d
                        Last launch number %d
                        Left number r%d, revision %s
                        Right %s"""
                        .formatted(
                                launchStartedCount,
                                maxCountValue,
                                launchId.getNumber(),
                                revision.getLeft().getRevisionNumber(),
                                revision.getLeft().getRevision(),
                                revision.getRight().getRevision()
                        );
                context.progress().updateText(message);
                nextUpdate = nextUpdate.plus(updatePeriod);
            }
        }

        synchronized void skipRevision(String rightRevision) {
            launchStartedCount++;
            log.info("Skip used right revision {}", rightRevision);
        }
    }
}
