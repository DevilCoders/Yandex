package ru.yandex.ci.storage.api.tests;

import java.util.Collection;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.AbstractExecutorService;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ForkJoinPool;
import java.util.function.Function;
import java.util.stream.Collectors;

import lombok.AllArgsConstructor;
import org.apache.commons.collections4.SetUtils;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.constant.Trunk;
import ru.yandex.ci.storage.core.db.model.revision.RevisionEntity;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultEntity;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_mute.TestMuteEntity;
import ru.yandex.ci.storage.core.db.model.test_revision.HistoryFilters;
import ru.yandex.ci.storage.core.db.model.test_revision.HistoryPageCursor;
import ru.yandex.ci.storage.core.db.model.test_revision.HistoryPaging;
import ru.yandex.ci.storage.core.db.model.test_revision.WrappedRevisionsBoundaries;
import ru.yandex.ci.storage.core.db.model.test_statistics.TestStatisticsEntity;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusWithStatistics;

@AllArgsConstructor
public class HistoryService {
    private final CiStorageDb db;
    private final int historyPageSize;
    private final int wrappedPageSize;

    private final AbstractExecutorService muteExecutor;

    public HistoryService(CiStorageDb db, int historyPageSize, int wrappedPageSize, int parallelism) {
        this.db = db;
        this.historyPageSize = historyPageSize;
        this.wrappedPageSize = wrappedPageSize;
        this.muteExecutor = new ForkJoinPool(parallelism);
    }

    public void shutdown() {
        this.muteExecutor.shutdown();
    }

    public Optional<TestStatusEntity.Id> findTestByOldId(String oldId) {
        var tests = db.currentOrReadOnly(() -> db.tests().find(oldId));
        if (tests.isEmpty()) {
            return Optional.empty();
        }

        var fromTrunk = tests.stream()
                .filter(x -> x.getId().getBranch().equals(Trunk.name())).findFirst()
                .map(TestStatusEntity::getId);
        if (fromTrunk.isPresent()) {
            return fromTrunk;
        }

        return Optional.of(tests.get(0).getId());
    }

    public List<TestStatusWithStatistics> getTestInfo(TestStatusEntity.Id testId) {
        return db.currentOrReadOnly(() -> this.getTestInfoInTx(testId));
    }

    private List<TestStatusWithStatistics> getTestInfoInTx(TestStatusEntity.Id testId) {
        var statuses = getTestStatuses(testId).stream()
                .filter(x -> x.getStatus() != Common.TestStatus.TS_NONE)
                .collect(
                        Collectors.toMap(x -> new TestStatisticsEntity.Id(x.getId()), x -> x)
                );

        // we can do it with inner join, but for now keep it simple.
        var statistics = this.db.testStatistics().find(statuses.keySet());

        var revisionIds = statuses.values().stream()
                .map(x -> new RevisionEntity.Id(x.getRevisionNumber(), x.getId().getBranch()))
                .collect(Collectors.toSet());

        var revisions = this.db.revisions().find(revisionIds);

        if (revisions.size() != revisionIds.size()) {
            notFoundRevisions(testId, revisionIds, revisions);
        }

        var revisionsMap = revisions.stream().collect(
                Collectors.toMap(x -> x.getId().getNumber(), Function.identity())
        );

        try {
            // replace with async io when orm supports it
            var mutes = muteExecutor.submit(() ->
                    statuses.values().parallelStream()
                            .map(TestStatusEntity::getId)
                            // orm takes transaction from thread, so we have to create tx
                            .map(id -> db.currentOrReadOnly(() -> this.db.testMutes().getLastAction(id)))
                            .filter(Optional::isPresent)
                            .map(Optional::get)
                            .collect(Collectors.toMap(x -> x.getId().getTestId(), Function.identity()))
            ).get();

            return statistics.stream()
                    .map(s -> getTestStatusWithStatistics(statuses, revisionsMap, mutes, s))
                    .collect(Collectors.toList());
        } catch (InterruptedException | ExecutionException e) {
            throw new RuntimeException(e);
        }
    }

    private TestStatusWithStatistics getTestStatusWithStatistics(
            Map<TestStatisticsEntity.Id, TestStatusEntity> statuses,
            Map<Long, RevisionEntity> revisions,
            Map<TestStatusEntity.Id, TestMuteEntity> mutes,
            TestStatisticsEntity statistics
    ) {
        var status = statuses.get(statistics.getId());
        return new TestStatusWithStatistics(
                status, statistics, revisions.get(status.getRevisionNumber()),
                mutes.get(status.getId())
        );
    }

