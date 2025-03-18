package ru.yandex.ci.storage.shard.task;

import java.util.Arrays;
import java.util.Map;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Collectors;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.storage.core.CheckIteration.IterationType;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.Common.ResultType;
import ru.yandex.ci.storage.core.Common.TestDiffType;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.ExtendedStageStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.ExtendedStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.MainStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.StageStatistics;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateStatistics;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffByHashEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffStatistics;
import ru.yandex.ci.storage.shard.ShardStatistics;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.mock;

public class TaskDiffProcessorTest extends CommonTestBase {
    ChunkAggregateEntity.Id iterationId = new ChunkAggregateEntity.Id(
            CheckIterationEntity.Id.of(CheckEntity.Id.of("100"), IterationType.FULL, 1),
            ChunkEntity.Id.of(Common.ChunkType.CT_SMALL_TEST, 1)
    );

    TestDiffByHashEntity.Id diffId = TestDiffByHashEntity.Id.of(iterationId, TestEntity.Id.of(1L, "b"));
    private TestDiffStatistics.Mutable allToolchainsStatistics;
    private TestDiffStatistics.Mutable suiteStatistics;
    private TestDiffStatistics.Mutable suiteAllToolchainsStatistics;
    private TaskDiffProcessor taskDiffProcessor;

    @BeforeEach
    public void beforeEach() {
        cleanUpStatistics();

        var statistics = mock(ShardStatistics.class);
        this.taskDiffProcessor = new TaskDiffProcessor(statistics);
    }

    @Test
    public void countsNew() {
        var aggregate = ChunkAggregateEntity.create(iterationId).toMutable();
        var diff = TestDiffByHashEntity.builder()
                .id(diffId)
                .resultType(ResultType.RT_TEST_MEDIUM)
                .statistics(
                        new TestDiffStatistics(
                                taskDiffProcessor.calculateStageDiffStatistics(
                                        TestDiffType.TDT_PASSED_NEW, false, false, false
                                ),
                                TestDiffStatistics.StatisticsGroup.EMPTY
                        )
                )
                .build();

        taskDiffProcessor.applyTestDiff(
                diff,
                allToolchainsStatistics,
                suiteStatistics,
                suiteAllToolchainsStatistics,
                aggregate
        );

        var statistics = aggregate.toImmutable().getStatistics();

        var expectedMain = MainStatistics.builder()
                .mediumTests(
                        StageStatistics.builder()
                                .total(1)
                                .passed(1)
                                .build()
                )
                .total(
                        StageStatistics.builder()
                                .total(1)
                                .passed(1)
                                .build()
                )
                .build();

        var expectedExtended = ExtendedStatistics.builder()
                .added(ExtendedStageStatistics.builder().total(1).passedAdded(1).build())
                .build();

        assertThat(statistics.getAll().getMain()).isEqualTo(expectedMain);
        assertThat(statistics.getAll().getExtended()).isEqualTo(expectedExtended);
    }

    @Test
    public void countsFailed() {
        var aggregate = ChunkAggregateEntity.create(iterationId).toMutable();
        TestDiffByHashEntity diff = TestDiffByHashEntity.builder()
                .id(diffId)
                .resultType(ResultType.RT_TEST_MEDIUM)
                .statistics(
                        new TestDiffStatistics(
                                taskDiffProcessor.calculateStageDiffStatistics(
                                        TestDiffType.TDT_FAILED, false, false, false
                                ),
                                TestDiffStatistics.StatisticsGroup.EMPTY
                        )
                )
                .build();

        taskDiffProcessor.applyTestDiff(
                diff,
                allToolchainsStatistics,
                suiteStatistics,
                suiteAllToolchainsStatistics,
                aggregate
        );

        var statistics = aggregate.toImmutable().getStatistics();

        var expectedMain = MainStatistics.builder()
                .mediumTests(
                        StageStatistics.builder()
                                .total(1)
                                .failed(1)
                                .build()
                )
                .total(
                        StageStatistics.builder()
                                .total(1)
                                .failed(1)
                                .build()
                )
                .build();

        var expectedExtended = ExtendedStatistics.EMPTY;

        assertThat(statistics.getAll().getMain()).isEqualTo(expectedMain);
        assertThat(statistics.getAll().getExtended()).isEqualTo(expectedExtended);
    }

