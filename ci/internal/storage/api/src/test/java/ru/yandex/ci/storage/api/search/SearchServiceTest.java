package ru.yandex.ci.storage.api.search;

import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.storage.api.StorageFrontApi;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.StorageYdbTestBase;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.ExtendedStageStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.ExtendedStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.IterationStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.StageStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.metrics.Metrics;
import ru.yandex.ci.storage.core.db.model.check_text_search.CheckTextSearchEntity;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.task_result.TestResult;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultEntity;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.DiffSearchFilters;
import ru.yandex.ci.storage.core.db.model.test_diff.SuitePageCursor;
import ru.yandex.ci.storage.core.db.model.test_diff.SuiteSearchFilters;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffByHashEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffImportantEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffStatistics;
import ru.yandex.ci.storage.core.db.model.test_diff.index.TestDiffBySuiteEntity;

import static org.assertj.core.api.Assertions.assertThat;
import static ru.yandex.ci.storage.core.Common.ResultType.RT_TEST_MEDIUM;
import static ru.yandex.ci.storage.core.Common.ResultType.RT_TEST_SUITE_MEDIUM;

public class SearchServiceTest extends StorageYdbTestBase {
    private static final TestEntity.Id LINUX_TEST_ID = new TestEntity.Id(1L, "linux", 2L);
    private static final TestEntity.Id WINDOWS_TEST_ID = new TestEntity.Id(1L, "windows", 2L);

    ChunkAggregateEntity.Id aggregateId = new ChunkAggregateEntity.Id(
            sampleIterationId, ChunkEntity.Id.of(Common.ChunkType.CT_SMALL_TEST, 1)
    );

    @Test
    void pagingSuites() {
        var service = new SearchService(db, 1, 1);
        createTestData();

        var filters = SuiteSearchFilters.builder()
                .status(StorageFrontApi.StatusFilter.STATUS_ALL)
                .category(StorageFrontApi.CategoryFilter.CATEGORY_ALL)
                .build();

        var result = service.searchSuites(
                aggregateId.getIterationId(), filters, true
        );

        assertThat(result.getSuites()).hasSize(1);
        assertThat(result.getSuites().get(0).getId().getCombinedTestId().getSuiteId()).isEqualTo(1L);
        assertThat(result.getPageCursor().getBackward()).isNull();
        assertThat(result.getPageCursor().getForward()).isNotNull();
        assertThat(result.getPageCursor().getForward().getSuiteId()).isEqualTo(2L);
        assertThat(result.getPageCursor().getForward().getPath()).isEqualTo("b");

        result = service.searchSuites(
                aggregateId.getIterationId(),
                filters.toBuilder()
                        .pageCursor(new SuitePageCursor(result.getPageCursor().getForward(), null))
                        .build(),
                true
        );

        assertThat(result.getSuites()).hasSize(1);
        assertThat(result.getSuites().get(0).getId().getCombinedTestId().getSuiteId()).isEqualTo(2L);
        assertThat(result.getPageCursor().getForward()).isNotNull();
        assertThat(result.getPageCursor().getBackward()).isNotNull();
        assertThat(result.getPageCursor().getBackward().getSuiteId()).isEqualTo(2L);
        assertThat(result.getPageCursor().getForward().getSuiteId()).isEqualTo(3L);
        assertThat(result.getPageCursor().getBackward().getPath()).isEqualTo("b");
        assertThat(result.getPageCursor().getForward().getPath()).isEqualTo("c");

        result = service.searchSuites(
                aggregateId.getIterationId(),
                filters.toBuilder()
                        .pageCursor(new SuitePageCursor(result.getPageCursor().getForward(), null))
                        .build(),
                true
        );

        assertThat(result.getSuites()).hasSize(1);
        assertThat(result.getSuites().get(0).getId().getCombinedTestId().getSuiteId()).isEqualTo(3L);
        assertThat(result.getPageCursor().getBackward()).isNotNull();
        assertThat(result.getPageCursor().getBackward().getSuiteId()).isEqualTo(3L);
        assertThat(result.getPageCursor().getForward()).isNull();
        assertThat(result.getPageCursor().getBackward().getPath()).isEqualTo("c");

        result = service.searchSuites(
                aggregateId.getIterationId(),
                filters.toBuilder()
                        .pageCursor(new SuitePageCursor(null, result.getPageCursor().getBackward()))
                        .build(),
                true
        );

        assertThat(result.getSuites()).hasSize(1);
        assertThat(result.getSuites().get(0).getId().getCombinedTestId().getSuiteId()).isEqualTo(2L);
        assertThat(result.getPageCursor().getForward()).isNotNull();
        assertThat(result.getPageCursor().getBackward()).isNotNull();
        assertThat(result.getPageCursor().getBackward().getSuiteId()).isEqualTo(2L);
        assertThat(result.getPageCursor().getForward().getSuiteId()).isEqualTo(3L);
        assertThat(result.getPageCursor().getBackward().getPath()).isEqualTo("b");
        assertThat(result.getPageCursor().getForward().getPath()).isEqualTo("c");

        result = service.searchSuites(
                aggregateId.getIterationId(),
                filters.toBuilder()
                        .pageCursor(new SuitePageCursor(null, result.getPageCursor().getBackward()))
                        .build(),
                true
        );

        assertThat(result.getSuites()).hasSize(1);
        assertThat(result.getSuites().get(0).getId().getCombinedTestId().getSuiteId()).isEqualTo(1L);
        assertThat(result.getPageCursor().getBackward()).isNull();
        assertThat(result.getPageCursor().getForward()).isNotNull();
        assertThat(result.getPageCursor().getForward().getSuiteId()).isEqualTo(2L);
        assertThat(result.getPageCursor().getForward().getPath()).isEqualTo("b");
    }