    private List<TestStatusEntity> getTestStatuses(TestStatusEntity.Id testId) {
        if (testId.getToolchain().isEmpty() || testId.getToolchain().equals(TestEntity.ALL_TOOLCHAINS)) {
            return db.tests().find(testId.getTestId(), testId.getSuiteId(), testId.getBranch());
        }

        return List.of(db.tests().get(testId));
    }

    public TestHistoryPage getTestHistory(
            TestStatusEntity.Id statusId, HistoryFilters historyFilters, HistoryPaging historyPaging
    ) {
        return db.currentOrReadOnly(() -> this.getTestHistoryInTx(statusId, historyFilters, historyPaging));
    }

    public Map<WrappedRevisionsBoundaries, Long> countRevisions(
            TestStatusEntity.Id testId,
            HistoryFilters filters,
            Collection<WrappedRevisionsBoundaries> values
    ) {
        return db.currentOrReadOnly(() -> this.db.testRevision().countRevisions(testId, filters, values));
    }

    private TestHistoryPage getTestHistoryInTx(
            TestStatusEntity.Id statusId, HistoryFilters historyFilters, HistoryPaging historyPaging
    ) {
        var testRevisions = db.testImportantRevision().getHistoryRevisions(
                statusId, historyFilters, historyPaging, historyPageSize + 1
        );

        if (historyPaging.getFromRevision() == 0 &&
                (historyPaging.getToRevision() == 0 || testRevisions.size() < historyPageSize + 1)) {
            var lastRevision = db.testRevision().getLast(statusId, historyFilters);
            if (lastRevision.isPresent()) {
                if (!testRevisions.contains(lastRevision.get().getId().getRevision())) {
                    testRevisions.add(0, lastRevision.get().getId().getRevision());
                }
            }
        }

        var results = db.testRevision().loadOnRevisions(
                statusId, testRevisions.stream().limit(historyPageSize).collect(Collectors.toList())
        );

        var revisionsIds = results.stream()
                .map(x -> new RevisionEntity.Id(x.getId().getRevision(), statusId.getBranch()))
                .collect(Collectors.toSet());

        var revisions = db.revisions().find(revisionsIds).stream()
                .collect(Collectors.toMap(r -> r.getId().getNumber(), Function.identity()));

        if (revisions.size() != revisionsIds.size()) {
            notFoundRevisions(statusId, revisionsIds, revisions.values());
        }

        var orderedRevisions = testRevisions.stream().distinct().sorted().toList();

        var comparator = Comparator.comparing((TestHistoryPage.TestRevision x) -> x.getRevision().getId().getNumber());
        return new TestHistoryPage(
                results.stream().collect(
                        Collectors.groupingBy(x -> x.getId().getRevision())
                ).entrySet().stream().map(e -> {
                    var revision = revisions.get(e.getKey());
                    var index = orderedRevisions.indexOf(e.getKey());
                    Long previous;

                    if (index == 0 && historyPaging.getToRevision() > 0) {
                        previous = historyPaging.getToRevision();
                    } else if (index > 0) {
                        previous = orderedRevisions.get(index - 1);
                    } else {
                        previous = null;
                    }

                    var hasWrapped = previous == null ?
                            e.getValue().stream().anyMatch(r -> r.getPreviousRevision() > 0 &&
                                    revision.getId().getNumber() > historyFilters.getToRevision()
                            ) :
                            e.getValue().stream().anyMatch(r -> r.getPreviousRevision() > previous);

                    return new TestHistoryPage.TestRevision(
                            revision,
                            e.getValue(),
                            hasWrapped ?
                                    new WrappedRevisionsBoundaries(
                                            e.getKey(), previous == null ?
                                            0 : Math.max(previous, historyFilters.getToRevision())
                                    ) : null
                    );
                }).sorted(comparator.reversed()).toList(),
                getPageFilters(testRevisions, historyPaging)
        );
    }

    private HistoryPageCursor getPageFilters(List<Long> revisions, HistoryPaging inPaging) {
        if (inPaging.getFromRevision() > 0) {
            return HistoryPageCursor.builder()
                    .next(
                            HistoryPaging.from(revisions.size() > historyPageSize ? revisions.get(historyPageSize) : 0)
                    )
                    .previous(HistoryPaging.to(inPaging.getFromRevision()))
                    .build();
        } else if (inPaging.getToRevision() > 0) {
            return HistoryPageCursor.builder()
                    .next(HistoryPaging.from(inPaging.getToRevision()))
                    .previous(
                            HistoryPaging.to(
                                    revisions.size() > historyPageSize ? revisions.get(historyPageSize - 1) : 0
                            ))
                    .build();
        } else {
            return HistoryPageCursor.builder()
                    .next(
                            HistoryPaging.from(
                                    revisions.size() > historyPageSize ? revisions.get(historyPageSize) : 0
                            )
                    )
                    .previous(HistoryPaging.EMPTY)
                    .build();
        }
    }

