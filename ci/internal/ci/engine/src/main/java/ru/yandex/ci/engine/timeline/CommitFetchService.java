package ru.yandex.ci.engine.timeline;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.common.collect.SetMultimap;
import lombok.Builder;
import lombok.RequiredArgsConstructor;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.CommitId;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.BranchInfo;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.discovery.DiscoveredCommit;
import ru.yandex.ci.core.discovery.DiscoveredCommitTable;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.timeline.ReleaseCommit;
import ru.yandex.ci.core.timeline.TimelineItem;
import ru.yandex.ci.engine.branch.BranchService;
import ru.yandex.ci.util.OffsetResults;

/**
 * Фетчинг и паджинация по интервалам коммитов.
 */
@Slf4j
@RequiredArgsConstructor
public class CommitFetchService {
    private final CiMainDb db;
    private final ArcService arcService;
    private final BranchService branchService;
    private final TimelineService timelineService;

    /**
     * Возвращает коммиты по заданному интервалу.
     * Интервал должен быть рассчитан где-то снаружи в зависимости от специфики запроса.
     */
    public OffsetResults<ReleaseCommit> fetchCommits(
            CommitRange range,
            CommitOffset offset,
            int limit
    ) {
        return OffsetResults.builder()
                .withTransactionReadOnly(db)
                .withItems(
                        limit,
                        newLimit -> {
                            var discoveredCommits = fetchDiscoveredCommits(range.trimToOffset(offset), newLimit);
                            return enrichCommits(range.getProcessId(), discoveredCommits);
                        }
                )
                .withTotal(() -> countCommits(range))
                .fetch();
    }

    public OffsetResults<TimelineCommit> fetchTimelineCommits(
            LaunchId launchId,
            CommitOffset offset,
            int limit,
            boolean includeFromActiveReleases
    ) {
        if (launchId.getProcessId().getType().isRelease()) {
            var range = buildCommitRangeForReleaseLaunch(launchId, includeFromActiveReleases);
            return fetchTimelineCommits(launchId, range, offset, limit);
        }

        return fetchTimelineCommitsForAction(launchId, limit);
    }

    private OffsetResults<TimelineCommit> fetchTimelineCommitsForAction(
            LaunchId launchId,
            int limit
    ) {
        return OffsetResults.builder()
                .withTransactionReadOnly(db)
                .withItems(limit, unused -> {
                    var launch = db.launches()
                            .findOptional(launchId)
                            .orElseThrow(() -> GrpcUtils.notFoundException("Launch not found " + launchId));
                    var revision = launch.getVcsInfo().getRevision();
                    var commit = arcService.getCommit(revision);
                    return List.of(TimelineCommit.forSingleActionCommit(commit, revision));
                })
                .withTotal(() -> 1L)
                .fetch();
    }

    public Map<Launch.Id, TimelineCommit> getTimelineCommitsForActions(List<Launch> launches) {
        var response = new HashMap<Launch.Id, TimelineCommit>(launches.size());
        for (var launch : launches) {
            Preconditions.checkState(!launch.getProcessId().getType().isRelease(),
                    "Cannot collect list of timeline commits for release %s", launch.getProcessId());

            var revision = launch.getVcsInfo().getRevision();
            var commit = arcService.getCommit(revision);
            response.put(launch.getId(), TimelineCommit.forSingleActionCommit(commit, revision));
        }
        return response;
    }

