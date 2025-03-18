package ru.yandex.ci.engine.launch.auto;

import java.time.Instant;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.atomic.AtomicReference;
import java.util.function.Function;
import java.util.stream.Collectors;

import com.google.common.annotations.VisibleForTesting;
import io.micrometer.core.instrument.Counter;
import io.micrometer.core.instrument.Gauge;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.Timer;
import lombok.Data;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.model.ReleaseConfig;
import ru.yandex.ci.core.config.a.model.StageConfig;
import ru.yandex.ci.core.db.model.AutoReleaseQueueItem;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.core.discovery.CommitDiscoveryProgress;
import ru.yandex.ci.core.discovery.DiscoveredCommit;
import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.ci.core.job.TaskUnrecoverableException;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.launch.ConfigHasSecurityProblemsException;
import ru.yandex.ci.engine.launch.LaunchCanNotBeStartedException;
import ru.yandex.ci.engine.launch.LaunchService;
import ru.yandex.ci.engine.launch.NotEligibleForAutoReleaseException;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.LaunchAutoReleaseDelegate;
import ru.yandex.ci.util.UserUtils;
import ru.yandex.lang.NonNullApi;

import static ru.yandex.ci.core.db.model.AutoReleaseQueueItem.State.CHECKING_FREE_STAGE;
import static ru.yandex.ci.core.db.model.AutoReleaseQueueItem.State.WAITING_CONDITIONS;
import static ru.yandex.ci.core.db.model.AutoReleaseQueueItem.State.WAITING_FREE_STAGE;
import static ru.yandex.ci.core.db.model.AutoReleaseQueueItem.State.WAITING_PREVIOUS_COMMITS;
import static ru.yandex.ci.core.db.model.AutoReleaseQueueItem.State.WAITING_SCHEDULE;

@Slf4j
@NonNullApi
public class AutoReleaseService implements LaunchAutoReleaseDelegate {
    private final CiDb db;
    private final ConfigurationService configurationService;
    private final LaunchService launchService;
    private final AutoReleaseSettingsService autoReleaseSettingsService;
    private final RuleEngine ruleEngine;
    private final ReleaseScheduler releaseScheduler;
    private final AutoReleaseQueue autoReleaseQueue;
    private final Statistics statistics;
    private final boolean debugQueue;

    public AutoReleaseService(
            CiDb db,
            ConfigurationService configurationService,
            LaunchService launchService,
            AutoReleaseSettingsService autoReleaseSettingsService,
            RuleEngine ruleEngine,
            ReleaseScheduler releaseScheduler,
            AutoReleaseQueue autoReleaseQueue,
            MeterRegistry meterRegistry,
            boolean debugQueue
    ) {
        this.db = db;
        this.configurationService = configurationService;
        this.launchService = launchService;
        this.autoReleaseSettingsService = autoReleaseSettingsService;
        this.ruleEngine = ruleEngine;
        this.releaseScheduler = releaseScheduler;
        this.autoReleaseQueue = autoReleaseQueue;
        this.statistics = new Statistics(meterRegistry);
        this.debugQueue = debugQueue;
    }

    public void addToAutoReleaseQueue(
            DiscoveredCommit discoveredCommit,
            ReleaseConfig releaseConfig,
            OrderedArcRevision releaseConfigRevision,
            DiscoveryType commitDiscoveredWith,
            Set<DiscoveryType> requiredDiscovery
    ) {
        autoReleaseQueue.addToAutoReleaseQueue(
                discoveredCommit, releaseConfig, releaseConfigRevision, commitDiscoveredWith, requiredDiscovery
        );
    }

    public void processAutoReleaseQueue() {
        log.info("Process auto release queue");
        var autoStartErrorCount = new ProcessingQueueErrors();

        if (debugQueue) {
            log.info("Queue before: {}", autoReleaseQueue.getQueueSnapshotDebugString());
        }

        this.statistics.queueProcessing.record(() -> {
            processReleasesWaitingPreviousCommits();
            processReleasesWaitingConditions(autoStartErrorCount);
            processReleasesWaitingFreeStage();
            processReleasesCheckingFreeStage(autoStartErrorCount);
        });

        if (debugQueue) {
            log.info("Queue after: {}", autoReleaseQueue.getQueueSnapshotDebugString());
        }

        statistics.processingQueueErrors.set(autoStartErrorCount);
        log.info("Processed.");
    }

