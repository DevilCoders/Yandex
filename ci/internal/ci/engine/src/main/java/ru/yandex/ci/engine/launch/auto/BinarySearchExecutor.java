package ru.yandex.ci.engine.launch.auto;


import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.VirtualCiProcessId;
import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.PostponeLaunch;
import ru.yandex.ci.core.launch.PostponeLaunch.PostponeStatus;
import ru.yandex.ci.core.launch.PostponeLaunch.StartReason;
import ru.yandex.ci.core.launch.PostponeLaunch.StopReason;
import ru.yandex.ci.core.launch.PostponeLaunchTable;
import ru.yandex.ci.engine.launch.LaunchService;
import ru.yandex.ci.engine.launch.auto.BinarySearchHandler.ComparisonResult;
import ru.yandex.ci.flow.db.CiDb;

@Slf4j
@RequiredArgsConstructor
public class BinarySearchExecutor {

    @Nonnull
    private final DiscoveryProgressChecker discoveryProgressChecker;
    @Nonnull
    private final LaunchService launchService;
    @Nonnull
    private final CiDb db;

    public void execute(BinarySearchHandler binarySearchHandler) {
        new BinarySearchRun(binarySearchHandler.beginExecution()).execute();
    }

    private List<PostponeLaunch> loadLaunches(
            PostponeLaunchTable.ProcessIdView processIdView,
            List<VirtualCiProcessId.VirtualType> virtualTypes
    ) {
        log.info("[{}] Loading postpone launches", processIdView.getProcessId());

        return db.currentOrReadOnly(() -> {
            var trunk = ArcBranch.trunk();
            var table = db.postponeLaunches();

            var firstIncomplete = table.firstIncompleteLaunch(processIdView, trunk, virtualTypes);
            firstIncomplete.ifPresent(launch -> log.info("Last incomplete launch: {}", launch));

            var lastNonIncomplete = firstIncomplete.flatMap(launch -> table.firstPreviousLaunch(launch, virtualTypes));
            lastNonIncomplete.ifPresent(launch -> log.info("Previous launch: {}", launch));

            var svnRevision = firstIncomplete
                    .map(PostponeLaunch::getSvnRevision)
                    .orElse(0L);
            var list = table.findProcessingList(processIdView, trunk, svnRevision, virtualTypes);

            firstIncomplete.ifPresent(launch -> list.add(0, launch));
            lastNonIncomplete.ifPresent(launch -> list.add(0, launch));
            return list;
        });
    }

    @RequiredArgsConstructor
    private class BinarySearchRun {
        private final Map<DiscoveryType, ArcCommit> discoveryCache = new HashMap<>();
        private final BinarySearchHandler.Execution binarySearchExecution;

        void execute() {
            log.info("[{}] Processing binary search", binarySearchExecution);

            // We should not check each processId - could be too much data
            var virtualTypes = binarySearchExecution.getVirtualTypes();
            var postponeLaunches = db.scan().run(() ->
                    db.postponeLaunches().findIncompleteProcesses(ArcBranch.trunk(), virtualTypes));
            log.info("Total postpone processes to handle: {}", postponeLaunches.size());

            for (var e : postponeLaunches) {
                var ciProcessId = CiProcessId.ofString(e.getProcessId());
                var settings = binarySearchExecution.getBinarySearchSettings(ciProcessId);
                if (settings == null) {
                    continue; // ---
                }

                log.info("[{}] has settings: {}", ciProcessId, settings);
                var discoveredCommit = getDiscoveryProgress(settings.getDiscoveryTypes());
                if (discoveredCommit == null) {
                    log.info("Skip processing [{}], no discovered commit found", ciProcessId);
                    continue; // ---
                }

                var launches = loadLaunches(e, virtualTypes).toArray(PostponeLaunch[]::new);

                var processor = new SingleProcessIdExecutor(
                        ciProcessId,
                        launches,
                        discoveredCommit,
                        settings.getMinIntervalDuration(),
                        settings.getMinIntervalSize(),
                        settings.getCloseLaunchesOlderThan(),
                        settings.getMaxActiveLaunches(),
                        settings.isTreatFailedLaunchAsComplete(),
                        binarySearchExecution
                );
                processor.process();
            }
            log.info("[{}] Binary tests processing is complete", binarySearchExecution);
        }