    private OffsetResults<TimelineCommit> fetchTimelineCommits(
            LaunchId rangeOwner,
            CommitRange range,
            CommitOffset offset,
            int limit
    ) {
        log.info("Commit range: {}; offset: {}; limit: {}; for: {}", range, offset, limit, rangeOwner);

        return OffsetResults.builder()
                .withTransactionReadOnly(db)
                .withItems(
                        limit,
                        newLimit -> {
                            var discoveredCommits = fetchDiscoveredCommits(range.trimToOffset(offset), newLimit);
                            var commits = enrichCommits(range.getProcessId(), discoveredCommits);

                            var activeLaunches = db.launches().getActiveLaunches(range.getProcessId());
                            // поддерживаем возможность запросить коммиты для завершившегося релиза
                            var currentLaunch = activeLaunches.stream()
                                    .filter(l -> l.getLaunchId().equals(rangeOwner))
                                    .findFirst()
                                    .orElseGet(() -> db.launches().findOptional(rangeOwner)
                                            .orElseThrow(() -> new RuntimeException("cannot find launch " + rangeOwner))
                                    );

                            var launchRanges = Stream.concat(activeLaunches.stream(), Stream.of(currentLaunch))
                                    .map(l -> new LaunchRange(
                                            CommitRange.builder()
                                                    .fromRevision(l.getVcsInfo().getRevision())
                                                    .toRevision(l.getVcsInfo().getPreviousRevision())
                                                    .build(),
                                            l
                                    ))
                                    .toList();

                            log.info("Launches: {}", launchRanges);

                            return commits
                                    .stream()
                                    .map(releaseCommit -> {
                                        var release = launchRanges.stream()
                                                .filter(l -> l.getRange().contains(releaseCommit.getRevision()))
                                                .map(LaunchRange::getLaunch)
                                                .findFirst()
                                                .orElse(null);
                                        return new TimelineCommit(releaseCommit, release);
                                    })
                                    .filter(rc -> {
                                        var hasRelease = rc.getRelease() != null;
                                        if (!hasRelease) {
                                            // есть текущий релиз и все активные
                                            // мы не нашли ни одного активного релиза, который бы содержал коммит
                                            // значит коммит уже не в активном релизе и уехал в стейбл
                                            // в случае отмены релиза, его коммиты передаются наверх,
                                            // поэтому этот интервал
                                            // будет закрыт следующим релизом в таймлайне
                                            log.info("Skipped commit {}, doesn't belong to any release",
                                                    rc.getCommit().getRevision().getCommitId());
                                        }
                                        return hasRelease;
                                    })
                                    .toList();
                        }
                )
                .withTotal(() -> countCommits(range))
                .fetch();
    }

    private long countCommits(CommitRange range) {
        // TODO: использовать BranchTraverseService
        CiProcessId processId = range.getProcessId();
        if (range.isSingleBranch() || range.getFromBranch().isTrunk()) {
            return db.discoveredCommit().count(
                    processId, range.getFromBranch(), range.getFromCommitNumber(), range.getToCommitNumber()
            );
        }

        BranchInfo branchInfo = branchService.getBranch(range.getFromBranch())
                .orElseThrow(() -> new RuntimeException("branch %s not found".formatted(range.getFromBranch())));

        OrderedArcRevision baseRevision = branchInfo.getBaseRevision();

        int fromBranchCount = db.discoveredCommit().count(
                processId, range.getFromBranch(), range.getFromCommitNumber(), -1
        );

        int tailCommitCount = 0;
        if (baseRevision.getNumber() != range.getToCommitNumber()) {
            tailCommitCount = db.discoveredCommit().count(
                    processId, baseRevision.getBranch(), baseRevision.getNumber(), range.getToCommitNumber()
            );
        }

        log.info("Found {} commits in branch {} and {} tail commits in branch {}",
                fromBranchCount, range.getFromBranch(),
                tailCommitCount, baseRevision.getBranch()
        );

        return (long) fromBranchCount + (long) tailCommitCount;
    }