    @Test
    public void countsDeleted() {
        var aggregate = ChunkAggregateEntity.create(iterationId).toMutable();
        var diff = TestDiffByHashEntity.builder()
                .id(diffId)
                .resultType(ResultType.RT_TEST_MEDIUM)
                .statistics(
                        new TestDiffStatistics(
                                taskDiffProcessor.calculateStageDiffStatistics(
                                        TestDiffType.TDT_DELETED, false, false, false
                                ),
                                TestDiffStatistics.StatisticsGroup.EMPTY
                        )
                )
                .build();

        taskDiffProcessor.applyTestDiff(
                diff,
                allToolchainsStatistics,
                suiteStatistics,
                suiteAllToolchainsStatistics,
                aggregate
        );

        var statistics = aggregate.toImmutable().getStatistics();

        var expectedMain = MainStatistics.builder()
                .mediumTests(
                        StageStatistics.builder()
                                .total(1)
                                .build()
                )
                .total(
                        StageStatistics.builder()
                                .total(1)
                                .build()
                )
                .build();

        var expectedExtended = ExtendedStatistics.builder()
                .deleted(ExtendedStageStatistics.builder().total(1).build())
                .build();

        assertThat(statistics.getAll().getMain()).isEqualTo(expectedMain);
        assertThat(statistics.getAll().getExtended()).isEqualTo(expectedExtended);
    }

    @Test
    public void countsSkipped() {
        var aggregate = ChunkAggregateEntity.create(iterationId).toMutable();
        var diff = TestDiffByHashEntity.builder()
                .id(diffId)
                .resultType(ResultType.RT_TEST_LARGE)
                .statistics(
                        new TestDiffStatistics(
                                taskDiffProcessor.calculateStageDiffStatistics(
                                        TestDiffType.TDT_SKIPPED, false, false, false
                                ),
                                TestDiffStatistics.StatisticsGroup.EMPTY
                        )
                )
                .build();

        taskDiffProcessor.applyTestDiff(
                diff,
                allToolchainsStatistics,
                suiteStatistics,
                suiteAllToolchainsStatistics,
                aggregate
        );

        var statistics = aggregate.toImmutable().getStatistics();

        var expectedMain = MainStatistics.builder()
                .largeTests(
                        StageStatistics.builder()
                                .total(1)
                                .skipped(1)
                                .build()
                )
                .total(
                        StageStatistics.builder()
                                .total(1)
                                .skipped(1)
                                .build()
                )
                .build();

        var expectedExtended = ExtendedStatistics.EMPTY;

        assertThat(statistics.getAll().getMain()).isEqualTo(expectedMain);
        assertThat(statistics.getAll().getExtended()).isEqualTo(expectedExtended);
    }

    @Test
    public void countsFlaky() {
        var aggregate = ChunkAggregateEntity.create(iterationId).toMutable();
        TestDiffByHashEntity diff = TestDiffByHashEntity.builder()
                .id(diffId)
                .resultType(ResultType.RT_TEST_MEDIUM)
                .statistics(
                        new TestDiffStatistics(
                                taskDiffProcessor.calculateStageDiffStatistics(
                                        TestDiffType.TDT_FLAKY_FAILED, false, false, false
                                ),
                                TestDiffStatistics.StatisticsGroup.EMPTY
                        )
                )
                .build();

        taskDiffProcessor.applyTestDiff(
                diff,
                allToolchainsStatistics,
                suiteStatistics,
                suiteAllToolchainsStatistics,
                aggregate
        );

        var statistics = aggregate.toImmutable().getStatistics();

        var expectedMain = MainStatistics.builder()
                .mediumTests(
                        StageStatistics.builder()
                                .total(1)
                                .build()
                )
                .total(
                        StageStatistics.builder()
                                .total(1)
                                .build()
                )
                .build();

        var expectedExtended = ExtendedStatistics.builder()
                .flaky(ExtendedStageStatistics.builder().total(1).build())
                .build();

        assertThat(statistics.getAll().getMain()).isEqualTo(expectedMain);
        assertThat(statistics.getAll().getExtended()).isEqualTo(expectedExtended);
    }