    @VisibleForTesting
    void processReleasesWaitingPreviousCommits() {
        var releasesInQueueByProcessId = getReleasesInQueueByProcessId(WAITING_PREVIOUS_COMMITS);

        sortByRevisionNumber(releasesInQueueByProcessId);

        var readyByProcessId = new HashMap<String, List<AutoReleaseQueueItem>>(releasesInQueueByProcessId.size());
        db.currentOrReadOnly(() -> {
            var commitProgressCache = new HashMap<String, Optional<CommitDiscoveryProgress>>();

            releasesInQueueByProcessId.forEach((processId, items) -> {
                for (var item : items) {
                    if (checkThatAllParentCommitsWereDiscovered(item, commitProgressCache)) {
                        readyByProcessId.computeIfAbsent(
                                processId, key -> new ArrayList<>(items.size())
                        ).add(item);
                    } else {
                        break;
                    }
                }
            });
        });

        for (var items : readyByProcessId.values()) {
            var lastIndex = items.size() - 1;
            var lastItem = items.remove(lastIndex);

            autoReleaseQueue.changeState(lastItem, WAITING_CONDITIONS);

            var obsoleteItemIds = items.stream().map(AutoReleaseQueueItem::getId).collect(Collectors.toSet());
            autoReleaseQueue.deleteObsoleteAutoReleases(obsoleteItemIds);
        }
    }

    private static void sortByRevisionNumber(Map<String, List<AutoReleaseQueueItem>> releasesInQueueByProcessId) {
        for (var items : releasesInQueueByProcessId.values()) {
            items.sort(Comparator.comparing(it -> it.getOrderedArcRevision().getNumber()));
        }
    }

    private Map<String, List<AutoReleaseQueueItem>> getReleasesInQueueByProcessId(AutoReleaseQueueItem.State state) {
        var releasesInQueue = autoReleaseQueue.findByState(state);

        var releasesInQueueByProcessId = new HashMap<String, List<AutoReleaseQueueItem>>(releasesInQueue.size());
        for (var item : releasesInQueue) {
            releasesInQueueByProcessId.computeIfAbsent(
                    item.getId().getProcessId(),
                    key -> new ArrayList<>()
            ).add(item);
        }

        log.info(
                "Found {} items in state {} for {} processes", releasesInQueue.size(), state,
                releasesInQueueByProcessId.size()
        );

        return releasesInQueueByProcessId;
    }

    private Map<BranchAndProcessId, AutoReleaseQueueItem> removeObsoleteAndGetLatest(AutoReleaseQueueItem.State state) {
        var queuedAutoReleases = autoReleaseQueue.findByState(state);

        var latestAutoReleases = getLastQueueItem(queuedAutoReleases);
        log.info(
                "Found {} items in state {} for {} processes",
                queuedAutoReleases.size(), state, latestAutoReleases.size()
        );

        deleteObsoleteAutoReleases(queuedAutoReleases, latestAutoReleases);
        return latestAutoReleases;
    }

    private boolean checkThatAllParentCommitsWereDiscovered(
            AutoReleaseQueueItem autoReleaseQueueItem,
            Map<String, Optional<CommitDiscoveryProgress>> allParentsDiscoveredCache
    ) {
        return allParentsDiscoveredCache.computeIfAbsent(
                        autoReleaseQueueItem.getId().getCommitId(),
                        commitId -> db.commitDiscoveryProgress().find(commitId))
                .map(progress -> checkThatAllParentCommitsWereDiscovered(
                        progress,
                        autoReleaseQueueItem.getRequiredDiscoveries()
                ))
                .orElse(false);
    }

    static boolean checkThatAllParentCommitsWereDiscovered(
            CommitDiscoveryProgress progress, Set<DiscoveryType> requiredDiscovery
    ) {
        return requiredDiscovery.stream().allMatch(required ->
                switch (required) {
                    case DIR -> progress.isDirDiscoveryFinished()
                            && progress.isDirDiscoveryFinishedForParents();
                    case GRAPH -> progress.isGraphDiscoveryFinished()
                            && progress.isGraphDiscoveryFinishedForParents();
                    case PCI_DSS -> progress.isPciDssStateProcessed()
                            && progress.getPciDssDiscoveryFinishedForParents();
                    case STORAGE -> throw new IllegalStateException(
                            "auto release requires not supported discovery type %s".formatted(required)
                    );
                }
        );
    }