    private List<DiscoveredCommit> fetchDiscoveredCommits(CommitRange range, int limit) {
        if (range.isEmpty()) {
            // нечего возвращать с таким оффсетом, но это и не повод выдавать ошибку
            return List.of();
        }

        // TODO: можно использовать BranchTraverseService
        CiProcessId processId = range.getProcessId();
        log.info("Fetching commits in {}, limit {}", range, limit);
        ArcBranch fromBranch = range.getFromBranch();
        if (range.isSingleBranch() || range.getFromBranch().isTrunk()) {
            log.info("Range is in single branch, use single request");
            return db.discoveredCommit().findCommits(
                    processId, fromBranch, range.getFromCommitNumber(), range.getToCommitNumber(), limit
            );
        }

        log.info("Branch in the begin of range doesn't equal to branch in the end." +
                " Use two requests. from {} -> to {}", range.getFromBranch(), range.getToBranch());
        var commits = db.discoveredCommit().findCommits(
                processId, fromBranch, range.getFromCommitNumber(), -1, limit
        );
        var discoveredCommits = new ArrayList<>(commits);

        log.info("Found {} commits in {}", discoveredCommits.size(), fromBranch);

        if (fromBranch.isReleaseOrUser()) {
            if (discoveredCommits.size() < limit || limit <= 0) {
                var commitsInBaseBranch = fetchDiscoveredCommitsFromBaseBranch(range, limit, processId,
                        discoveredCommits);
                log.info("Found {} tail commits", commitsInBaseBranch.size());
                discoveredCommits.addAll(commitsInBaseBranch);
            }
        } else {
            log.info("Skip loading tail commits for non-persisted branch: {}", fromBranch);
        }
        return discoveredCommits;
    }

    private List<DiscoveredCommit> fetchDiscoveredCommitsFromBaseBranch(
            CommitFetchService.CommitRange range,
            int limit,
            CiProcessId processId,
            List<DiscoveredCommit> discoveredCommits
    ) {
        var fromBranch = range.getFromBranch();
        BranchInfo branchInfo = branchService.getBranch(fromBranch)
                .orElseThrow(() -> new RuntimeException("branch %s not found".formatted(fromBranch)));

        OrderedArcRevision baseRevision = branchInfo.getBaseRevision();
        ArcBranch baseBranch = baseRevision.getBranch();
        int tailLimit = limit - discoveredCommits.size();

        log.info("There are no more commits in {} branch. Loading rest {} commits from {} starting at {}",
                fromBranch, tailLimit, baseBranch, baseRevision
        );
        if (baseRevision.getNumber() != range.getToCommitNumber()) {
            return db.discoveredCommit().findCommits(processId,
                    baseRevision.getBranch(), baseRevision.getNumber(), range.getToCommitNumber(), tailLimit);
        }
        return List.of();
    }

    public CommitRange buildCommitRangeForReleaseLaunch(LaunchId launchId, boolean includeFromActiveReleases) {
        log.info("Requesting commits for release launch {}", launchId);

        Launch launch = db.currentOrReadOnly(() -> db.launches().findOptional(launchId))
                .orElseThrow(() -> GrpcUtils.notFoundException("Launch not found " + launchId));


        var toRevision = includeFromActiveReleases
                ? timelineService.getPreviousStable(launch).map(TimelineItem::getArcRevision).orElse(null)
                : launch.getVcsInfo().getPreviousRevision();

        return CommitRange.builder()
                .processId(launchId.getProcessId())
                .fromRevision(launch.getVcsInfo().getRevision())
                .toRevision(toRevision)
                .build();
    }

    public List<ReleaseCommit> enrichCommits(CiProcessId processId, List<DiscoveredCommit> discoveredCommits) {
        if (discoveredCommits.isEmpty()) {
            return List.of();
        }
        Set<ArcRevision> commitIds = discoveredCommits.stream()
                .map(DiscoveredCommit::getCommitId)
                .map(ArcRevision::of)
                .collect(Collectors.toSet());

        Set<LaunchId> launchIds = discoveredCommits.stream()
                .flatMap(dc -> dc.getState().getCancelledLaunchIds().stream())
                .collect(Collectors.toSet());

        Map<LaunchId, Launch> cancelledLaunches = db.currentOrReadOnly(() ->
                db.launches().getProcessLaunches(launchIds)
                        .stream()
                        .collect(Collectors.toMap(Launch::getLaunchId, Function.identity()))
        );

        SetMultimap<CommitId, ArcBranch> branches =
                branchService.getBranchesAtRevisions(processId, commitIds);

        List<ReleaseCommit> commits = new ArrayList<>(discoveredCommits.size());

        for (DiscoveredCommit discoveredCommit : discoveredCommits) {
            ArcCommit arcCommit = arcService.getCommit(discoveredCommit.getArcRevision());

            List<Launch> commitCancelledLaunches = discoveredCommit.getState()
                    .getCancelledLaunchIds().stream()
                    .distinct()
                    .map(cancelledLaunches::get)
                    .collect(Collectors.toList());

            commits.add(
                    new ReleaseCommit(
                            arcCommit,
                            discoveredCommit.getArcRevision(),
                            commitCancelledLaunches,
                            branches.get(arcCommit.getRevision()),
                            discoveredCommit.getState()
                    )
            );
        }

        return Collections.unmodifiableList(commits);
    }