    @Test
    void pagingDiffs() {
        var service = new SearchService(db, 1, 1);
        createTestData();

        var result = service.searchDiffs(
                aggregateId.getIterationId(),
                DiffSearchFilters.builder()
                        .resultTypes(Set.of(RT_TEST_MEDIUM))
                        .build()
        );

        assertThat(result.getDiffs()).hasSize(1);
        assertThat(result.getDiffs().get(0).getId().getCombinedTestId().getToolchain()).isEqualTo("linux");
        assertThat(result.getPageCursor().getBackwardPageId()).isEqualTo(0);
        assertThat(result.getPageCursor().getForwardPageId()).isEqualTo(2);

        var filters = DiffSearchFilters.builder()
                .resultTypes(Set.of(RT_TEST_MEDIUM))
                .page(result.getPageCursor().getForwardPageId())
                .build();

        result = service.searchDiffs(
                aggregateId.getIterationId(), filters
        );

        assertThat(result.getDiffs()).hasSize(1);
        assertThat(result.getDiffs().get(0).getId().getCombinedTestId().getToolchain()).isEqualTo("toolchain");
        assertThat(result.getPageCursor().getBackwardPageId()).isEqualTo(1);
        assertThat(result.getPageCursor().getForwardPageId()).isEqualTo(3);

        result = service.searchDiffs(
                aggregateId.getIterationId(),
                filters.toBuilder()
                        .page(result.getPageCursor().getForwardPageId())
                        .build()
        );

        assertThat(result.getDiffs()).hasSize(1);
        assertThat(result.getDiffs().get(0).getId().getCombinedTestId().getToolchain()).isEqualTo("windows");
        assertThat(result.getPageCursor().getBackwardPageId()).isEqualTo(2);
        assertThat(result.getPageCursor().getForwardPageId()).isEqualTo(0);

        result = service.searchDiffs(
                aggregateId.getIterationId(),
                filters.toBuilder()
                        .page(result.getPageCursor().getBackwardPageId())
                        .build()
        );

        assertThat(result.getDiffs()).hasSize(1);
        assertThat(result.getDiffs().get(0).getId().getCombinedTestId().getToolchain()).isEqualTo("toolchain");
        assertThat(result.getPageCursor().getBackwardPageId()).isEqualTo(1);
        assertThat(result.getPageCursor().getForwardPageId()).isEqualTo(3);

        result = service.searchDiffs(
                aggregateId.getIterationId(),
                filters.toBuilder()
                        .page(result.getPageCursor().getBackwardPageId())
                        .build()
        );

        assertThat(result.getDiffs()).hasSize(1);
        assertThat(result.getDiffs().get(0).getId().getCombinedTestId().getToolchain()).isEqualTo("linux");
        assertThat(result.getPageCursor().getBackwardPageId()).isEqualTo(0);
        assertThat(result.getPageCursor().getForwardPageId()).isEqualTo(2);
    }

