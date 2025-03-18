package ru.yandex.ci.engine.launch;

import java.time.Clock;
import java.time.Instant;
import java.util.Collection;
import java.util.Comparator;
import java.util.Objects;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import org.joda.time.Duration;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementDto;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.model.CleanupConfig;
import ru.yandex.ci.core.config.a.model.CleanupReason;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.discovery.DiscoveredCommitState;
import ru.yandex.ci.core.launch.Activity;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchPullRequestInfo;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.launch.LaunchStatistics;
import ru.yandex.ci.engine.branch.BranchService;
import ru.yandex.ci.engine.event.EventPublisher;
import ru.yandex.ci.engine.event.LaunchEvent;
import ru.yandex.ci.engine.notification.xiva.XivaNotifier;
import ru.yandex.ci.engine.pr.PullRequestService;
import ru.yandex.ci.engine.timeline.CommitRangeService;
import ru.yandex.ci.flow.engine.definition.common.UpstreamLink;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.FlowLaunchUpdateDelegate;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.commune.bazinga.BazingaTaskManager;

public class LaunchStateSynchronizer implements FlowLaunchUpdateDelegate {
    private static final Logger log = LoggerFactory.getLogger(LaunchStateSynchronizer.class);

    /**
     * Джоба, помеченная этим тегом определяет статус всего лонча.
     * Когда она завершается, флоу считается завершенным.
     */
    public static final String FINAL_JOB_TAG = "release-final-job";

    private final CiMainDb db;
    private final PullRequestService pullRequestService;
    private final BranchService branchService;
    private final CommitRangeService commitRangeService;
    private final EventPublisher eventPublisher;
    private final BazingaTaskManager bazingaTaskManager;
    private final Clock clock;
    private final XivaNotifier xivaNotifier;

    public LaunchStateSynchronizer(
            CiMainDb db,
            PullRequestService pullRequestService,
            BranchService branchService,
            CommitRangeService commitRangeService,
            EventPublisher eventPublisher,
            BazingaTaskManager bazingaTaskManager,
            Clock clock,
            XivaNotifier xivaNotifier
    ) {
        this.db = db;
        this.pullRequestService = pullRequestService;
        this.branchService = branchService;
        this.commitRangeService = commitRangeService;
        this.eventPublisher = eventPublisher;
        this.bazingaTaskManager = bazingaTaskManager;
        this.clock = clock;
        this.xivaNotifier = xivaNotifier;
    }

    @Override
    public void flowLaunchUpdated(FlowLaunchEntity flowLaunch) {
        db.currentOrTx(() -> {
            Instant finished = null;

            boolean waitingForCleanup = false;
            Launch launch = db.launches().get(flowLaunch.getLaunchId());
            LaunchState.Status newStatus = calculateStatus(launch, flowLaunch);
            log.info("Launch {} calculates new status: {}", launch.getId(), newStatus);

            var oldStatus = launch.getStatus();
            if (!oldStatus.isTerminal() && allowStartCleanup(launch, newStatus)) {
                log.info("Try moving to cleanup state: {} -> {}", oldStatus, newStatus);

                // Flow переведен в конечный статус
                // Нужно проверить наличие cleanup задач
                if (shouldWaitForCleanup(launch, flowLaunch)) {
                    waitingForCleanup = true;
                    if (newStatus == LaunchState.Status.SUCCESS) {
                        newStatus = LaunchState.Status.WAITING_FOR_CLEANUP;
                    }
                    log.info("Flow will wait for cleanup");

                    var cleanupTrigger = flowLaunch.getFlowInfo().getCleanupConfig();
                    if (cleanupTrigger != null && cleanupTrigger.getWithDelay() != null) {
                        var delay = cleanupTrigger.getWithDelay();
                        log.info("Scheduling cleanup task with delay: {}", delay);
                        bazingaTaskManager.schedule(new LaunchCleanupTask(launch.getId(), CleanupReason.FINISH),
                                org.joda.time.Instant.now().plus(Duration.millis(delay.toMillis())));
                    }
                }
            }

            if (newStatus.isTerminal()) {
                finished = flowLaunch.getJobs().values()
                        .stream()
                        .map(JobState::getLastLaunch)
                        .filter(Objects::nonNull)
                        .map(l -> l.getLastStatusChange().getDate())
                        .max(Comparator.naturalOrder())
                        .orElse(null);
            }

            String statusText = flowLaunch.getJobs().values()
                    .stream()
                    .map(JobState::getLastLaunch)
                    .filter(Objects::nonNull)
                    .max(Comparator.comparing(jobLaunch -> jobLaunch.getLastStatusChange().getDate()))
                    .map(JobLaunch::getStatusText)
                    .orElse("");

            if (newStatus == LaunchState.Status.CANCELED
                    || (newStatus == LaunchState.Status.FAILURE
                    && flowLaunch.getLaunchId().getProcessId().getType() != CiProcessId.Type.RELEASE)
            ) {
                finished = clock.instant();
            }

            LaunchState newState = new LaunchState(
                    flowLaunch.getFlowLaunchId().asString(),
                    flowLaunch.getCreatedDate(),
                    finished,
                    newStatus,
                    statusText
            );

            if (launch.getState().equals(newState)) {
                simpleLaunchUpdate(launch, waitingForCleanup);
                return;
            }

            var statistics = calculateStatistics(flowLaunch);

            notifyArcanum(launch, newState);
            log.info("New launch {} status is {}", flowLaunch.getLaunchId(), newStatus);

            stateUpdated(launch, newState, waitingForCleanup, statistics);
        });
    }