    public List<LaunchesByStatus> getLaunches(TestStatusEntity.Id statusId, long revision) {
        return db.currentOrReadOnly(() -> this.getLaunchesInTx(statusId, revision));
    }

    private List<LaunchesByStatus> getLaunchesInTx(TestStatusEntity.Id statusId, long revision) {
        var numberOfLaunchesByStatus = this.db.testLaunches().countLaunches(statusId, revision);
        if (numberOfLaunchesByStatus.isEmpty()) {
            return List.of();
        }

        var launches = this.db.testLaunches().getLastLaunches(statusId, revision).values().stream()
                .collect(Collectors.toMap(x -> x.getId().toRunId(), Function.identity()));

        var runs = this.db.testResults().find(
                launches.keySet()).stream().collect(Collectors.toMap(TestResultEntity::getId, Function.identity())
        );

        return launches.entrySet().stream()
                .map(e -> new LaunchesByStatus(
                                e.getValue().getStatus(),
                                numberOfLaunchesByStatus.get(e.getValue().getStatus()),
                                getLastRun(runs, e.getKey())
                        )
                )
                .toList();
    }

    private TestResultEntity getLastRun(
            Map<TestResultEntity.Id, TestResultEntity> runs,
            TestResultEntity.Id runId
    ) {
        var run = runs.get(runId);
        if (run != null) {
            return run;
        }

        return TestResultEntity.builder()
                .id(runId)
                .status(Common.TestStatus.TS_NONE)
                .build();
    }

    public WrappedRevisionsFrame getWrappedRevisions(
            TestStatusEntity.Id statusId,
            HistoryFilters filters,
            WrappedRevisionsBoundaries boundaries
    ) {
        return db.currentOrReadOnly(() -> getWrappedRevisionsInTx(statusId, filters, boundaries));
    }

    private WrappedRevisionsFrame getWrappedRevisionsInTx(
            TestStatusEntity.Id statusId, HistoryFilters filters, WrappedRevisionsBoundaries boundaries
    ) {
        var revisionNumbers = db.testRevision().getWrappedRevisions(statusId, filters, boundaries, wrappedPageSize);

        var testRevisions = db.testRevision().loadOnRevisions(statusId, revisionNumbers);

        if (testRevisions.isEmpty()) {
            return new WrappedRevisionsFrame(List.of());
        }

        var minRevision = testRevisions.stream().mapToLong(x -> x.getId().getRevision()).min().orElse(0L);

        var revisionsIds = testRevisions.stream()
                .map(x -> new RevisionEntity.Id(x.getId().getRevision(), statusId.getBranch()))
                .collect(Collectors.toSet());

        var revisions = db.revisions().find(revisionsIds).stream()
                .collect(Collectors.toMap(x -> x.getId().getNumber(), Function.identity()));

        if (revisionsIds.size() != revisions.size()) {
            notFoundRevisions(statusId, revisionsIds, revisions.values());
        }

        var comparator = Comparator.comparing((TestHistoryPage.TestRevision x) -> x.getRevision().getId().getNumber());
        var result = testRevisions.stream().collect(
                        Collectors.groupingBy(x -> x.getId().getRevision())
                ).entrySet().stream()
                .map(e -> new TestHistoryPage.TestRevision(revisions.get(e.getKey()), e.getValue(), null))
                .sorted(comparator.reversed()).collect(Collectors.toList());

        if (revisionNumbers.size() == wrappedPageSize) {
            var updatedBoundaries = new WrappedRevisionsBoundaries(minRevision, boundaries.getTo());
            var last = new TestHistoryPage.TestRevision(result.get(result.size() - 1), updatedBoundaries);
            result.set(result.size() - 1, last);
        }

        return new WrappedRevisionsFrame(result);
    }

    private void notFoundRevisions(
            TestStatusEntity.Id statusId,
            Set<RevisionEntity.Id> revisionsIds,
            Collection<RevisionEntity> arcRevisions
    ) {
        var missing = SetUtils.difference(
                revisionsIds,
                arcRevisions.stream().map(RevisionEntity::getId).collect(Collectors.toSet())
        );

        throw new RuntimeException(
                "One or more revisions not found for test %s, revisions: [%s]".formatted(
                        statusId,
                        missing.stream().map(Object::toString).collect(Collectors.joining(", "))
                )
        );
    }
}
