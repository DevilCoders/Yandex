package ru.yandex.ci.engine.timeline;

import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.RequiredArgsConstructor;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;
import one.util.streamex.StreamEx;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.Branch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.discovery.DiscoveredCommit;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.launch.ReleaseVcsInfo;
import ru.yandex.ci.core.timeline.BranchState;
import ru.yandex.ci.core.timeline.Offset;
import ru.yandex.ci.core.timeline.TimelineBranchItem;
import ru.yandex.ci.core.timeline.TimelineItem;
import ru.yandex.ci.engine.branch.BranchTraverseService;
import ru.yandex.ci.engine.launch.CanStartLaunchAtRevision;

/**
 * Работает с интервалами коммитов в таймлайне. Отвечает на вопрос, какие коммиты входят в релиз или релизную ветку,
 * а также перекладывает коммиты и список отмененных релизов в случае отмены релиза.
 */
@Slf4j
@RequiredArgsConstructor
public class CommitRangeService {
    private final CiMainDb db;
    private final TimelineService timelineService;
    private final BranchTraverseService branchTraverseService;

    /**
     * Рассчитывает интервал коммитов, который должен быть включен в процесс на заданной ревизии.
     * Не выполняет никаких изменений связанных сущностей.
     *
     * @param processId      process id
     * @param selectedBranch выбранная ветка. В ветках возможен запуск только на базовой ревизии.
     *                       Запустить релиз на ревизии, от которой отведена ветка, в транке нельзя,
     *                       можно только в ветке, при этом отличаться будет только selectedBranch.
     * @param revision       ревизия, на которой запускается релиз или отводится ветка
     * @return интервал коммитов
     */
    public Range getCommitsToCapture(CiProcessId processId, ArcBranch selectedBranch, OrderedArcRevision revision) {
        logTimeline(processId, selectedBranch);

        var sameRevisionItem = timelineService.getLastIn(processId, selectedBranch)
                .filter(topCaptured -> topCaptured.getType() == TimelineItem.Type.LAUNCH)
                .filter(topCaptured ->
                        topCaptured.getArcRevision().getCommitId().equals(revision.getCommitId())
                );
        if (sameRevisionItem.isPresent()) {
            log.info("Found same revision item {}, capture single commit", sameRevisionItem);
            return new Range(revision, revision, 0);
        }

        var rangeConsumer = new BranchTraverseService.RangeConsumer() {
            private OrderedArcRevision previousRevision;
            private int commitCount = 0;

            @Override
            public boolean consume(OrderedArcRevision fromRevision, @Nullable OrderedArcRevision unused) {
                var branch = fromRevision.getBranch();
                var offset = Offset.fromRevisionNumberExclusive(fromRevision.getNumber());
                var itemAfter = timelineService.getTimeline(processId, branch, offset, 1)
                        .stream()
                        .findFirst();

                previousRevision = itemAfter.map(TimelineItem::getArcRevision).orElse(null);

                log.info("Item after {} is {}, previous revision {}", fromRevision, itemAfter, previousRevision);

                if (previousRevision == null || previousRevision.fromSameBranch(fromRevision)) {
                    commitCount += countCommitsSameBranch(processId, fromRevision, previousRevision);
                } else {
                    // нашли в таймлайне ревизию с другой ветки, это переехавший элемент
                    log.info("Found item at base revision {}", previousRevision);
                    commitCount += countCommits(processId, fromRevision, previousRevision);
                }

                return previousRevision != null;
            }
        };
        branchTraverseService.traverse(revision, null, rangeConsumer);

        Range range = new Range(revision, rangeConsumer.previousRevision, rangeConsumer.commitCount);
        log.info("Capture commits: {}", range);
        return range;
    }

    /**
     * Проверяет, возможен ли запуск на выбранной ревизии
     *
     * @param selectedBranch выбранная ветка. В ветках возможен запуск только на базовой ревизии.
     *                       Запустить релиз на ревизии, от которой отведена ветка, в транке нельзя,
     *                       можно только в ветке, при этом отличаться будет только selectedBranch.
     */
    public CanStartLaunchAtRevision canStartLaunchAt(
            CiProcessId processId,
            OrderedArcRevision revision,
            ArcBranch selectedBranch
    ) {
        return canStartLaunchAt(processId, revision, selectedBranch, true, false);
    }