    @Test
    public void searchSuites() {
        var service = new SearchService(db, 20, 20);

        createTestData();

        var result = service.searchSuites(
                aggregateId.getIterationId(),
                SuiteSearchFilters.builder()
                        .resultTypes(Set.of(RT_TEST_SUITE_MEDIUM))
                        .status(StorageFrontApi.StatusFilter.STATUS_ALL)
                        .category(StorageFrontApi.CategoryFilter.CATEGORY_ALL)
                        .build(),
                true
        );

        assertThat(result.getSuites()).hasSize(1);
        assertThat(result.getSuites().get(0).getId().getCombinedTestId().getSuiteId()).isEqualTo(1L);

        result = service.searchSuites(
                aggregateId.getIterationId(),
                SuiteSearchFilters.builder()
                        .status(StorageFrontApi.StatusFilter.STATUS_ALL)
                        .category(StorageFrontApi.CategoryFilter.CATEGORY_ALL)
                        .build(),
                true
        );

        assertThat(result.getSuites()).hasSize(3);

        result = service.searchSuites(
                aggregateId.getIterationId(),
                SuiteSearchFilters.builder()
                        .status(StorageFrontApi.StatusFilter.STATUS_FAILED)
                        .category(StorageFrontApi.CategoryFilter.CATEGORY_ALL)
                        .build(),
                true
        );

        assertThat(result.getSuites()).hasSize(2);
        assertThat(
                result.getSuites().stream()
                        .map(x -> x.getId().getCombinedTestId().getSuiteId())
                        .collect(Collectors.toSet())
        ).contains(1L, 2L);

        result = service.searchSuites(
                aggregateId.getIterationId(),
                SuiteSearchFilters.builder()
                        .status(StorageFrontApi.StatusFilter.STATUS_FAILED)
                        .category(StorageFrontApi.CategoryFilter.CATEGORY_CHANGED)
                        .build(),
                true
        );

        assertThat(result.getSuites()).hasSize(1);
        assertThat(result.getSuites().get(0).getId().getCombinedTestId().getSuiteId()).isEqualTo(2L);

        result = service.searchSuites(
                aggregateId.getIterationId(),
                SuiteSearchFilters.builder()
                        .status(StorageFrontApi.StatusFilter.STATUS_FAILED)
                        .category(StorageFrontApi.CategoryFilter.CATEGORY_CHANGED)
                        .specialCases(StorageFrontApi.SpecialCasesFilter.SPECIAL_CASE_BROKEN_BY_DEPS)
                        .build(),
                true
        );

        assertThat(result.getSuites()).hasSize(1);
        assertThat(result.getSuites().get(0).getId().getCombinedTestId().getSuiteId()).isEqualTo(3L);

        result = service.searchSuites(
                aggregateId.getIterationId(),
                SuiteSearchFilters.builder()
                        .status(StorageFrontApi.StatusFilter.STATUS_ALL)
                        .category(StorageFrontApi.CategoryFilter.CATEGORY_ALL)
                        .specialCases(StorageFrontApi.SpecialCasesFilter.SPECIAL_CASE_TIMEOUT)
                        .build(),
                true
        );

        assertThat(result.getSuites()).hasSize(1);
        assertThat(result.getSuites().get(0).getId().getSuiteId()).isEqualTo(3L);
    }