    @Test
    public void calculateDiffStatistics_shouldChangeFailedInStrongMode_whenStrongModeIsFalse() {
        boolean isStrongMode = false;
        // Run TaskDiffProcessor.calculateDiffStatistics for each TestDiffType
        Map<TestDiffType, MainStatistics> result = calculateDiffStatisticsForAllDiffTypes(isStrongMode);
        assertThat(
                result.entrySet().stream()
                        .filter(entry -> entry.getValue().getTotal() != null)
                        .filter(entry -> entry.getValue().getTotal().getFailedInStrongMode() != 0)
                        .map(Map.Entry::getKey)
                        .collect(Collectors.toSet())
        ).isEqualTo(Set.of());
    }

    @Test
    public void calculateDiffStatistics_shouldChangeFailedInStrongMode_whenStrongModeIsTrue() {
        boolean isStrongMode = true;
        // Run TaskDiffProcessor.calculateDiffStatistics for each TestDiffType
        Map<TestDiffType, MainStatistics> result = calculateDiffStatisticsForAllDiffTypes(isStrongMode);
        Set<TestDiffType> shouldChangeFailedAddedInStrongMode = Arrays.stream(TestDiffType.values())
                .filter(testDiffType -> switch (testDiffType) {
                    case TDT_TIMEOUT_BROKEN, TDT_TIMEOUT_NEW, TDT_FLAKY_BROKEN, TDT_FLAKY_NEW, TDT_EXTERNAL_BROKEN,
                            TDT_EXTERNAL_NEW, TDT_INTERNAL_BROKEN, TDT_INTERNAL_NEW,
                            TDT_TIMEOUT_FAILED, TDT_FLAKY_FAILED, TDT_EXTERNAL_FAILED, TDT_INTERNAL_FAILED,
                            TDT_FAILED, TDT_FAILED_NEW, TDT_FAILED_BROKEN,
                            TDT_FAILED_BY_DEPS_NEW, TDT_FAILED_BY_DEPS_BROKEN -> true;
                    default -> false;
                })
                .collect(Collectors.toSet());
        assertThat(
                result.entrySet().stream()
                        .filter(entry -> entry.getValue().getTotal() != null)
                        .filter(entry -> entry.getValue().getTotal().getFailedInStrongMode() != 0)
                        .map(Map.Entry::getKey)
                        .collect(Collectors.toSet())
        ).isEqualTo(shouldChangeFailedAddedInStrongMode);
    }

    @Test
    public void calculateDiffStatistics_shouldChangeFailedAdded_whenStrongModeIsFalse() {
        boolean isStrongMode = false;
        // Run TaskDiffProcessor.calculateDiffStatistics for each TestDiffType
        Map<TestDiffType, MainStatistics> result = calculateDiffStatisticsForAllDiffTypes(isStrongMode);
        Set<TestDiffType> shouldChangeFailedAdded = Arrays.stream(TestDiffType.values())
                .filter(testDiffType -> switch (testDiffType) {
                    case TDT_FAILED_BROKEN, TDT_FAILED_NEW -> true;
                    default -> false;
                })
                .collect(Collectors.toSet());
        assertThat(
                result.entrySet().stream()
                        .filter(entry -> entry.getValue().getTotal() != null)
                        .filter(entry -> entry.getValue().getTotal().getFailedAdded() != 0)
                        .map(Map.Entry::getKey)
                        .collect(Collectors.toSet())
        ).isEqualTo(shouldChangeFailedAdded);
    }