    public CanStartLaunchAtRevision canStartLaunchAt(
            CiProcessId processId,
            OrderedArcRevision revision,
            ArcBranch selectedBranch,
            boolean allowLaunchWithoutCommits,
            boolean allowLaunchEarlier
    ) {
        if (processId.getType() != CiProcessId.Type.RELEASE) {
            return CanStartLaunchAtRevision.allowed();
        }
        return timelineService.getLastIn(processId, selectedBranch)
                .map(topCapturedTimelineItem -> {
                    OrderedArcRevision topCaptured = topCapturedTimelineItem.getArcRevision();

                    if (topCaptured.getCommitId().equals(revision.getCommitId())) {
                        if (!allowLaunchWithoutCommits) {
                            return CanStartLaunchAtRevision.forbidden("forbidden starting launch without commits");
                        }
                        return timelineService.canStartReleaseWithoutCommits(
                                topCapturedTimelineItem,
                                selectedBranch,
                                allowLaunchEarlier
                        );
                    }

                    if (topCaptured.getBranch().equals(selectedBranch)) {
                        if (!allowLaunchEarlier) {
                            if (!topCaptured.isBefore(revision)) {
                                var item = switch (topCapturedTimelineItem.getType()) {
                                    case BRANCH -> "branch %s".formatted(
                                            topCapturedTimelineItem.getBranch().getId().getBranch());
                                    case LAUNCH -> "launch number %s".formatted(
                                            topCapturedTimelineItem.getLaunch().getLaunchId().getNumber());
                                };
                                return CanStartLaunchAtRevision.forbidden(
                                        "forbidden at earlier revision then the latest item (%s) revision %s"
                                                .formatted(
                                                        item,
                                                        topCaptured.getCommitId()
                                                ));
                            }
                        }
                        return CanStartLaunchAtRevision.allowed();
                    }

                    // последний занятый коммит в другой ветке
                    // это возможно только если запуск на базовой ревизии ветки
                    Preconditions.checkState(!selectedBranch.isTrunk(),
                            "top captured commit is in branch %s, but timeline was requested for trunk",
                            topCaptured);
                    Preconditions.checkState(topCaptured.getBranch().isTrunk(),
                            "top captured in branch %s, but timeline was requested for another branch %s",
                            topCaptured, selectedBranch);
                    // разрешаем запуск, т.к. передана ревизия ветки, а занята только транковая
                    return CanStartLaunchAtRevision.allowed();
                })
                .orElseGet(() -> {
                    if (selectedBranch.equals(revision.getBranch())) {
                        // timeline для ветки пуст, и запуск на ревизии ветки, значит все свободно
                        return CanStartLaunchAtRevision.allowed();
                    }
                    var baseRevision = branchTraverseService.getBaseRevision(selectedBranch);
                    if (!baseRevision.isBeforeOrSame(revision)) {
                        return CanStartLaunchAtRevision.forbidden(
                                "forbidden at earlier revision then base branch revision %s"
                                        .formatted(baseRevision.getCommitId()));
                    }
                    return CanStartLaunchAtRevision.allowed();
                });
    }