    @Test
    public void searchSuitesByFullText() {
        var service = new SearchService(db, 20, 20);

        createTestData();

        var result = service.searchSuites(
                aggregateId.getIterationId(),
                SuiteSearchFilters.builder()
                        .status(StorageFrontApi.StatusFilter.STATUS_ALL)
                        .category(StorageFrontApi.CategoryFilter.CATEGORY_ALL)
                        .testName("ru.yandex.ci.storage.tests.StorageFrontApiTests")
                        .build(),
                true
        );

        assertThat(result.getSuites()).hasSize(1);

        var filters = SuiteSearchFilters.builder()
                .status(StorageFrontApi.StatusFilter.STATUS_ALL)
                .category(StorageFrontApi.CategoryFilter.CATEGORY_ALL)
                .testName("ru.yandex.ci.storage.tests.StorageFrontApiTests")
                .subtestName("cancelsCheck()")
                .build();
        result = service.searchSuites(
                aggregateId.getIterationId(),
                filters,
                true
        );

        assertThat(result.getSuites()).hasSize(1);

        result = service.searchSuites(
                aggregateId.getIterationId(),
                SuiteSearchFilters.builder()
                        .status(StorageFrontApi.StatusFilter.STATUS_ALL)
                        .category(StorageFrontApi.CategoryFilter.CATEGORY_ALL)
                        .testName("ru.yandex.ci.storage.tests.StorageFrontApiTests")
                        .subtestName("fake")
                        .build(),
                true
        );

        assertThat(result.getSuites()).hasSize(0);
    }

    @Test
    public void searchSuitesInMetaIteration() {
        var service = new SearchService(db, 20, 20);

        createTestData();

        var result = service.searchSuites(
                aggregateId.getIterationId().toMetaId(),
                SuiteSearchFilters.builder()
                        .resultTypes(Set.of(RT_TEST_SUITE_MEDIUM))
                        .status(StorageFrontApi.StatusFilter.STATUS_ALL)
                        .category(StorageFrontApi.CategoryFilter.CATEGORY_ALL)
                        .build(),
                true
        );

        assertThat(
                result.getSuites().stream()
                        .map(x -> x.getId().getCombinedTestId().getSuiteId())
                        .collect(Collectors.toList())
        )
                .containsOnly(1L, 4L);
    }

    @Test
    public void searchSuitesWithAdded() {
        var service = new SearchService(db, 20, 20);

        createTestData();

        var results = service.searchSuites(
                aggregateId.getIterationId(),
                SuiteSearchFilters.builder()
                        .status(StorageFrontApi.StatusFilter.STATUS_PASSED)
                        .category(StorageFrontApi.CategoryFilter.CATEGORY_CHANGED)
                        .specialCases(StorageFrontApi.SpecialCasesFilter.SPECIAL_CASE_ADDED)
                        .build(),
                true
        );

        assertThat(results.getSuites()).hasSize(1);
        assertThat(results.getSuites().get(0).getId().getCombinedTestId().getSuiteId()).isEqualTo(1L);
    }


