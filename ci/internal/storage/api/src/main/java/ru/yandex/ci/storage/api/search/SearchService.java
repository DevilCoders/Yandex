package ru.yandex.ci.storage.api.search;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import com.google.common.base.Stopwatch;
import lombok.extern.slf4j.Slf4j;

import yandex.cloud.repository.db.IsolationLevel;

import ru.yandex.ci.storage.api.StorageFrontApi;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultEntity;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.DiffPageCursor;
import ru.yandex.ci.storage.core.db.model.test_diff.DiffSearchFilters;
import ru.yandex.ci.storage.core.db.model.test_diff.SuitePageCursor;
import ru.yandex.ci.storage.core.db.model.test_diff.SuiteSearchFilters;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffEntity;

@Slf4j
public class SearchService {
    private final CiStorageDb db;
    private final int suitePageSize;
    private final int testPageSize;

    public SearchService(CiStorageDb db, int suitePageSize, int testPageSize) {
        this.db = db;
        this.suitePageSize = suitePageSize;
        this.testPageSize = testPageSize;
    }

    public SuiteSearchResult searchSuites(
            CheckIterationEntity.Id id, SuiteSearchFilters filters, boolean useImportantIndex
    ) {
        var stopwatch = Stopwatch.createStarted();

        var results = useImportantIndex && canUseImportantIndex(filters)
                ? searchImportantSuites(id, filters)
                : searchAllSuites(id, filters);

        log.info(
                "Suite search for iteration {} ydb query took {}ms", id, stopwatch.elapsed(TimeUnit.MILLISECONDS)
        );

        if (results.isEmpty()) {
            return new SuiteSearchResult(results, new SuitePageCursor(null, null));
        }

        return new SuiteSearchResult(
                results.stream()
                        .limit(suitePageSize)
                        .sorted(Comparator.comparing(x -> x.getId().getPath()))
                        .collect(Collectors.toList()),
                getPageFilters(results, filters.getPageCursor())
        );
    }

    private boolean canUseImportantIndex(SuiteSearchFilters filters) {
        return filters.getCategory().equals(StorageFrontApi.CategoryFilter.CATEGORY_CHANGED) ||
                !filters.getSpecialCases().equals(StorageFrontApi.SpecialCasesFilter.SPECIAL_CASE_NONE);
    }

    private List<TestDiffEntity> searchAllSuites(CheckIterationEntity.Id id, SuiteSearchFilters filters) {
        if (filters.getResultTypes().size() == 1) {
            return this.db.readOnly().withStatementIsolationLevel(IsolationLevel.STALE_CONSISTENT_READ_ONLY).run(
                    () -> this.db.testDiffs().searchSuites(id, filters, suitePageSize + 1)
            );
        }

        return this.db.scan().run(() -> this.db.testDiffs().searchSuites(id, filters, suitePageSize + 1));
    }

    private List<TestDiffEntity> searchImportantSuites(CheckIterationEntity.Id id, SuiteSearchFilters filters) {
        return this.db.readOnly().withStatementIsolationLevel(IsolationLevel.STALE_CONSISTENT_READ_ONLY).run(
                () -> this.db.importantTestDiffs().searchSuites(id, filters, suitePageSize + 1).stream()
                        .map(TestDiffEntity::new).collect(Collectors.toList())
        );
    }

    public SuiteListResult listSuite(
            TestDiffEntity.Id diffId, SuiteSearchFilters filters, int page, boolean useSuiteIndex
    ) {
        var stopwatch = Stopwatch.createStarted();

        page = Math.max(1, page);
        var skip = (page - 1) * this.testPageSize;
        var take = this.testPageSize + 1;
        var results = useSuiteIndex ?
                listSuiteWithIndex(diffId, filters, skip, take) : listSuiteWithoutIndex(diffId, filters, skip, take);

        log.info(
                "List suite {}, ydb query took {}ms",
                diffId, stopwatch.elapsed(TimeUnit.MILLISECONDS)
        );

        return new SuiteListResult(
                results.stream()
                        .limit(testPageSize)
                        .collect(Collectors.toList()),
                results.size() > this.testPageSize
        );
    }

    private List<TestDiffEntity> listSuiteWithIndex(
            TestDiffEntity.Id diffId, SuiteSearchFilters filters, int skip, int take
    ) {
        return this.db.readOnly().withStatementIsolationLevel(IsolationLevel.STALE_CONSISTENT_READ_ONLY).run(
                () -> {
                    var diff = this.db.testDiffs().find(diffId);
                    if (diff.isEmpty()) {
                        return List.of();
                    }

                    return this.db.testDiffsByHash().listSuite(
                                    diff.get().getAggregateIdHash(), diffId, filters, skip, take

                            ).stream()
                            .map(TestDiffEntity::new)
                            .collect(Collectors.toList());
                }
        );
    }

    private List<TestDiffEntity> listSuiteWithoutIndex(
            TestDiffEntity.Id diffId, SuiteSearchFilters filters, int skip, int take
    ) {
        return this.db.readOnly().withStatementIsolationLevel(IsolationLevel.STALE_CONSISTENT_READ_ONLY).run(
                () -> this.db.testDiffs().listSuite(diffId, filters, skip, take)
        );
    }