    @Test
    public void calculateDiffStatistics_shouldChangeFailedAdded_whenStrongModeIsTrue() {
        boolean isStrongMode = true;
        // Run TaskDiffProcessor.calculateDiffStatistics for each TestDiffType
        Map<TestDiffType, MainStatistics> result = calculateDiffStatisticsForAllDiffTypes(isStrongMode);
        Set<TestDiffType> shouldChangeFailedAdded = Arrays.stream(TestDiffType.values())
                .filter(testDiffType -> switch (testDiffType) {
                    case TDT_TIMEOUT_BROKEN, TDT_TIMEOUT_NEW, TDT_FLAKY_BROKEN, TDT_FLAKY_NEW, TDT_EXTERNAL_BROKEN,
                            TDT_EXTERNAL_NEW, TDT_INTERNAL_BROKEN, TDT_INTERNAL_NEW,
                            TDT_FAILED_BROKEN, TDT_FAILED_NEW -> true;
                    default -> false;
                })
                .collect(Collectors.toSet());
        assertThat(
                result.entrySet().stream()
                        .filter(entry -> entry.getValue().getTotal() != null)
                        .filter(entry -> entry.getValue().getTotal().getFailedAdded() != 0)
                        .map(Map.Entry::getKey)
                        .collect(Collectors.toSet())
        ).isEqualTo(shouldChangeFailedAdded);
    }

    @Test
    public void calculateDiffStatistics_shouldChangeFailed_whenStrongModeIsFalse() {
        boolean isStrongMode = false;
        // Run TaskDiffProcessor.calculateDiffStatistics for each TestDiffType
        Map<TestDiffType, MainStatistics> result = calculateDiffStatisticsForAllDiffTypes(isStrongMode);
        Set<TestDiffType> shouldChangeFailed = Arrays.stream(TestDiffType.values())
                .filter(testDiffType -> switch (testDiffType) {
                    case TDT_FAILED, TDT_FAILED_BROKEN, TDT_FAILED_NEW -> true;
                    default -> false;
                })
                .collect(Collectors.toSet());
        assertThat(
                result.entrySet().stream()
                        .filter(entry -> entry.getValue().getTotal() != null)
                        .filter(entry -> entry.getValue().getTotal().getFailed() != 0)
                        .map(Map.Entry::getKey)
                        .collect(Collectors.toSet())
        ).isEqualTo(shouldChangeFailed);
    }

    @Test
    public void calculateDiffStatistics_shouldChangeFailed_whenStrongModeIsTrue() {
        boolean isStrongMode = true;
        // Run TaskDiffProcessor.calculateDiffStatistics for each TestDiffType
        Map<TestDiffType, MainStatistics> result = calculateDiffStatisticsForAllDiffTypes(isStrongMode);
        Set<TestDiffType> shouldChangeFailed = Arrays.stream(TestDiffType.values())
                .filter(testDiffType -> switch (testDiffType) {
                    case TDT_FAILED, TDT_FAILED_BROKEN, TDT_FAILED_NEW,
                            TDT_TIMEOUT_BROKEN, TDT_TIMEOUT_NEW, TDT_FLAKY_BROKEN, TDT_FLAKY_NEW, TDT_EXTERNAL_BROKEN,
                            TDT_EXTERNAL_NEW, TDT_INTERNAL_BROKEN, TDT_INTERNAL_NEW,
                            TDT_TIMEOUT_FAILED, TDT_FLAKY_FAILED, TDT_EXTERNAL_FAILED, TDT_INTERNAL_FAILED -> true;
                    default -> false;
                })
                .collect(Collectors.toSet());
        assertThat(
                result.entrySet().stream()
                        .filter(entry -> entry.getValue().getTotal() != null)
                        .filter(entry -> entry.getValue().getTotal().getFailed() != 0)
                        .map(Map.Entry::getKey)
                        .collect(Collectors.toSet())
        ).isEqualTo(shouldChangeFailed);
    }