    @Test
    void listsSuite() {
        var service = new SearchService(db, 20, 20);
        createTestData();

        var result = this.db.currentOrReadOnly(() -> service.listSuite(
                new TestDiffEntity.Id(
                        aggregateId.getIterationId(),
                        RT_TEST_SUITE_MEDIUM,
                        "a",
                        TestEntity.Id.of(1L, "@all")
                ),
                SuiteSearchFilters.builder()
                        .status(StorageFrontApi.StatusFilter.STATUS_ALL)
                        .category(StorageFrontApi.CategoryFilter.CATEGORY_CHANGED)
                        .resultTypes(Set.of(RT_TEST_SUITE_MEDIUM))
                        .build(),
                0,
                true
        ));
        var resultWithoutIndex = this.db.currentOrReadOnly(() -> service.listSuite(
                new TestDiffEntity.Id(
                        aggregateId.getIterationId(),
                        RT_TEST_SUITE_MEDIUM,
                        "a",
                        TestEntity.Id.of(1L, "@all")
                ),
                SuiteSearchFilters.builder()
                        .status(StorageFrontApi.StatusFilter.STATUS_ALL)
                        .category(StorageFrontApi.CategoryFilter.CATEGORY_CHANGED)
                        .resultTypes(Set.of(RT_TEST_SUITE_MEDIUM))
                        .build(),
                0,
                false
        ));

        assertThat(result).isEqualTo(resultWithoutIndex);

        assertThat(result.getDiffs()).hasSize(1);

        result = service.listSuite(
                new TestDiffEntity.Id(
                        aggregateId.getIterationId(),
                        RT_TEST_SUITE_MEDIUM,
                        "a",
                        TestEntity.Id.of(1L, "toolchain")
                ),
                SuiteSearchFilters.builder()
                        .status(StorageFrontApi.StatusFilter.STATUS_ALL)
                        .category(StorageFrontApi.CategoryFilter.CATEGORY_ALL)
                        .resultTypes(Set.of(RT_TEST_SUITE_MEDIUM))
                        .toolchain("toolchain")
                        .build(),
                1,
                true
        );

        assertThat(result.getDiffs()).hasSize(1);

        var all = SuiteSearchFilters.builder()
                .status(StorageFrontApi.StatusFilter.STATUS_ALL)
                .category(StorageFrontApi.CategoryFilter.CATEGORY_ALL)
                .resultTypes(Set.of(RT_TEST_SUITE_MEDIUM))
                .build();
        var onePageService = new SearchService(db, 1, 1);
        result = this.db.currentOrReadOnly(() -> onePageService.listSuite(
                new TestDiffEntity.Id(
                        aggregateId.getIterationId(),
                        RT_TEST_SUITE_MEDIUM,
                        "a",
                        TestEntity.Id.of(1L, "@all")
                ),
                all,
                1,
                true
        ));

        assertThat(result.getDiffs()).hasSize(1);
        assertThat(result.getDiffs().get(0).getId().getTestId()).isEqualTo(4L);

        result = this.db.currentOrReadOnly(() -> onePageService.listSuite(
                new TestDiffEntity.Id(
                        aggregateId.getIterationId(),
                        RT_TEST_SUITE_MEDIUM,
                        "a",
                        TestEntity.Id.of(1L, "@all")
                ),
                all,
                2,
                true
        ));

        assertThat(result.getDiffs()).hasSize(1);
        assertThat(result.getDiffs().get(0).getId().getTestId()).isEqualTo(2L);
    }

    @Test
    void listsDiff() {
        var service = new SearchService(db, 20, 20);
        createTestData();

        var testA = LINUX_TEST_ID.toAllToolchainsId();
        var linux = LINUX_TEST_ID;
        var windows = WINDOWS_TEST_ID;

        var diffId = new TestDiffEntity.Id(
                aggregateId.getIterationId(),
                RT_TEST_MEDIUM,
                "a",
                testA
        );
        var result = service.listDiff(diffId);

        assertThat(result.getDiff()).isNotNull();
        assertThat(result.getDiff().getId()).isEqualTo(diffId);
        assertThat(result.getChildren()).hasSize(2);
        assertThat(
                result.getChildren().stream()
                        .map(x -> x.getDiff().getId().getCombinedTestId())
                        .collect(Collectors.toSet())
        ).contains(linux, windows);

        result = service.listDiff(
                new TestDiffEntity.Id(
                        aggregateId.getIterationId(),
                        RT_TEST_MEDIUM,
                        "a",
                        testA
                )
        );

        // todo should we show all statuses?
        assertThat(result.getChildren()).hasSize(2);
        //assertThat(result.getDiffs()).hasSize(1);
        assertThat(
                result.getChildren().stream()
                        .map(x -> x.getDiff().getId().getCombinedTestId())
                        .collect(Collectors.toSet())
        ).contains(linux);

        result = service.listDiff(
                new TestDiffEntity.Id(
                        aggregateId.getIterationId(),
                        RT_TEST_MEDIUM,
                        "a",
                        testA
                )
        );

        // todo should we show all statuses?
        assertThat(result.getChildren()).hasSize(2);
        //assertThat(result.getDiffs()).hasSize(1);
        assertThat(
                result.getChildren().stream()
                        .map(x -> x.getDiff().getId().getCombinedTestId())
                        .collect(Collectors.toSet())
        ).contains(windows);
    }