    private LaunchStatistics calculateStatistics(FlowLaunchEntity flowLaunch) {
        var retries = flowLaunch.getJobs().values().stream()
                .mapToInt(s -> s.getLaunches().isEmpty() ? 0 : s.getLaunches().size() - 1)
                .sum();
        return LaunchStatistics.builder()
                .retries(retries)
                .build();
    }

    @SuppressWarnings("FutureReturnValueIgnored")
    public void stateUpdated(Launch launch, LaunchState newState, boolean startWaitingForCleanup,
                             @Nullable LaunchStatistics statistics) {
        if (newState.getStatus() == LaunchState.Status.CANCELED) {
            db.discoveredCommit().updateOrCreate(
                    launch.getLaunchId().getProcessId(),
                    launch.getVcsInfo().getRevision(), stateOptional -> {
                        DiscoveredCommitState.Builder stateBuilder = stateOptional.map(DiscoveredCommitState::toBuilder)
                                .orElseThrow();
                        stateBuilder.cancelledLaunchId(launch.getLaunchId());
                        if (launch.isDisplaced()) {
                            stateBuilder.displacedLaunchId(launch.getLaunchId());
                        }
                        return stateBuilder.build();
                    }
            );

            if (launch.getProcessId().getType() == CiProcessId.Type.RELEASE) {
                commitRangeService.freeCommits(launch, newState);
            }
        }

        var updatedLaunchBuilder = launch.toBuilder()
                .status(newState.getStatus())
                .statusText(newState.getText())
                .flowLaunchId(
                        newState.getFlowLaunchId()
                                .map(FlowLaunchId::of)
                                .map(FlowLaunchId::asString)
                                .orElse(null)
                )
                .started(newState.getStarted().orElse(null))
                .finished(newState.getFinished().orElse(null));

        if (startWaitingForCleanup) {
            updatedLaunchBuilder.waitingForCleanup(true);
        }
        if (statistics != null) {
            updatedLaunchBuilder.statistics(statistics);
        }
        var newActivity = Activity.fromStatus(newState.getStatus());
        if (newActivity == Activity.ACTIVE || launch.getActivity() != newActivity) {
            updatedLaunchBuilder.activity(newActivity);
            updatedLaunchBuilder.activityChanged(Instant.now(clock));
        }

        var updatedLaunch = updatedLaunchBuilder.build();
        branchService.updateTimelineBranchAndLaunchItems(updatedLaunch);
        db.launches().save(updatedLaunch);

        // todo: move to separate class NotificationService?
        eventPublisher.publish(new LaunchEvent(updatedLaunch, clock));
        xivaNotifier.onLaunchStateChanged(updatedLaunch, launch);
    }

    private void simpleLaunchUpdate(Launch launch, boolean startWaitingForCleanup) {
        if (startWaitingForCleanup) {
            db.launches().save(launch.toBuilder().waitingForCleanup(true).build());
        }
    }

    private boolean allowStartCleanup(Launch launch, LaunchState.Status newStatus) {
        if (newStatus.isTerminal()) {
            return true;
        }
        var cleanupConfig = launch.getFlowInfo().getCleanupConfig();
        if (cleanupConfig == null) {
            return CleanupConfig.DEFAULT_OPTIONS.contains(newStatus);
        } else {
            return cleanupConfig.getOnStatus().contains(newStatus);
        }
    }

    private boolean shouldWaitForCleanup(Launch launch, FlowLaunchEntity flowLaunch) {
        // Очистка для релизов пока не поддерживается
        return !flowLaunch.isStaged() &&
                flowLaunch.hasCleanupJobs() &&
                !flowLaunch.isCleanupRunning() &&
                !launch.isWaitingForCleanup();
    }


    private LaunchState.Status calculateStatus(Launch launch, FlowLaunchEntity flowLaunch) {
        if (isCancelledState(launch, flowLaunch)) {
            return LaunchState.Status.CANCELED;
        }

        if (isCancelling(launch)) {
            return LaunchState.Status.CANCELLING;
        }

        if (isSuccessState(flowLaunch)) {
            if (launch.isWaitingForCleanup() && !flowLaunch.isCleanupRunning()) {
                return LaunchState.Status.WAITING_FOR_CLEANUP;
            }
            return LaunchState.Status.SUCCESS;
        }

        return flowLaunch.getState().getLaunchStatus();
    }