    @Test
    public void calculateDiffStatistics_shouldChangePassedAdded_whenStrongModeIsFalse() {
        boolean isStrongMode = false;
        // Run TaskDiffProcessor.calculateDiffStatistics for each TestDiffType
        Map<TestDiffType, MainStatistics> result = calculateDiffStatisticsForAllDiffTypes(isStrongMode);
        Set<TestDiffType> shouldChangePassedAdded = Arrays.stream(TestDiffType.values())
                .filter(testDiffType -> testDiffType == TestDiffType.TDT_PASSED_FIXED)
                .collect(Collectors.toSet());
        assertThat(
                result.entrySet().stream()
                        .filter(entry -> entry.getValue().getTotal() != null)
                        .filter(entry -> entry.getValue().getTotal().getPassedAdded() != 0)
                        .map(Map.Entry::getKey)
                        .collect(Collectors.toSet())
        ).isEqualTo(shouldChangePassedAdded);
    }

    @Test
    public void calculateDiffStatistics_shouldChangePassedAdded_whenStrongModeIsTrue() {
        boolean isStrongMode = true;
        // Run TaskDiffProcessor.calculateDiffStatistics for each TestDiffType
        Map<TestDiffType, MainStatistics> result = calculateDiffStatisticsForAllDiffTypes(isStrongMode);
        Set<TestDiffType> shouldChangePassedAdded = Arrays.stream(TestDiffType.values())
                .filter(testDiffType -> switch (testDiffType) {
                    case TDT_PASSED_FIXED,
                            TDT_TIMEOUT_FIXED, TDT_FLAKY_FIXED, TDT_EXTERNAL_FIXED, TDT_INTERNAL_FIXED -> true;
                    default -> false;
                })
                .collect(Collectors.toSet());
        assertThat(
                result.entrySet().stream()
                        .filter(entry -> entry.getValue().getTotal() != null)
                        .filter(entry -> entry.getValue().getTotal().getPassedAdded() != 0)
                        .map(Map.Entry::getKey)
                        .collect(Collectors.toSet())
        ).isEqualTo(shouldChangePassedAdded);
    }

    private Map<TestDiffType, MainStatistics> calculateDiffStatisticsForAllDiffTypes(boolean isStrongMode) {
        return Arrays.stream(TestDiffType.values())
                .filter(it -> it != TestDiffType.UNRECOGNIZED)
                .collect(Collectors.toMap(Function.identity(), testDiffType -> {
                    cleanUpStatistics();
                    return calculateDiffStatistics(testDiffType, isStrongMode)
                            .getAll().getMain();
                }));
    }

    private ChunkAggregateStatistics calculateDiffStatistics(TestDiffType testDiffType, boolean isStrongMode) {
        var aggregate = ChunkAggregateEntity.create(iterationId).toMutable();
        TestDiffByHashEntity diff = TestDiffByHashEntity.builder()
                .id(diffId)
                .resultType(ResultType.RT_TEST_MEDIUM)
                .statistics(
                        new TestDiffStatistics(
                                taskDiffProcessor.calculateStageDiffStatistics(
                                        testDiffType, false, false, isStrongMode
                                ),
                                TestDiffStatistics.StatisticsGroup.EMPTY
                        )
                )
                .build();

        taskDiffProcessor.applyTestDiff(diff, allToolchainsStatistics, suiteStatistics,
                suiteAllToolchainsStatistics, aggregate);

        return aggregate.toImmutable().getStatistics();
    }

    private void cleanUpStatistics() {
        this.allToolchainsStatistics = TestDiffStatistics.EMPTY.toMutable();
        this.suiteStatistics = TestDiffStatistics.EMPTY.toMutable();
        this.suiteAllToolchainsStatistics = TestDiffStatistics.EMPTY.toMutable();
    }
}