        @Nullable
        private ArcCommit getDiscoveryProgress(Set<DiscoveryType> discoveryTypes) {
            ArcCommit earliestCommit = null;
            for (var discoveryType : discoveryTypes) {
                var arcCommit = getDiscoveryProgress(discoveryType);
                if (arcCommit == null) {
                    return null;
                }
                if (earliestCommit == null) {
                    earliestCommit = arcCommit;
                } else {
                    earliestCommit = arcCommit.getSvnRevision() < earliestCommit.getSvnRevision()
                            ? arcCommit
                            : earliestCommit;
                }
            }
            return earliestCommit;
        }

        @Nullable
        private ArcCommit getDiscoveryProgress(DiscoveryType discoveryType) {
            if (discoveryCache.containsKey(discoveryType)) {
                return discoveryCache.get(discoveryType);
            }

            var lastCommit = db.currentOrReadOnly(() ->
                    discoveryProgressChecker.getLastProcessedCommitInTx(ArcBranch.trunk(), discoveryType));
            if (lastCommit.isEmpty()) {
                log.warn("Unable to find last discovered commit for {}", discoveryType);
                discoveryCache.put(discoveryType, null);
                return null;
            }
            log.info("[{}] Last discovered commit: {}", discoveryType, lastCommit.get());

            // Commit must be loaded already
            var discoveredCommit = db.currentOrReadOnly(() -> db.arcCommit().get(lastCommit.get()));
            log.info("[{}] Discovered commits up to: {}", discoveredCommit, discoveredCommit);
            Preconditions.checkState(discoveredCommit.getSvnRevision() > 0,
                    "[%s] Last discovered commit has invalid svnRevision: %s", discoveryType, discoveredCommit);
            discoveryCache.put(discoveryType, discoveredCommit);
            return discoveredCommit;
        }
    }

    @RequiredArgsConstructor
    private class SingleProcessIdExecutor {
        private final CiProcessId processId;
        private final PostponeLaunch[] launches;
        private final ArcCommit discoveredCommit;
        private final Duration minIntervalDuration;
        private final int minIntervalSize;
        private final Duration closeLaunchesOlderThan;
        private final int maxActiveLaunches;
        private final boolean treatFailedLaunchAsComplete;
        private final BinarySearchHandler.Execution binarySearchExecution;
        private int currentActiveLaunches = 0;

        void process() {
            this.currentActiveLaunches = getNumberOfLaunches(PostponeStatus.STARTED);

            log.info("[{}], total launches to check: {}, current active launches: {}",
                    processId, launches.length, currentActiveLaunches);

            log.info("Comparing with discovered commit {}", commitToString());

            db.currentOrTx(this::startScheduledLaunches); // Try to schedule some outstanding launches
            db.currentOrTx(this::checkStartedLaunches);
            db.currentOrTx(this::startNewLaunches);
            db.currentOrTx(this::startScheduledLaunches); // Try to schedule again (pick scheduled on this cycle)
            db.currentOrTx(this::cancelObsoleteLaunches);

            log.info("[{}] is complete", processId);
        }

        private void cancelObsoleteLaunches() {
            log.info("Canceling obsolete launches...");

            /*
            Check all launches older than `closeLaunchesOlderThan` (in comparison with discovered), from the beginning
            If no running tasks found so far - CANCEL it, i.e. don't wait for SUCCESS launch forever
             */

            for (var i = 0; i < launches.length; i++) {
                var launch = launches[i];
                if (!isObsolete(launch)) {
                    log.info("Not obsolete, stop checking obsolete launches at {}", launchToString(launch));
                    break;
                }

                var status = launch.getStatus();
                if (status.isRunning()) {
                    log.info("Running, stop checking obsolete launches at {}", launchToString(launch));
                    break;
                }

                if (status == PostponeStatus.NEW) {
                    changeStatus(i, PostponeStatus.SKIPPED, null, StopReason.OBSOLETE_LAUNCH);
                }

            }
        }

        private void startScheduledLaunches() {
            for (var i = 0; i < launches.length; i++) {
                if (launches[i].getStatus() == PostponeStatus.START_SCHEDULED) {
                    startLaunch(i, Objects.requireNonNullElse(launches[i].getStartReason(), StartReason.BINARY_SEARCH));
                }
            }
        }