    /**
     * Освобождает коммиты из релиза, например, в случае отмены.
     * Обновляет соседние запуски и ветки в процессе, передавая им освобожденные коммиты
     *
     * @param releaseToFree запуск, интервал коммитов которого будет освобожден.
     * @param newState      состояние в которое переходит релиз
     */
    public void freeCommits(Launch releaseToFree, LaunchState newState) {
        log.info("Freeing commits from release {}", releaseToFree.getLaunchId());
        Preconditions.checkArgument(releaseToFree.getProcessId().getType() == CiProcessId.Type.RELEASE,
                "freeing commits supported only for release processes"
        );
        OrderedArcRevision newPrevious = releaseToFree.getVcsInfo().getPreviousRevision();

        logTimeline(releaseToFree.getProcessId(), releaseToFree.getVcsInfo().getRevision().getBranch());

        Optional<TimelineItem> upcomingItemOptional = timelineService.getNextAfter(releaseToFree);
        if (upcomingItemOptional.isEmpty()) {
            log.info("Launch {} is last in timeline", releaseToFree.getId());
            return;
        }
        TimelineItem upcomingItem = upcomingItemOptional.get();
        log.info("Found upcoming timeline item: {} at revision {}",
                upcomingItem.getNumber(), upcomingItem.getRevision());

        List<TimelineItem> atUpcomingRevision = timelineService.getAtRevision(upcomingItem.getProcessId(),
                upcomingItem.getArcRevision());

        log.info("Found {} items at revision", atUpcomingRevision.size());

        List<TimelineBranchItem> updatedBranches = StreamEx.of(atUpcomingRevision)
                .filter(item -> item.getType() == TimelineItem.Type.BRANCH)
                .map(TimelineItem::getBranch)
                .map(b -> updateBranch(b, newPrevious, releaseToFree, newState))
                .toList();

        if (isReleaseHaveCommits(releaseToFree)) {
            List<Launch> updatedLaunchesInBranches = updatedBranches.stream()
                    .flatMap(ub -> timelineService.getFirstIn(ub.getProcessId(), ub.getArcBranch()).stream())
                    .peek(item -> Preconditions.checkState(
                            item.getType() == TimelineItem.Type.LAUNCH,
                            "Expected launch as a first timeline item in branch %s, but found %s",
                            item.getArcRevision().getBranch(), item
                    ))
                    .map(TimelineItem::getLaunch)
                    .map(l -> updateLaunch(l, newPrevious, releaseToFree))
                    .collect(Collectors.toList());
            updatedLaunchesInBranches.forEach(db.launches()::save);
        }

        List<Launch> updatedLaunches = StreamEx.of(atUpcomingRevision)
                .filter(item -> item.getType() == TimelineItem.Type.LAUNCH)
                .map(TimelineItem::getLaunch)
                .map(l -> updateLaunch(l, newPrevious, releaseToFree))
                .toList();

        updatedBranches.forEach(db.timelineBranchItems()::save);
        updatedLaunches.forEach(db.launches()::save);

        log.info("{} branches updated, {} launches updated", updatedBranches.size(), updatedLaunches.size());
    }

    private void logTimeline(CiProcessId processId, ArcBranch branch) {
        var limit = 30;
        var timelineVersion = timelineService.currentTimelineVersion(processId);
        var timeline = timelineService.getTimeline(processId,
                branch, Offset.EMPTY, limit
        );
        var message = timeline.stream()
                .map(item -> String.format("%s : %s", item.getRevision(), item))
                .collect(Collectors.joining("\n"));

        log.info("Timeline version {}, first {} items:\n{}", timelineVersion, limit, message);
    }

    /**
     * @return ревизию, на которой есть завершенный релиз, и при этом можно выполнить запуск (перезапуск)
     */
    public Optional<OrderedArcRevision> getLastReleaseCommitCanStartAt(CiProcessId processId, ArcBranch branch) {
        return timelineService.getLastIn(processId, branch)
                .filter(timelineItem ->
                        canStartLaunchAt(
                                processId,
                                timelineItem.getArcRevision(),
                                branch
                        ).isAllowed()
                )
                .map(TimelineItem::getArcRevision);
    }

    private Launch updateLaunch(Launch launch, @Nullable OrderedArcRevision newPrevious, Launch cancelledLaunch) {
        Preconditions.checkArgument(launch.getProcessId().equals(cancelledLaunch.getProcessId()));


        ReleaseVcsInfo releaseVcsInfo = launch.getVcsInfo().getReleaseVcsInfo();
        Preconditions.checkState(releaseVcsInfo != null,
                "release launch %s should have releaseVcsInfo, but it's not",
                launch.getLaunchId()
        );


        int cancelledNumber = cancelledLaunch.getLaunchId().getNumber();
        log.info("Adding to cancelled release itself {} and cancelled by current: {}",
                cancelledNumber, cancelledLaunch.getCancelledReleases());

        var launchBuilder = launch.toBuilder();
        launchBuilder.cancelledReleases(cancelledLaunch.getCancelledReleases());
        launchBuilder.cancelledRelease(cancelledNumber);
        launchBuilder.displacedReleases(cancelledLaunch.getDisplacedReleases());
        if (cancelledLaunch.isDisplaced()) {
            launchBuilder.displacedRelease(cancelledNumber);
        }

        if (isReleaseHaveCommits(cancelledLaunch)) {
            log.info("Change previous commit for launch {}: {} -> {}",
                    launch.getId(), launch.getVcsInfo().getPreviousRevision(), newPrevious
            );

            OrderedArcRevision launchRevision = launch.getVcsInfo().getRevision();
            int commitCount = countCommits(launch.getProcessId(), launchRevision, newPrevious);

            log.info("New commit count: {}", commitCount);
            launchBuilder.vcsInfo(launch.getVcsInfo().toBuilder()
                    .commitCount(commitCount)
                    .releaseVcsInfo(releaseVcsInfo.withPreviousRevision(newPrevious))
                    .previousRevision(newPrevious)
                    .build());
        }
        return launchBuilder.build();
    }