    void processReleasesWaitingConditions(ProcessingQueueErrors processingQueueErrors) {
        var latestAutoReleases = removeObsoleteAndGetLatest(WAITING_CONDITIONS);

        for (var item : latestAutoReleases.values()) {
            tryChangeStateToCheckingFreeStage(item, processingQueueErrors);
        }
    }

    private void tryChangeStateToCheckingFreeStage(
            AutoReleaseQueueItem item,
            ProcessingQueueErrors processingQueueErrors
    ) {
        AutoReleaseContext context;
        try {
            context = AutoReleaseContext.create(db, configurationService, item);
        } catch (NotEligibleForAutoReleaseException | TaskUnrecoverableException e) {
            log.error("{}, error during context creation in `tryChangeStateToCheckingFreeStage`", item.getId(), e);
            processingQueueErrors.otherErrors++;
            return;
        }

        checkAutoReleaseConditions(item, context);
    }

    @VisibleForTesting
    Action checkAutoReleaseConditions(AutoReleaseQueueItem item, AutoReleaseContext context) {
        var conditionResult = ruleEngine.test(
                context.getProcessId(),
                context.getRevision(),
                context.getConditions()
        );
        var state = autoReleaseQueue.computeAutoReleaseState(context);

        if (!state.isEnabled(item.getOrderedArcRevision().getBranch().getType())) {
            if (conditionResult.getAction() != Action.LAUNCH_AND_RESCHEDULE) {
                log.info("deleting {} from queue, cause auto release is disabled: {}, state {}", item.getId(), context,
                        state);
                autoReleaseQueue.dropItem(item);
            }
            return Action.WAIT_COMMITS;
        }

        switch (conditionResult.getAction()) {
            case WAIT_COMMITS -> {
                log.info("deleting {} from queue, cause rule result is to wait more commits: {}",
                        item.getId(), context);
                autoReleaseQueue.dropItem(item);
            }
            case SCHEDULE -> schedule(item, context, conditionResult.getScheduledAt());
            case LAUNCH_RELEASE, LAUNCH_AND_RESCHEDULE -> autoReleaseQueue.changeState(item, CHECKING_FREE_STAGE);
            default -> throw new IllegalStateException("Unsupported action: " + conditionResult.getAction());
        }

        return conditionResult.getAction();
    }

    private void schedule(AutoReleaseQueueItem item, AutoReleaseContext context, Instant scheduledAt) {
        db.currentOrTx(() -> {
            autoReleaseQueue.changeState(item, WAITING_SCHEDULE);
            releaseScheduler.schedule(context.getProcessId(), scheduledAt);
        });
    }

    private void deleteObsoleteAutoReleases(
            List<AutoReleaseQueueItem> queuedAutoReleases,
            Map<BranchAndProcessId, AutoReleaseQueueItem> latestAutoReleasesByProcessId
    ) {
        var obsoleteReleases = queuedAutoReleases.stream()
                .filter(it -> !it.getId().equals(latestAutoReleasesByProcessId.get(BranchAndProcessId.of(it)).getId()))
                .map(AutoReleaseQueueItem::getId)
                .collect(Collectors.toSet());

        autoReleaseQueue.deleteObsoleteAutoReleases(obsoleteReleases);
    }

    @VisibleForTesting
    void processReleasesWaitingFreeStage() {
        removeObsoleteAndGetLatest(WAITING_FREE_STAGE);
    }

    @VisibleForTesting
    void processReleasesCheckingFreeStage(ProcessingQueueErrors autoStartErrorCount) {
        var releasesCheckingFreeStage = autoReleaseQueue.findByState(CHECKING_FREE_STAGE);
        log.info("Checking free stage, {} items found", releasesCheckingFreeStage.size());
        var latestAutoReleases = getLastQueueItem(releasesCheckingFreeStage);
        deleteObsoleteAutoReleases(releasesCheckingFreeStage, latestAutoReleases);

        log.info("Checking free stage, {} processes found", latestAutoReleases.size());

        for (var item : latestAutoReleases.values()) {
            try {
                tryLaunchAutoRelease(item, autoStartErrorCount);
            } catch (NotEligibleForAutoReleaseException | TaskUnrecoverableException e) {
                log.error("Failed to start auto release {}", item.getId(), e);
                autoStartErrorCount.otherErrors++;
            }
        }
    }