    @Value(staticConstructor = "of")
    public static class CommitOffset {
        ArcBranch branch;
        long number;

        public boolean isPresent() {
            return number > 0;
        }

        public static CommitOffset empty() {
            var offset = CommitOffset.of(ArcBranch.trunk(), -1);
            Preconditions.checkState(!offset.isPresent());
            return offset;
        }
    }

    @Value
    @Builder(toBuilder = true)
    public static class CommitRange {
        CiProcessId processId;

        // порядок коммитов от более поздних к более ранним, т.е. fromCommit > toCommit
        ArcBranch fromBranch;

        long fromCommitNumber;

        @Nullable
        ArcBranch toBranch;

        long toCommitNumber;

        public boolean isEmpty() {
            // Do not use @lombok.Builder.Default, ever
            return isSingleBranch() && toCommitNumber > -1 && fromCommitNumber <= toCommitNumber;
        }

        public boolean isSingleBranch() {
            return fromBranch.equals(toBranch) || (toBranch == null && fromBranch.isTrunk());
        }

        public CommitRange trimToOffset(CommitOffset offset) {
            if (!offset.isPresent()) {
                return this;
            }

            var offsetNumber = offset.getNumber() == 1
                    ? DiscoveredCommitTable.LTE_COMMIT_ZERO
                    : offset.getNumber() - 1;

            if (offset.getBranch().equals(fromBranch)) {
                return toBuilder()
                        .fromCommitNumber(Math.min(fromCommitNumber, offsetNumber))
                        .build();
            }

            return toBuilder()
                    .fromCommitNumber(offsetNumber)
                    .fromBranch(offset.getBranch())
                    .build();
        }

        /**
         * Проверяет, что коммит входит в интервал.
         * Может проверять коммиты которые на той же самой линии истории, что и интервал.
         * Т.е. если интервал начинается в ветке и заканчивается транком и когда
         * 1. коммит из другой ветки
         * 2. коммит из транка, но позже отведения ветки
         * то результат неопределен.
         */
        public boolean contains(OrderedArcRevision revision) {
            // Не конфузимся тут, помним, что fromCommit это более поздний коммит. Интервал назад в историю.
            if (toBranch != null) {
                if (toBranch.equals(fromBranch)) {
                    return revision.getBranch().equals(fromBranch)
                            && revision.getNumber() <= fromCommitNumber
                            && revision.getNumber() > toCommitNumber;
                } else {
                    if (revision.getBranch().equals(fromBranch)) {
                        return revision.getNumber() <= fromCommitNumber;
                    } else if (revision.getBranch().equals(toBranch)) {
                        return revision.getNumber() > toCommitNumber;
                    } else {
                        return false;
                    }
                }
            } else if (revision.getBranch().equals(fromBranch)) {
                return revision.getNumber() <= fromCommitNumber;
            } else {
                return revision.getBranch().isTrunk();
            }
        }

        public static class Builder {

            {
                fromCommitNumber = Long.MAX_VALUE;
                toCommitNumber = -1;
            }

            public Builder toRevision(@Nullable OrderedArcRevision revision) {
                if (revision == null) {
                    toCommitNumber(-1);
                    toBranch(null);
                } else {
                    toCommitNumber(revision.getNumber());
                    toBranch(revision.getBranch());
                }
                return this;
            }

            public Builder fromRevision(OrderedArcRevision revision) {
                fromCommitNumber(revision.getNumber());
                fromBranch(revision.getBranch());
                return this;
            }
        }
    }

    @Value
    private static class LaunchRange {
        CommitRange range;
        Launch launch;
    }
}