    public DiffListResult listDiff(TestDiffEntity.Id diffId) {
        var stopwatch = Stopwatch.createStarted();

        var result = this.db.readOnly().withStatementIsolationLevel(IsolationLevel.STALE_CONSISTENT_READ_ONLY)
                .run(() -> {
                    var toolchains = new HashSet<String>();
                    if (diffId.getToolchain().equals(TestEntity.ALL_TOOLCHAINS)) {
                        var iteration = this.db.checkIterations().get(diffId.getIterationId());
                        toolchains.addAll(iteration.getStatistics().getToolchains().keySet());
                        toolchains.remove(TestEntity.ALL_TOOLCHAINS);
                    } else {
                        toolchains.add(diffId.getToolchain());
                    }

                    var lastIteration = this.db.checkIterations()
                            .findLast(diffId.getCheckId(), diffId.getIterationId().getIterationType())
                            .orElseThrow();

                    var diffs = this.db.testDiffs().listDiff(diffId, toolchains, lastIteration.getId().getNumber());

                    var runs = this.db.testResults().find(
                            diffId.getIterationId().getCheckId(),
                            diffId.getIterationId().getIterationType(),
                            diffId.getCombinedTestId().getSuiteId(),
                            diffId.getCombinedTestId().getToolchain(),
                            diffId.getTestId()
                    ).stream().collect(Collectors.groupingBy(x -> x.getId().getToolchain()));

                    return new DiffListResult(
                            diffs.stream()
                                    .filter(x -> x.getId().equals(diffId))
                                    .findAny().orElseThrow(),
                            diffs.stream()
                                    .filter(x -> !x.getId().getToolchain().equals(TestEntity.ALL_TOOLCHAINS))
                                    .map(diff -> createDiffWithRuns(runs, diff))
                                    .collect(Collectors.toList())
                    );
                });

        log.info(
                "List diff {} ydb query took {}ms", diffId, stopwatch.elapsed(TimeUnit.MILLISECONDS)
        );

        return result;
    }

    private DiffListResult.DiffWithRuns createDiffWithRuns(
            Map<String, List<TestResultEntity>> runsByToolchain, TestDiffEntity diff
    ) {
        var runs = runsByToolchain.getOrDefault(diff.getId().getToolchain(), List.of());

        if (diff.getId().getIterationNumber() == 0) {
            return new DiffListResult.DiffWithRuns(diff, runs);
        }

        return new DiffListResult.DiffWithRuns(
                diff,
                runs.stream()
                        .filter(x -> x.getId().getIterationNumber() == diff.getId().getIterationNumber())
                        .collect(Collectors.toList())
        );
    }

    public Optional<TestResultEntity> getTestRun(
            CheckIterationEntity.Id id, TestEntity.Id testId, String taskId, int partition, int retryNumber
    ) {
        return this.db.readOnly().withStatementIsolationLevel(IsolationLevel.STALE_CONSISTENT_READ_ONLY)
                .run(() -> Optional.ofNullable(this.db.testResults().find(id, testId, taskId, partition, retryNumber)));
    }

    public DiffSearchResult searchDiffs(CheckIterationEntity.Id iterationId, DiffSearchFilters filters) {
        var stopwatch = Stopwatch.createStarted();

        var result = this.db.readOnly().withStatementIsolationLevel(IsolationLevel.STALE_CONSISTENT_READ_ONLY)
                .run(() -> {
                    var from = (filters.getPage() - 1) * suitePageSize;
                    var results = this.db.testDiffs().searchDiffs(iterationId, filters, from, suitePageSize + 1);

                    return new DiffSearchResult(
                            results.stream()
                                    .limit(suitePageSize)
                                    .sorted(Comparator.comparing(x -> x.getId().toString()))
                                    .collect(Collectors.toList()),
                            getPageFilters(results, filters.getPage())
                    );
                });

        log.info(
                "Search diffs for iteration {} ydb query took {}ms",
                iterationId,
                stopwatch.elapsed(TimeUnit.MILLISECONDS)
        );

        return result;
    }

    private DiffPageCursor getPageFilters(List<TestDiffEntity> results, int page) {
        return DiffPageCursor.builder()
                .backwardPageId(results.size() > 0 ? page - 1 : 0)
                .forwardPageId(results.size() > suitePageSize ? page + 1 : 0)
                .build();
    }

    private SuitePageCursor getPageFilters(List<TestDiffEntity> results, SuitePageCursor inDiffPageCursor) {
        var forward = (SuitePageCursor.Page) null;
        var backward = (SuitePageCursor.Page) null;

        if (inDiffPageCursor.getForward() != null) {
            if (results.size() > suitePageSize) {
                forward = toPage(results.get(suitePageSize).getId());
            }

            backward = inDiffPageCursor.getForward();
        } else if (inDiffPageCursor.getBackward() != null) {
            if (results.size() > suitePageSize) {
                backward = toPage(results.get(suitePageSize - 1).getId());
            }

            forward = inDiffPageCursor.getBackward();
        } else {
            if (results.size() > suitePageSize) {
                forward = toPage(results.get(suitePageSize).getId());
            }
        }

        return new SuitePageCursor(forward, backward);
    }

    private SuitePageCursor.Page toPage(TestDiffEntity.Id id) {
        return new SuitePageCursor.Page(id.getCombinedTestId().getSuiteId(), id.getPath());
    }

    public List<String> getSuggest(
            CheckIterationEntity.Id iterationId, Common.CheckSearchEntityType entityType, String value
    ) {
        if (value.length() <= 1) {
            return List.of();
        }

        var numberOfStars = value.chars().filter(ch -> ch == '*').count();
        var searchValue = numberOfStars == 0 ? value : value + "*";

        var result = this.db.currentOrReadOnly(
                () -> this.db.checkTextSearch().findSuggest(iterationId, entityType, searchValue)
        );

        if (numberOfStars == 1 && entityType == Common.CheckSearchEntityType.CSET_PATH && !result.isEmpty()) {
            result = new ArrayList<>(result);
            result.add(0, value);
        }

        return result;
    }
}