    private void tryLaunchAutoRelease(
            AutoReleaseQueueItem item, ProcessingQueueErrors autoStartErrorCount
    ) throws NotEligibleForAutoReleaseException {
        var context = AutoReleaseContext.create(db, configurationService, item);
        var releaseAction = checkAutoReleaseConditions(item, context);

        if (!releaseAction.allowsLaunch()) {
            return;
        }

        if (!firstStageInReleaseFlowIsFree(context)) {
            log.info("First stage is busy: {}", context);
            autoReleaseQueue.changeState(item, WAITING_FREE_STAGE);
            return;
        }

        log.info("Launching auto release: {}", context);

        var configRevision = context.getReleaseConfigRevision();
        if (context.getRevision().getBranch().isRelease()) {
            if (context.getReleaseConfig().getBranches().getDefaultConfigSource().isTrunk()) {
                configRevision = configurationService.getLastValidConfig(
                        context.getProcessId().getPath(), ArcBranch.trunk()
                ).getRevision();
            }
        }

        try {
            launchService.startRelease(
                    context.getProcessId(),
                    context.getRevision().toRevision(),
                    context.getRevision().getBranch(),
                    UserUtils.loginForInternalCiProcesses(),
                    configRevision,
                    false,
                    false,
                    null,
                    releaseAction == Action.LAUNCH_AND_RESCHEDULE,
                    null,
                    null,
                    null
            );
        } catch (LaunchCanNotBeStartedException e) {
            // this happens when user has manually launched release faster then CI
            if (releaseAction == Action.LAUNCH_AND_RESCHEDULE) {
                log.warn("launch failed {} cause commit is not free, recheck conditions", context, e);
                autoReleaseQueue.changeState(item, WAITING_CONDITIONS);
            } else {
                log.warn("launch failed {} cause commit is not free, deleting from queue", context, e);
                autoReleaseQueue.dropItem(item);
            }

            return;
        } catch (ConfigHasSecurityProblemsException e) {
            log.warn("Launch failed {}, cause config has security problems", context, e);
            autoStartErrorCount.configHasSecurityProblems++;
            return;
        } catch (Exception e) {
            log.error("Launch failed {}", context, e);
            autoStartErrorCount.otherErrors++;
            return;
        }

        if (releaseAction == Action.LAUNCH_AND_RESCHEDULE) {
            autoReleaseQueue.changeState(item, WAITING_CONDITIONS);
        } else {
            autoReleaseQueue.dropItem(item);
        }

        this.statistics.releasesAutoStarted.increment();
        log.info("Launched auto release: {}", context);
    }

    public boolean firstStageInReleaseFlowIsFree(AutoReleaseContext context) {
        var stageGroupState = db.currentOrReadOnly(
                () -> db.stageGroup().findOptional(context.getStageGroupId())
        );

        if (stageGroupState.isEmpty()) {
            log.info("Stage is free, cause stageGroupState is not found: {}", context);
            return true;
        }

        var firstStageId = Optional.of(context.getReleaseConfig())
                .map(ReleaseConfig::getStages)
                .orElseGet(List::of)
                .stream()
                .findFirst()
                .map(StageConfig::getId)
                .orElse(StageConfig.IMPLICIT_STAGE.getId()); // No need to fail here

        log.info("First stage '{}', {}", firstStageId, context);

        boolean stageFree = stageGroupState.get().isStageFree(firstStageId);
        log.info("Stage is free {}: {}", stageFree, context);
        return stageFree;
    }

    @Override
    public void scheduleLaunchAfterFlowUnlockedStage(FlowLaunchId flowLaunchId, LaunchId launchId) {
        var processId = launchId.getProcessId();
        if (processId.getType() != CiProcessId.Type.RELEASE) {
            log.info("processId.type != release: flowId {}, launchId {}", flowLaunchId.asString(), processId);
            return;
        }

        db.currentOrTx(() ->
                db.autoReleaseQueue().findByProcessIdAndState(processId, WAITING_FREE_STAGE)
                        .forEach(queuedItem -> autoReleaseQueue.changeState(queuedItem, CHECKING_FREE_STAGE))
        );
    }

    @Override
    public void scheduleLaunchAfterScheduledTimeHasCome(CiProcessId processId) {
        if (processId.getType() != CiProcessId.Type.RELEASE) {
            log.info("processId.type != release: process {}", processId);
            return;
        }

        db.currentOrTx(() ->
                db.autoReleaseQueue().findByProcessIdAndState(processId, WAITING_SCHEDULE)
                        .forEach(queuedItem -> autoReleaseQueue.changeState(queuedItem, WAITING_CONDITIONS))
        );
    }