        private void checkStartedLaunches() {
            log.info("Checking started launches...");

            // Check if all launches are complete...
            for (int i = 0; i < launches.length; i++) {
                var launch = launches[i];
                var status = launch.getStatus();
                if (status != PostponeStatus.STARTED && status != PostponeStatus.COMPLETE) {
                    continue; // ---
                }

                log.info("Checking {}", launchToString(launch));
                if (!updateStatusIfRequired(i)) {
                    continue;
                }

                launch = launches[i];
                status = launch.getStatus();

                // Compare against previous started or complete launch
                int prevPos = lookupPreviousStartedOrComplete(i);
                if (prevPos < 0) {
                    // Skip all previous launches if not skipped yet, but only if all first launches are NEW
                    log.info("No previous started launch, skip");
                    if (status == PostponeStatus.COMPLETE) {
                        skipAllFirstLaunches(i);
                    }
                    continue; // ---
                }

                var prevStartedLaunch = launches[prevPos];
                log.info("Previous {}", launchToString(prevStartedLaunch));

                if (!updateStatusIfRequired(prevPos)) {
                    continue;
                }

                closeCompleteLaunches(prevPos, i);
            }
        }

        // The idea is simple - start each large test once in a configured delay
        private void startNewLaunches() {
            log.info("Starting new launches...");

            // Start from last known commit
            int lastStartedIdx = lookupLastStartedOrComplete();
            var lastLaunchIdx = lastStartedIdx;
            var lastLaunch = lastStartedIdx >= 0
                    ? launches[lastStartedIdx]
                    : null;

            var launchesSize = launches.length;
            int startFrom = Math.max(lastStartedIdx, 0);

            if (startFrom < launchesSize) {
                log.info("Starting from r{}, ({} out of {})",
                        launches[startFrom].getSvnRevision(), startFrom, launchesSize);
            }

            for (int i = 0; i < startFrom; i++) {
                var launch = launches[i];
                log.info("Skip {}", launchToString(launch));
            }

            // First cycle - start everything we didn't start yet
            for (int i = startFrom; i < launchesSize; i++) {
                var launch = launches[i];
                log.info("Checking {}", launchToString(launch));
                if (!isFullyDiscovered(launch)) {
                    log.info("Interrupt (not fully discovered yet) < r{}", discoveredCommit.getSvnRevision());
                    break;
                }
                boolean orderedLaunch = lastLaunch != null && lastLaunch.getSvnRevision() < launch.getSvnRevision();

                boolean addLaunch;
                if (i == 0 && lastLaunch == null) {
                    // First launch of a kind
                    addLaunch = true;
                } else if (orderedLaunch && isDelayPassed(lastLaunch, launch) && i - lastLaunchIdx > minIntervalSize) {
                    // Delay passed and commit number covered
                    addLaunch = true;
                } else if (i == launchesSize - 1 && orderedLaunch && isDiscoveryDelayPassed(lastLaunch)) {
                    // Last and discovered
                    addLaunch = true;
                } else {
                    addLaunch = false;
                }
                if (addLaunch) {
                    if (launch.getStatus() == PostponeStatus.NEW) {
                        startLaunch(i, StartReason.DEFAULT);
                    } else {
                        log.info("Skip adding");
                    }
                    lastLaunch = launch;
                    lastLaunchIdx = i;
                }
            }
        }

        private boolean updateStatusIfRequired(int index) {
            var launch = launches[index];
            var status = launch.getStatus();
            if (status == PostponeStatus.STARTED) {
                var launchObject = db.launches().getLaunchStatusView(launch.toLaunchId());
                var launchStatus = launchObject.getStatus();
                switch (launchStatus) {
                    case SUCCESS -> changeStatus(index, PostponeStatus.COMPLETE);
                    case CANCELED -> changeStatus(index, PostponeStatus.CANCELED);
                    case FAILURE -> {
                        var targetStatus = treatFailedLaunchAsComplete
                                ? PostponeStatus.COMPLETE
                                : PostponeStatus.FAILURE;
                        changeStatus(index, targetStatus);
                    }
                    default -> {
                        log.info("{} status is {}, skip", launchObject.getLaunchId(), launchStatus);
                        return false;
                    }
                }
            }
            return true;
        }