    private static boolean isCancelling(Launch launch) {
        return launch.getStatus() == LaunchState.Status.CANCELLING;
    }

    private static boolean isSuccessState(FlowLaunchEntity flowLaunch) {
        if (flowLaunch.isStaged()) {
            // Когда staged-пайплайн доходит до конца, он автоматически дизейблится. Поэтому здесь достаточно просто
            // отреагировать на дизейблинг.

            if (flowLaunch.isDisabled()) {
                return true;
            }
        }

        Collection<JobState> jobs = flowLaunch.getJobs().values();
        if (jobs.stream()
                .anyMatch(job -> job.getLastLaunch() != null
                        && job.getLastStatusChangeType() != null
                        && !job.getLastStatusChangeType().isFinished()
                )
        ) {
            return false;
        }

        var jobsToCheck = jobs.stream().filter(JobState::isVisible);

        if (jobs.stream().anyMatch(x -> x.hasTag(FINAL_JOB_TAG))) {
            jobsToCheck = jobsToCheck.filter(j -> j.hasTag(FINAL_JOB_TAG));
        } else {
            var upstreamIds = jobs.stream()
                    .flatMap(x -> x.getUpstreams().stream().map(UpstreamLink::getEntity))
                    .collect(Collectors.toSet());

            jobsToCheck = jobsToCheck.filter(j -> !upstreamIds.contains(j.getJobId()));
        }

        if (jobsToCheck.allMatch(LaunchStateSynchronizer::isJobSuccessful)) {
            log.info("All jobs are successful, finishing release");
            return true;
        }

        return false;
    }

    private static boolean isCancelledState(Launch launch, FlowLaunchEntity flowLaunch) {
        if (launch.getStatus() == LaunchState.Status.CANCELED) {
            return true;
        }

        if (launch.getStatus() != LaunchState.Status.CANCELLING) {
            return false;
        }

        return flowLaunch.isDisabled();
    }

    private static boolean isJobSuccessful(JobState jobState) {
        if (jobState.getLastLaunch() != null) {
            log.info(
                    "Checking job {} with status history {}",
                    jobState.getJobId(),
                    jobState.getLastLaunch().getStatusHistory().stream()
                            .map(s -> s.getType().toString())
                            .collect(Collectors.joining(", "))
            );
        } else {
            log.info(
                    "Checking job {} without status history", jobState.getJobId()
            );
        }

        return jobState.isSuccessful();
    }

    private void notifyArcanum(Launch launch, LaunchState newState) {
        if (!launch.isNotifyPullRequest()) {
            return;
        }

        ArcanumMergeRequirementDto.Status oldArcanumStatus = toArcanumStatus(launch.getStatus());
        ArcanumMergeRequirementDto.Status newArcanumStatus = toArcanumStatus(newState.getStatus());
        if (oldArcanumStatus == newArcanumStatus) {
            log.info("Skip updating Arcanum, status {} not changed", oldArcanumStatus);
            return;
        }

        LaunchPullRequestInfo pullRequestInfo = launch.getVcsInfo().getPullRequestInfo();
        Preconditions.checkState(pullRequestInfo != null, "Pull request info cannot be null");
        Preconditions.checkState(pullRequestInfo.getRequirementId() != null, "Requirement id cannot be null");

        log.info(
                "Updating Arcanum, PR {}, diffSet {}, requirement {} with status {}",
                pullRequestInfo.getPullRequestId(),
                pullRequestInfo.getDiffSetId(),
                pullRequestInfo.getRequirementId(),
                newArcanumStatus
        );

        pullRequestService.sendMergeRequirementStatus(
                pullRequestInfo,
                pullRequestInfo.getRequirementId(),
                newArcanumStatus,
                launch.getProject(),
                launch.getLaunchId(),
                null
        );
    }

    private static ArcanumMergeRequirementDto.Status toArcanumStatus(LaunchState.Status status) {
        return switch (status) {
            // Возможно, WAITING_FOR_CLEANUP нужно разделить на подстатусы (WFC_SUCCESS, WFC_CANCELED и т.д. ?)
            case SUCCESS, WAITING_FOR_CLEANUP -> ArcanumMergeRequirementDto.Status.SUCCESS;
            case FAILURE -> ArcanumMergeRequirementDto.Status.FAILURE;
            case CANCELED -> ArcanumMergeRequirementDto.Status.CANCELLED;
            case STARTING -> ArcanumMergeRequirementDto.Status.UNKNOWN;
            default -> ArcanumMergeRequirementDto.Status.PENDING;
        };
    }

}