    public Map<CiProcessId, AutoReleaseState> findAutoReleaseStateOrDefault(List<CiProcessId> processIds) {
        if (processIds.isEmpty()) {
            return Map.of();
        }

        var autoReleaseSettings = autoReleaseSettingsService.findLastForProcessIds(processIds);
        var result = new HashMap<CiProcessId, AutoReleaseState>();

        var configPaths = processIds.stream()
                .map(CiProcessId::getPath)
                .collect(Collectors.toSet());

        var configStates = db.currentOrReadOnly(() -> db.configStates().findByIds(configPaths));
        for (var processId : processIds) {
            var configPath = processId.getPath();
            var configState = configStates.get(configPath);

            if (configState == null || configState.getStatus() != ConfigState.Status.OK) {
                log.info("Default state returned for processId {}: (config found: {}, config status: {})",
                        processId,
                        configState != null,
                        configState != null ? configState.getStatus() : null
                );
                result.put(processId, AutoReleaseState.DEFAULT);
                continue;
            }

            var releaseConfigState = configState.findRelease(processId.getSubId()).orElse(null);

            var autoReleaseState = autoReleaseQueue.computeAutoReleaseState(
                    releaseConfigState, autoReleaseSettings.get(processId), processId
            );
            result.put(processId, autoReleaseState);
        }

        return result;
    }

    public AutoReleaseState findAutoReleaseStateOrDefault(CiProcessId processId) {
        return findAutoReleaseStateOrDefault(List.of(processId)).get(processId);
    }

    public Optional<AutoReleaseState> updateAutoReleaseState(
            CiProcessId processId, boolean enabled, String login, String message
    ) {
        var oldState = findAutoReleaseStateOrDefault(processId);
        if (!oldState.isEditable()) {
            log.info("Auto release is not configured for {}", processId);
            return Optional.empty();
        }

        autoReleaseSettingsService.updateAutoReleaseState(processId, enabled, login, message);
        return Optional.of(findAutoReleaseStateOrDefault(processId));
    }

    private static Map<BranchAndProcessId, AutoReleaseQueueItem> getLastQueueItem(List<AutoReleaseQueueItem> items) {
        var grouped = items.stream().collect(Collectors.groupingBy(BranchAndProcessId::of));

        return grouped.keySet().stream()
                .map(key -> grouped.get(key).stream()
                        .max(Comparator.comparing(it -> it.getOrderedArcRevision().getNumber()))
                        .orElseThrow()
                )
                .collect(Collectors.toMap(BranchAndProcessId::of, Function.identity()));
    }

    @Value
    private static class Statistics {
        AtomicReference<ProcessingQueueErrors> processingQueueErrors;
        Counter releasesAutoStarted;
        Timer queueProcessing;

        Statistics(MeterRegistry meterRegistry) {
            this.processingQueueErrors = new AtomicReference<>(new ProcessingQueueErrors());
            this.releasesAutoStarted = Counter.builder(LaunchService.METRICS_GROUP)
                    .tag("releases", "auto_started")
                    .register(meterRegistry);
            this.queueProcessing = Timer.builder(AutoReleaseMetrics.METRICS_GROUP + ".queue_processing")
                    .register(meterRegistry);

            Gauge.builder(
                            AutoReleaseMetrics.METRICS_GROUP + ".can_not_start_release_due_to_errors",
                            () -> this.processingQueueErrors.get().otherErrors
                    )
                    .tag("error_type", "other")
                    .register(meterRegistry);

            Gauge.builder(
                            AutoReleaseMetrics.METRICS_GROUP + ".can_not_start_release_due_to_errors",
                            () -> this.processingQueueErrors.get().configHasSecurityProblems
                    )
                    .tag("error_type", "config_has_security_problems")
                    .register(meterRegistry);
        }
    }

    @Data
    static class ProcessingQueueErrors {
        int otherErrors;
        int configHasSecurityProblems;
    }

    @Value(staticConstructor = "of")
    static class BranchAndProcessId {
        ArcBranch branch;
        String processId;

        public static BranchAndProcessId of(AutoReleaseQueueItem item) {
            return BranchAndProcessId.of(item.getOrderedArcRevision().getBranch(), item.getId().getProcessId());
        }
    }

}