        private void closeCompleteLaunches(int first, int second) {
            var postponeLaunchFirst = launches[first];
            var postponeLaunchSecond = launches[second];

            // Do not try to make binary search between failed launches
            if (postponeLaunchFirst.getStatus() != PostponeStatus.COMPLETE ||
                    postponeLaunchSecond.getStatus() != PostponeStatus.COMPLETE) {
                log.info("First launch {}, second launch {}. Cannot run binary diff when one of statuses is not {}",
                        launchToString(postponeLaunchFirst),
                        launchToString(postponeLaunchSecond),
                        PostponeStatus.COMPLETE);
                return; // ---
            }

            var newLaunchesInBetween = getNewLaunches(first + 1, second);
            if (!newLaunchesInBetween.isEmpty()) {
                log.info("Check if there are new failed tests between {} and {}",
                        launchToString(postponeLaunchFirst), launchToString(postponeLaunchSecond));

                // compare with storage
                var launchFirst = db.launches().get(postponeLaunchFirst.toLaunchId());
                var launchSecond = db.launches().get(postponeLaunchSecond.toLaunchId());

                var comparisonResult = hasNewFailedTests(launchFirst, launchSecond);
                var skip = switch (comparisonResult) {
                    case NOT_READY -> {
                        log.info("Test results are not ready, skip this check...");
                        yield true;
                    }
                    case LEFT_CANCELED -> {
                        log.info("Override left launch as canceled");
                        overwriteExternalStatus(first, PostponeStatus.CANCELED);
                        yield true;
                    }
                    case RIGHT_CANCELED -> {
                        log.info("Override right launch as canceled");
                        overwriteExternalStatus(second, PostponeStatus.CANCELED);
                        yield true;
                    }
                    case BOTH_CANCELED -> {
                        log.info("Override both launches as canceled");
                        overwriteExternalStatus(first, PostponeStatus.CANCELED);
                        overwriteExternalStatus(second, PostponeStatus.CANCELED);
                        yield true;
                    }
                    case HAS_NEW_FAILED_TESTS -> {
                        // TODO: configure number of binary searches to run in parallel and compare with this number
                        var numberOfRunningLaunches = getNumberOfRunningLaunches(first + 1, second);
                        if (numberOfRunningLaunches > 0) {
                            log.info("Cannot close range. Cannot start binary search, current active launches: {}",
                                    numberOfRunningLaunches);
                            yield true;
                        }
                        int midPos = newLaunchesInBetween.size() == 1
                                ? newLaunchesInBetween.get(0)
                                : newLaunchesInBetween.get(newLaunchesInBetween.size() / 2);
                        log.info("Cannot close range. Running binary search between launches. Starting from {}",
                                launchToString(launches[midPos]));
                        startLaunch(midPos, StartReason.BINARY_SEARCH);
                        yield true;
                    }
                    case NO_NEW_FAILED_TESTS -> {
                        log.info("No new failed tests between {} and {}",
                                launchToString(postponeLaunchFirst), launchToString(postponeLaunchSecond));
                        yield false;
                    }
                };
                if (skip) {
                    return; // ---
                }
            }


            skipAllPreviousLaunches(first + 1, second);
        }

        private ComparisonResult hasNewFailedTests(Launch launchFrom, Launch launchTo) {
            return binarySearchExecution.hasNewFailedTests(launchFrom, launchTo);
        }

        private void startLaunch(int index, StartReason startReason) {
            if (currentActiveLaunches < maxActiveLaunches) {
                changeStatus(index, PostponeStatus.STARTED, startReason, null);
                launchService.startDelayedOrPostponedLaunch(launches[index].toLaunchId());
                currentActiveLaunches++;
                log.info("Current active launches: {}", currentActiveLaunches);
            } else {
                log.info("Cannot start new launch, max number of launches: {}, current number of active launches: {}",
                        maxActiveLaunches, currentActiveLaunches);
                if (startReason == StartReason.BINARY_SEARCH) {
                    changeStatus(index, PostponeStatus.START_SCHEDULED, startReason, null);
                }
            }
        }

        private void cancelSkippedLaunch(int index) {
            var launch = launches[index];
            if (launch.getStatus() == PostponeStatus.SKIPPED) {
                var launchId = launch.toLaunchId();
                log.info("Canceling skipped launch: {}", launchId);
                launchService.cancelDelayedOrPostponedLaunchInTx(db.launches().get(launchId));
            }
        }

        private void changeStatus(int index, PostponeStatus targetStatus) {
            changeStatus(index, targetStatus, null, null);
        }

        private void changeStatus(
                int index,
                PostponeStatus targetStatus,
                @Nullable StartReason startReason,
                @Nullable StopReason stopReason
        ) {
            var launch = launches[index];
            if (launch.getStatus() != targetStatus) {
                log.info("Schedule status change: {} -> {}, start reason = {}, stop reason = {}",
                        launchToString(launch), targetStatus, startReason, stopReason);

                var targetLaunch = launch.withStatus(targetStatus);
                if (startReason != null) {
                    targetLaunch = targetLaunch.withStartReason(startReason);
                }
                if (stopReason != null) {
                    targetLaunch = targetLaunch.withStopReason(stopReason);
                }
                launches[index] = targetLaunch;
                db.postponeLaunches().save(targetLaunch);

                if (targetStatus.isExecutionComplete()) {
                    currentActiveLaunches--;
                    log.info("Current active launches: {}", currentActiveLaunches);
                }

                binarySearchExecution.onLaunchStatusChange(targetLaunch);
                cancelSkippedLaunch(index);
            } else {
                log.info("Launch already have requested status: {}", launchToString(launch));
            }
        }