    @Test
    void getsTestRun() {
        var service = new SearchService(db, 20, 20);
        createTestData();

        var testRun = service.getTestRun(
                aggregateId.getIterationId(),
                LINUX_TEST_ID,
                "taskId",
                1,
                1
        );

        assertThat(testRun).isPresent();
    }

    @Test
    void searchDiffs() {
        var service = new SearchService(db, 20, 20);
        createTestData();

        var result = service.searchDiffs(
                aggregateId.getIterationId(),
                DiffSearchFilters.builder()
                        .path("a")
                        .build()
        );

        assertThat(result.getDiffs()).hasSize(4);

        result = service.searchDiffs(
                aggregateId.getIterationId(),
                DiffSearchFilters.builder()
                        .diffTypes(Set.of(Common.TestDiffType.TDT_FAILED))
                        .build()
        );

        assertThat(result.getDiffs()).hasSize(1);

        result = service.searchDiffs(
                aggregateId.getIterationId(),
                DiffSearchFilters.builder()
                        .tags(Set.of("tag_b"))
                        .build()
        );

        assertThat(result.getDiffs()).hasSize(1);
        assertThat(result.getDiffs().get(0).getId().getCombinedTestId()).isEqualTo(LINUX_TEST_ID);
    }

    @SuppressWarnings("MethodLength")
    private void createTestData() {
        var linuxDiff = TestDiffByHashEntity.builder()
                .id(createId(LINUX_TEST_ID))
                .tags(Set.of("tag_a", "tag_b"))
                .resultType(RT_TEST_MEDIUM)
                .isLast(true)
                .path("a")
                .statistics(
                        new TestDiffStatistics(
                                new TestDiffStatistics.StatisticsGroup(
                                        StageStatistics.builder()
                                                .failed(1)
                                                .build(),
                                        ExtendedStatistics.EMPTY
                                ),
                                TestDiffStatistics.StatisticsGroup.EMPTY
                        )
                )
                .build();

        var diffs = List.of(
                TestDiffByHashEntity.builder()
                        .id(createId(1L, TestEntity.ALL_TOOLCHAINS, null))
                        .path("a")
                        .isLast(true)
                        .resultType(RT_TEST_SUITE_MEDIUM)
                        .statistics(
                                new TestDiffStatistics(
                                        TestDiffStatistics.StatisticsGroup.EMPTY,
                                        new TestDiffStatistics.StatisticsGroup(
                                                StageStatistics.builder()
                                                        .failed(1)
                                                        .passed(1)
                                                        .passedAdded(1)
                                                        .build(),
                                                ExtendedStatistics.builder()
                                                        .added(
                                                                ExtendedStageStatistics.builder()
                                                                        .total(1)
                                                                        .passedAdded(1)
                                                                        .build()
                                                        )
                                                        .build()
                                        )
                                )
                        )
                        .build(),
                TestDiffByHashEntity.builder()
                        .id(createId(1L, "toolchain", null))
                        .path("a")
                        .isLast(true)
                        .resultType(RT_TEST_SUITE_MEDIUM)
                        .statistics(
                                new TestDiffStatistics(
                                        TestDiffStatistics.StatisticsGroup.EMPTY,
                                        new TestDiffStatistics.StatisticsGroup(
                                                StageStatistics.builder()
                                                        .failed(1)
                                                        .passed(1)
                                                        .passedAdded(1)
                                                        .build(),
                                                ExtendedStatistics.builder()
                                                        .added(
                                                                ExtendedStageStatistics.builder()
                                                                        .total(1)
                                                                        .passedAdded(1)
                                                                        .build()
                                                        )
                                                        .build()
                                        )
                                )
                        )
                        .build(),
                TestDiffByHashEntity.builder()
                        .id(createId(1L, TestEntity.ALL_TOOLCHAINS, 2L))
                        .path("a")
                        .name("java")
                        .isLast(true)
                        .resultType(RT_TEST_MEDIUM)
                        .statistics(
                                new TestDiffStatistics(
                                        TestDiffStatistics.StatisticsGroup.EMPTY,
                                        new TestDiffStatistics.StatisticsGroup(
                                                StageStatistics.builder()
                                                        .failed(1)
                                                        .passed(1)
                                                        .passedAdded(1)
                                                        .build(),
                                                ExtendedStatistics.EMPTY
                                        )
                                )
                        )
                        .build(),
                linuxDiff,
                TestDiffByHashEntity.builder()
                        .id(createId(WINDOWS_TEST_ID))
                        .path("a")
                        .isLast(true)
                        .resultType(RT_TEST_MEDIUM)
                        .statistics(
                                new TestDiffStatistics(
                                        new TestDiffStatistics.StatisticsGroup(
                                                StageStatistics.builder()
                                                        .passed(1)
                                                        .build(),
                                                ExtendedStatistics.EMPTY
                                        ),
                                        TestDiffStatistics.StatisticsGroup.EMPTY
                                )
                        )
                        .build(),
                TestDiffByHashEntity.builder()
                        .id(createId(1L, TestEntity.ALL_TOOLCHAINS, 4L))
                        .path("a")
                        .isLast(true)
                        .resultType(RT_TEST_MEDIUM)
                        .statistics(
                                new TestDiffStatistics(
                                        TestDiffStatistics.StatisticsGroup.EMPTY,
                                        new TestDiffStatistics.StatisticsGroup(
                                                StageStatistics.builder()
                                                        .failed(1)
                                                        .build(),
                                                ExtendedStatistics.EMPTY
                                        )
                                )
                        )
                        .build(),
                TestDiffByHashEntity.builder()
                        .id(createId(1L, "toolchain", 5L))
                        .path("a")
                        .diffType(Common.TestDiffType.TDT_FAILED)
                        .isLast(true)
                        .resultType(RT_TEST_MEDIUM)
                        .statistics(
                                new TestDiffStatistics(
                                        TestDiffStatistics.StatisticsGroup.EMPTY,
                                        new TestDiffStatistics.StatisticsGroup(
                                                StageStatistics.builder()
                                                        .failed(1)
                                                        .build(),
                                                ExtendedStatistics.EMPTY
                                        )
                                )
                        )
                        .build(),
                TestDiffByHashEntity.builder()
                        .id(createId(2L, TestEntity.ALL_TOOLCHAINS, null))
                        .path("b")
                        .isLast(true)
                        .resultType(Common.ResultType.RT_BUILD)
                        .statistics(
                                new TestDiffStatistics(
                                        new TestDiffStatistics.StatisticsGroup(
                                                StageStatistics.builder()
                                                        .failed(1)
                                                        .failedAdded(1)
                                                        .build(),
                                                ExtendedStatistics.EMPTY
                                        ),
                                        TestDiffStatistics.StatisticsGroup.EMPTY
                                )
                        )
                        .build(),
                TestDiffByHashEntity.builder()
                        .id(createId(3L, TestEntity.ALL_TOOLCHAINS, null))
                        .path("c")
                        .isLast(true)
                        .resultType(Common.ResultType.RT_BUILD)
                        .statistics(
                                new TestDiffStatistics(
                                        new TestDiffStatistics.StatisticsGroup(
                                                StageStatistics.builder()
                                                        .failedByDeps(1)
                                                        .failedByDepsAdded(1)
                                                        .build(),
                                                ExtendedStatistics.EMPTY
                                        ),
                                        new TestDiffStatistics.StatisticsGroup(
                                                StageStatistics.EMPTY,
                                                ExtendedStatistics.builder()
                                                        .timeout(
                                                                ExtendedStageStatistics.builder()
                                                                        .total(1)
                                                                        .build()
                                                        )
                                                        .build()
                                        )
                                )
                        )
                        .build(),
                TestDiffByHashEntity.builder()
                        .id(
                                TestDiffByHashEntity.Id.of(
                                        aggregateId.toIterationMetaId(),
                                        TestEntity.Id.of(4L, TestEntity.ALL_TOOLCHAINS)
                                )
                        )
                        .path("d")
                        .isLast(true)
                        .resultType(RT_TEST_SUITE_MEDIUM)
                        .statistics(
                                new TestDiffStatistics(
                                        new TestDiffStatistics.StatisticsGroup(
                                                StageStatistics.builder()
                                                        .passed(1)
                                                        .build(),
                                                ExtendedStatistics.EMPTY
                                        ),
                                        TestDiffStatistics.StatisticsGroup.EMPTY
                                )
                        )
                        .build()
        );

        var iterationStatistics = Map.of(
                "toolchain", IterationStatistics.ToolchainStatistics.EMPTY,
                "linux", IterationStatistics.ToolchainStatistics.EMPTY,
                "windows", IterationStatistics.ToolchainStatistics.EMPTY,
                TestEntity.ALL_TOOLCHAINS, IterationStatistics.ToolchainStatistics.EMPTY
        );

        this.db.currentOrTx(() -> {
            this.db.checkIterations().save(
                    CheckIterationEntity.builder()
                            .id(sampleIterationId)
                            .statistics(new IterationStatistics(null, Metrics.EMPTY, iterationStatistics))
                            .build()
            );
        });

        this.db.currentOrTx(() -> {
            this.db.testDiffsByHash().save(diffs);

            this.db.testDiffs().save(
                    diffs.stream().map(TestDiffEntity::new).collect(Collectors.toList())
            );

            this.db.testDiffsBySuite().save(
                    diffs.stream().map(TestDiffBySuiteEntity::new).collect(Collectors.toList())
            );

            this.db.importantTestDiffs().save(
                    diffs.stream()
                            .filter(TestDiffByHashEntity::isImportant)
                            .map(TestDiffImportantEntity::new)
                            .collect(Collectors.toList())
            );
        });

        this.db.currentOrTx(() -> {
            var result = TestResult.builder()
                    .id(
                            new TestResultEntity.Id(
                                    linuxDiff.getId().getAggregateId().getIterationId(),
                                    linuxDiff.getId().getTestId(),
                                    "taskId", 1, 1
                            )
                    )
                    .path("a")
                    .name("ru.yandex.ci.storage.tests.StorageFrontApiTests")
                    .subtestName("cancelsCheck()")
                    .resultType(linuxDiff.getResultType())
                    .chunkId(linuxDiff.getId().getAggregateId().getChunkId())
                    .build();

            this.db.checkTextSearch().save(CheckTextSearchEntity.index(result));
            this.db.testResults().save(new TestResultEntity(result));
        });
    }

    private TestDiffByHashEntity.Id createId(long suiteId, String toolchain, @Nullable Long testId) {
        return TestDiffByHashEntity.Id.of(
                aggregateId, new TestEntity.Id(suiteId, toolchain, testId == null ? suiteId : testId)
        );
    }

    private TestDiffByHashEntity.Id createId(TestEntity.Id testId) {
        return TestDiffByHashEntity.Id.of(
                aggregateId, testId
        );
    }
}