    private int countCommits(CiProcessId processId, OrderedArcRevision from, @Nullable OrderedArcRevision to) {
        var commitCounter = new BranchTraverseService.RangeConsumer() {
            private int commitCount;

            @Override
            public boolean consume(OrderedArcRevision fromRevision, @Nullable OrderedArcRevision toRevision) {
                commitCount += countCommitsSameBranch(processId, fromRevision, toRevision);
                log.info("Found {} commits in range {} - {}", commitCount, from, toRevision);
                return false;
            }
        };
        branchTraverseService.traverse(from, to, commitCounter);

        return commitCounter.commitCount;
    }

    private TimelineBranchItem updateBranch(Branch branch, @Nullable OrderedArcRevision newPrevious,
                                            Launch cancelled, LaunchState newState) {
        TimelineBranchItem item = branch.getItem();
        log.info("Updating branch {}", branch.getId());

        BranchState newBranchState = item.getState().registerLaunch(
                cancelled.getLaunchId().getNumber(), newState.getStatus(), true
        );

        var builder = item.toBuilder().state(newBranchState);

        if (isReleaseHaveCommits(cancelled)) {
            OrderedArcRevision previousRevision = item.getVcsInfo().getPreviousRevision();
            OrderedArcRevision baseRevision = branch.getInfo().getBaseRevision();

            log.info("Change previous commit for branch {}: {} -> {}", branch.getId(), previousRevision, newPrevious);

            int trunkCommitCount = countCommits(branch.getProcessId(), baseRevision, newPrevious);
            log.info("New commits count {}", trunkCommitCount);
            builder.vcsInfo(
                    item.getVcsInfo().toBuilder()
                            .previousRevision(newPrevious)
                            .trunkCommitCount(trunkCommitCount)
                            .build()
            );
        } else {
            log.info("Release without commits, updating vcsInfo for branch {} skipped", branch.getInfo());
        }
        return builder.build();
    }

    private int countCommitsSameBranch(CiProcessId processId, OrderedArcRevision revision,
                                       @Nullable OrderedArcRevision previousRevision) {
        long previousNumber = previousRevision != null ? previousRevision.getNumber() : -1;
        return db.discoveredCommit().count(processId, revision.getBranch(), revision.getNumber(), previousNumber);
    }

    public CancelledAndDisplacedLaunches getCancelledAndDisplacedLaunches(
            CiProcessId processId, OrderedArcRevision revision, @Nullable OrderedArcRevision previousRevision) {
        Set<Integer> cancelled = new HashSet<>();
        Set<Integer> displaced = new HashSet<>();

        branchTraverseService.traverse(revision, previousRevision, (fromRevision, toRevision) -> {
            int limitCommits = 500;
            long toRevisionNumber = toRevision == null ? -1 : toRevision.getNumber();
            List<DiscoveredCommit> commits = db.discoveredCommit().findCommitsWithCancelledLaunches(processId,
                    fromRevision.getBranch(), fromRevision.getNumber(), toRevisionNumber,
                    limitCommits
            );
            log.info("Found {} commits from {} to {}", commits.size(), fromRevision, toRevision);
            if (commits.size() == limitCommits) {
                log.warn("found {} commits with cancelled releases, result is truncated", limitCommits);
            }

            commits.stream()
                    .flatMap(commit -> commit.getState().getCancelledLaunchIds().stream())
                    .map(LaunchId::getNumber)
                    .forEach(cancelled::add);

            commits.stream()
                    .flatMap(commit -> commit.getState().getDisplacedLaunchIds().stream())
                    .map(LaunchId::getNumber)
                    .forEach(displaced::add);


            return false;
        });
        return new CancelledAndDisplacedLaunches(cancelled, displaced);
    }

    private static boolean isReleaseHaveCommits(Launch launch) {
        var vcsInfo = launch.getVcsInfo();
        return !Objects.equals(vcsInfo.getPreviousRevision(), vcsInfo.getRevision());
    }

    @Value
    public static class Range {
        @Nonnull
        OrderedArcRevision revision;

        @Nullable
        OrderedArcRevision previousRevision;

        int count;
    }

    @Value
    public static class CancelledAndDisplacedLaunches {
        @Nonnull
        Set<Integer> cancelled;

        @Nonnull
        Set<Integer> displaced;
    }
}