        private void overwriteExternalStatus(int index, PostponeStatus targetStatus) {
            var launch = launches[index];
            if (launch.getStatus() != targetStatus) {
                log.info("Schedule status change, override: {} -> {}",
                        launchToString(launch), targetStatus);
                var targetLaunch = launch.withStatus(targetStatus);
                launches[index] = targetLaunch;
                db.postponeLaunches().save(targetLaunch);
            }
        }

        private void skipAllFirstLaunches(int toExcluding) {
            boolean allNew = true;
            for (int i = 0; i < toExcluding; i++) {
                var status = launches[i].getStatus();
                if (status.isRunning()) {
                    allNew = false;
                    break;
                }
            }
            if (allNew) {
                skipAllPreviousLaunches(0, toExcluding);
            }
        }

        private void skipAllPreviousLaunches(int fromIncluding, int toExcluding) {
            for (int i = fromIncluding; i < toExcluding; i++) {
                if (launches[i].getStatus() == PostponeStatus.NEW) {
                    changeStatus(i, PostponeStatus.SKIPPED);
                }
            }
        }

        private List<Integer> getNewLaunches(int fromIncluding, int toExcluding) {
            if (toExcluding <= fromIncluding) {
                return List.of();
            }
            var toLaunch = new ArrayList<Integer>(toExcluding - fromIncluding);
            for (int i = fromIncluding; i < toExcluding; i++) {
                if (launches[i].getStatus() == PostponeStatus.NEW) {
                    toLaunch.add(i);
                }
            }
            return toLaunch;
        }

        private int getNumberOfRunningLaunches(int fromIncluding, int toExcluding) {
            int total = 0;
            for (int i = fromIncluding; i < toExcluding; i++) {
                if (launches[i].getStatus().isRunning()) {
                    total++;
                }
            }
            return total;
        }

        private int lookupPreviousStartedOrComplete(int toExcluding) {
            for (var i = toExcluding - 1; i >= 0; i--) {
                var status = launches[i].getStatus();
                if (status == PostponeStatus.STARTED || status == PostponeStatus.COMPLETE) {
                    return i;
                }
            }
            return -1;
        }

        private int lookupLastStartedOrComplete() {
            return lookupPreviousStartedOrComplete(launches.length);
        }

        private int getNumberOfLaunches(PostponeStatus status) {
            int total = 0;
            for (PostponeLaunch launch : launches) {
                if (launch.getStatus() == status) {
                    total++;
                }
            }
            return total;
        }

        private boolean isFullyDiscovered(PostponeLaunch launch) {
            // 'last discovered' is not actually discovered; it's discovered up `this` commit, excluding `this`
            return launch.getSvnRevision() < discoveredCommit.getSvnRevision();
        }

        boolean isDiscoveryDelayPassed(PostponeLaunch launch) {
            return isDelayPassed(launch.getCommitTime(), discoveredCommit.getCreateTime());
        }

        boolean isDelayPassed(PostponeLaunch from, PostponeLaunch to) {
            return isDelayPassed(from.getCommitTime(), to.getCommitTime());
        }

        private boolean isDelayPassed(Instant previousTime, Instant currentTime) {
            var delay = Duration.between(previousTime, currentTime);
            return delay.toMillis() >= this.minIntervalDuration.toMillis();
        }

        private boolean isObsolete(PostponeLaunch launch) {
            var previousTime = launch.getCommitTime();
            var currentTime = discoveredCommit.getCreateTime();
            var delay = Duration.between(previousTime, currentTime);
            return delay.toMillis() >= this.closeLaunchesOlderThan.toMillis();
        }

        private String commitToString() {
            return "r" + discoveredCommit.getSvnRevision() +
                    ", " + discoveredCommit.getCreateTime();
        }

        private String launchToString(PostponeLaunch postponeLaunch) {
            return "r" + postponeLaunch.getSvnRevision() +
                    ", " + postponeLaunch.getCommitTime() +
                    ", " + postponeLaunch.getStatus();
        }
    }


}
