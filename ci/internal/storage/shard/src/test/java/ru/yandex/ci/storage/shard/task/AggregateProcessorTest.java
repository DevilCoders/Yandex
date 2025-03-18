package ru.yandex.ci.storage.shard.task;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.Common.ResultType;
import ru.yandex.ci.storage.core.Common.TestStatus;
import ru.yandex.ci.storage.core.StorageTestUtils;
import ru.yandex.ci.storage.core.db.model.check_iteration.ExtendedStageStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.ExtendedStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.MainStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.StageStatistics;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.task_result.TestResult;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultEntity;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffByHashEntity;
import ru.yandex.ci.storage.shard.ShardStatistics;
import ru.yandex.ci.storage.shard.StorageShardTestBase;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.mock;

public class AggregateProcessorTest extends StorageShardTestBase {
    private static final TestEntity.Id TEST_ID = new TestEntity.Id(
            1L, "b", 3L
    );

    private static final TestEntity.Id TEST_ID_TWO = new TestEntity.Id(
            1L, "b", 4L
    );

    private AggregateProcessor aggregateProcessor;

    @BeforeEach
    public void setUp() {
        var statistics = mock(ShardStatistics.class);
        aggregateProcessor = new AggregateProcessor(new TaskDiffProcessor(statistics));
    }

    @Test
    public void processesSingleBrokenTest() {
        var aggregate = StorageTestUtils.createAggregate().toMutable();

        var leftResult = TestResult.builder()
                .id(createId(aggregate.getId(), TEST_ID, "left"))
                .chunkId(aggregate.getId().getChunkId())
                .status(TestStatus.TS_OK)
                .resultType(ResultType.RT_BUILD)
                .build();

        var rightResult = TestResult.builder()
                .id(createId(aggregate.getId(), TEST_ID, "right"))
                .chunkId(aggregate.getId().getChunkId())
                .status(TestStatus.TS_FAILED)
                .isRight(true)
                .resultType(ResultType.RT_BUILD)
                .build();

        shardCache.modify(cache -> {
            aggregateProcessor.process(cache, aggregate, leftResult);
            aggregateProcessor.process(cache, aggregate, rightResult);
        });

        var result = aggregate.toImmutable();

        var expectedMain = MainStatistics.builder()
                .build(
                        StageStatistics.builder()
                                .total(1)
                                .failed(1)
                                .failedAdded(1)
                                .build()
                )
                .total(
                        StageStatistics.builder()
                                .total(1)
                                .failed(1)
                                .failedAdded(1)
                                .build()
                )
                .build();

        var expectedExtended = ExtendedStatistics.EMPTY;

        assertThat(result.getStatistics().getAll().getMain()).isEqualTo(expectedMain);
        assertThat(result.getStatistics().getAll().getExtended()).isEqualTo(expectedExtended);
    }

    @Test
    public void processesSingleBrokenTestRestart() {
        var aggregate = StorageTestUtils.createAggregate().toMutable();

        var leftResult = TestResult.builder()
                .id(createId(aggregate.getId(), TEST_ID, "left"))
                .chunkId(aggregate.getId().getChunkId())
                .status(TestStatus.TS_OK)
                .resultType(ResultType.RT_TEST_MEDIUM)
                .build();

        var rightResult = TestResult.builder()
                .id(createId(aggregate.getId(), TEST_ID, "right"))
                .chunkId(aggregate.getId().getChunkId())
                .status(TestStatus.TS_FAILED)
                .isRight(true)
                .resultType(ResultType.RT_TEST_MEDIUM)
                .build();

        shardCache.modify(cache -> {
            aggregateProcessor.process(cache, aggregate, leftResult);
            aggregateProcessor.process(cache, aggregate, rightResult);
        });

        var secondAggregateMutable = StorageTestUtils.createAggregate().toMutable();
        secondAggregateMutable.setId(aggregate.getId().toIterationId(2));

        var metaAggregateMutable = aggregate.toImmutable().toMutable();
        metaAggregateMutable.setId(aggregate.getId().toIterationMetaId());

        var rightResultRestart = TestResult.builder()
                .id(createId(secondAggregateMutable.getId(), TEST_ID, "right"))
                .chunkId(aggregate.getId().getChunkId())
                .status(TestStatus.TS_OK)
                .isRight(true)
                .resultType(ResultType.RT_TEST_MEDIUM)
                .build();

        shardCache.modify(cache -> {
            aggregateProcessor.process(cache, secondAggregateMutable, metaAggregateMutable, rightResultRestart);
        });

        var secondAggregate = secondAggregateMutable.toImmutable();
        var metaAggregate = metaAggregateMutable.toImmutable();

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
                .flaky(
                        ExtendedStageStatistics.builder()
                                .total(1)
                                .failedAdded(1)
                                .build()
                )
                .build();

        assertThat(metaAggregate.getStatistics().getAll().getMain()).isEqualTo(expectedMain);
        assertThat(metaAggregate.getStatistics().getAll().getExtended()).isEqualTo(expectedExtended);

        assertThat(secondAggregate.getStatistics().getAll().getMain()).isEqualTo(expectedMain);
        assertThat(secondAggregate.getStatistics().getAll().getExtended()).isEqualTo(expectedExtended);

        var firstAggregateDiffs = shardCache.testDiffs().get(
                aggregate.getId()
        ).getAll();

        assertThat(firstAggregateDiffs.stream().noneMatch(TestDiffByHashEntity::isLast)).isTrue();

    }

    @Test
    public void processesSeveralTests() {
        var aggregate = StorageTestUtils.createAggregate().toMutable();

        var leftResult = TestResult.builder()
                .id(createId(aggregate.getId(), TEST_ID, "left"))
                .chunkId(aggregate.getId().getChunkId())
                .status(TestStatus.TS_OK)
                .resultType(ResultType.RT_TEST_MEDIUM)
                .build();

        var rightResult = TestResult.builder()
                .id(createId(aggregate.getId(), TEST_ID, "right"))
                .chunkId(aggregate.getId().getChunkId())
                .status(TestStatus.TS_FAILED)
                .isRight(true)
                .resultType(ResultType.RT_TEST_MEDIUM)
                .build();

        var rightResultForFlaky = TestResult.builder()
                .id(createId(aggregate.getId(), TEST_ID, "right"))
                .chunkId(aggregate.getId().getChunkId())
                .status(TestStatus.TS_OK)
                .isRight(true)
                .resultType(ResultType.RT_TEST_MEDIUM)
                .build();

        var leftSuiteResult = TestResult.builder()
                .id(createId(aggregate.getId(), TEST_ID.toSuiteId(), "left"))
                .chunkId(aggregate.getId().getChunkId())
                .status(TestStatus.TS_OK)
                .resultType(ResultType.RT_TEST_SUITE_MEDIUM)
                .build();

        var rightSuiteResult = TestResult.builder()
                .id(createId(aggregate.getId(), TEST_ID.toSuiteId(), "right"))
                .chunkId(aggregate.getId().getChunkId())
                .status(TestStatus.TS_OK)
                .isRight(true)
                .resultType(ResultType.RT_TEST_SUITE_MEDIUM)
                .build();

        var leftResultTwo = TestResult.builder()
                .id(createId(aggregate.getId(), TEST_ID_TWO, "left"))
                .chunkId(aggregate.getId().getChunkId())
                .status(TestStatus.TS_OK)
                .resultType(ResultType.RT_TEST_MEDIUM)
                .build();

        var rightResultTwo = TestResult.builder()
                .id(createId(aggregate.getId(), TEST_ID_TWO, "right"))
                .chunkId(aggregate.getId().getChunkId())
                .status(TestStatus.TS_OK)
                .resultType(ResultType.RT_TEST_MEDIUM)
                .isRight(true)
                .build();

        shardCache.modify(cache -> {
            aggregateProcessor.process(cache, aggregate, leftResult);
            aggregateProcessor.process(cache, aggregate, rightResult);
            aggregateProcessor.process(cache, aggregate, leftSuiteResult);
            aggregateProcessor.process(cache, aggregate, rightSuiteResult);
            aggregateProcessor.process(cache, aggregate, rightResultForFlaky);
            aggregateProcessor.process(cache, aggregate, leftResultTwo);
            aggregateProcessor.process(cache, aggregate, rightResultTwo);
        });

        var result = aggregate.toImmutable();

        var expectedMain = MainStatistics.builder()
                .mediumTests(
                        StageStatistics.builder()
                                .total(3)
                                .passed(2)
                                .build()
                )
                .total(
                        StageStatistics.builder()
                                .total(3)
                                .passed(2)
                                .build()
                )
                .build();

        var expectedExtended = ExtendedStatistics.builder()
                .flaky(
                        ExtendedStageStatistics.builder()
                                .total(1)
                                .failedAdded(1)
                                .build()
                )
                .build();

        assertThat(result.getStatistics().getAll().getMain()).isEqualTo(expectedMain);
        assertThat(result.getStatistics().getAll().getExtended()).isEqualTo(expectedExtended);
    }

    @Test
    public void processesDeletedTest() {
        var aggregate = StorageTestUtils.createAggregate().toMutable();
        var leftResult = TestResult.builder()
                .id(createId(aggregate.getId(), TEST_ID, "left"))
                .chunkId(aggregate.getId().getChunkId())
                .status(TestStatus.TS_OK)
                .resultType(ResultType.RT_BUILD)
                .build();

        var rightResult = TestResult.builder()
                .id(createId(aggregate.getId(), TEST_ID, "left"))
                .chunkId(aggregate.getId().getChunkId())
                .status(TestStatus.TS_NONE)
                .isRight(true)
                .resultType(ResultType.RT_BUILD)
                .build();


        shardCache.modify(cache -> {
            aggregateProcessor.process(cache, aggregate, leftResult);
            aggregateProcessor.process(cache, aggregate, rightResult);
        });

        var result = aggregate.toImmutable();
        var expectedMain = MainStatistics.builder()
                .build(
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

        assertThat(result.getStatistics().getAll().getMain()).isEqualTo(expectedMain);
        assertThat(result.getStatistics().getAll().getExtended()).isEqualTo(expectedExtended);

        var allToolchainsDiff = shardCache.testDiffs().getOrThrow(
                TestDiffByHashEntity.Id.of(
                        new ChunkAggregateEntity.Id(aggregate.getId().getIterationId(), aggregate.getId().getChunkId()),
                        TEST_ID.toAllToolchainsId()
                )
        );

        assertThat(allToolchainsDiff.getLeft()).isEqualTo(TestStatus.TS_OK);
        assertThat(allToolchainsDiff.getRight()).isEqualTo(TestStatus.TS_NONE);
        assertThat(allToolchainsDiff.getDiffType()).isEqualTo(Common.TestDiffType.TDT_DELETED);
    }

    @Test
    public void processesNewTest() {
        var aggregate = StorageTestUtils.createAggregate().toMutable();

        var leftResult = TestResult.builder()
                .id(createId(aggregate.getId(), TEST_ID, "right"))
                .chunkId(aggregate.getId().getChunkId())
                .status(TestStatus.TS_NONE)
                .isRight(false)
                .resultType(ResultType.RT_TEST_MEDIUM)
                .build();

        var rightResult = TestResult.builder()
                .id(createId(aggregate.getId(), TEST_ID, "right"))
                .chunkId(aggregate.getId().getChunkId())
                .status(TestStatus.TS_OK)
                .isRight(true)
                .resultType(ResultType.RT_TEST_MEDIUM)
                .build();

        shardCache.modify(cache -> aggregateProcessor.process(cache, aggregate, rightResult));

        var result = aggregate.toImmutable();

        assertThat(result.getStatistics().getAll().getMain()).isEqualTo(
                MainStatistics.builder()
                        .mediumTests(StageStatistics.builder().total(1).passed(1).build())
                        .total(StageStatistics.builder().total(1).passed(1).build())
                        .build()
        );

        assertThat(result.getStatistics().getAll().getExtended()).isEqualTo(ExtendedStatistics.EMPTY);

        shardCache.modify(cache -> aggregateProcessor.process(cache, aggregate, leftResult));

        result = aggregate.toImmutable();

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

        assertThat(result.getStatistics().getAll().getMain()).isEqualTo(expectedMain);
        assertThat(result.getStatistics().getAll().getExtended()).isEqualTo(expectedExtended);
    }

    @Test
    public void rollbacksBrokenTest() {
        var aggregate = StorageTestUtils.createAggregate().toMutable();

        var leftResult = TestResult.builder()
                .id(createId(aggregate.getId(), TEST_ID, "left"))
                .chunkId(aggregate.getId().getChunkId())
                .status(TestStatus.TS_OK)
                .resultType(ResultType.RT_TEST_MEDIUM)
                .build();

        var rightResult = TestResult.builder()
                .id(createId(aggregate.getId(), TEST_ID, "right"))
                .chunkId(aggregate.getId().getChunkId())
                .status(TestStatus.TS_FAILED)
                .isRight(true)
                .resultType(ResultType.RT_TEST_MEDIUM)
                .build();

        var nextRightResult = TestResult.builder()
                .id(createId(aggregate.getId(), TEST_ID, "right"))
                .chunkId(aggregate.getId().getChunkId())
                .status(TestStatus.TS_OK)
                .isRight(true)
                .resultType(ResultType.RT_TEST_MEDIUM)
                .build();

        shardCache.modify(cache -> {
            aggregateProcessor.process(cache, aggregate, leftResult);
            aggregateProcessor.process(cache, aggregate, rightResult);
            aggregateProcessor.process(cache, aggregate, nextRightResult);
        });

        var result = aggregate.toImmutable();

        var expectedMain = MainStatistics.builder()
                .mediumTests(
                        StageStatistics.builder()
                                .total(1)
                                .passed(0)
                                .build()
                )
                .total(
                        StageStatistics.builder()
                                .total(1)
                                .passed(0)
                                .build()
                )
                .build();

        var expectedExtended = ExtendedStatistics.builder()
                .flaky(
                        ExtendedStageStatistics.builder()
                                .total(1)
                                .failedAdded(1)
                                .build()
                )
                .build();

        assertThat(result.getStatistics().getAll().getMain()).isEqualTo(expectedMain);
        assertThat(result.getStatistics().getAll().getExtended()).isEqualTo(expectedExtended);
    }

    @Test
    public void noneToFailedThenRestartOkToOk() {
        var aggregate = StorageTestUtils.createAggregate().toMutable();

        var rightResult = TestResult.builder()
                .id(createId(aggregate.getId(), TEST_ID, "right"))
                .chunkId(aggregate.getId().getChunkId())
                .status(TestStatus.TS_FAILED)
                .isRight(true)
                .resultType(ResultType.RT_TEST_MEDIUM)
                .build();

        shardCache.modify(cache -> {
            aggregateProcessor.process(cache, aggregate, rightResult);
        });

        var secondAggregateMutable = StorageTestUtils.createAggregate().toMutable();
        secondAggregateMutable.setId(aggregate.getId().toIterationId(2));

        var metaAggregateMutable = aggregate.toImmutable().toMutable();
        metaAggregateMutable.setId(aggregate.getId().toIterationMetaId());

        var leftResultRestart = TestResult.builder()
                .id(createId(secondAggregateMutable.getId(), TEST_ID, "left"))
                .chunkId(aggregate.getId().getChunkId())
                .status(TestStatus.TS_OK)
                .isRight(false)
                .resultType(ResultType.RT_TEST_MEDIUM)
                .build();

        var rightResultRestart = TestResult.builder()
                .id(createId(secondAggregateMutable.getId(), TEST_ID, "right"))
                .chunkId(aggregate.getId().getChunkId())
                .status(TestStatus.TS_OK)
                .isRight(true)
                .resultType(ResultType.RT_TEST_MEDIUM)
                .build();

        shardCache.modify(cache -> {
            aggregateProcessor.process(cache, secondAggregateMutable, metaAggregateMutable, leftResultRestart);
            aggregateProcessor.process(cache, secondAggregateMutable, metaAggregateMutable, rightResultRestart);
        });

        var secondAggregate = secondAggregateMutable.toImmutable();
        var metaAggregate = metaAggregateMutable.toImmutable();

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
                .flaky(
                        ExtendedStageStatistics.builder()
                                .total(1)
                                .failedAdded(1)
                                .build()
                )
                .build();

        assertThat(metaAggregate.getStatistics().getAll().getMain()).isEqualTo(expectedMain);
        assertThat(metaAggregate.getStatistics().getAll().getExtended()).isEqualTo(expectedExtended);

        assertThat(secondAggregate.getStatistics().getAll().getMain()).isEqualTo(expectedMain);
        assertThat(secondAggregate.getStatistics().getAll().getExtended()).isEqualTo(expectedExtended);
    }

    /* todo broken suite child skip
    @Test
    public void processesBrokenSuiteBeforeTest() {
        var leftSuiteResult = TaskResultEntity.builder()
                .id(
                        new TaskResultEntity.Id(
                                new TestDiffEntity.Id(aggregate.getId(), TEST_ID.toSuiteId()), 0, false, 0
                        )
                )
                .status(TestStatus.TS_TIMEOUT)
                .resultType(ResultType.RT_TEST_SUITE_MEDIUM)
                .build();

        var rightSuiteResult = TaskResultEntity.builder()
                .id(
                        new TaskResultEntity.Id(
                                new TestDiffEntity.Id(aggregate.getId(), TEST_ID.toSuiteId()), 0, true, 0
                        )
                )
                .status(TestStatus.TS_FAILED)
                .resultType(ResultType.RT_TEST_SUITE_MEDIUM)
                .build();

        var leftResult = TaskResultEntity.builder()
                .id(createId(false))
                .status(TestStatus.TS_OK)
                .resultType(ResultType.RT_TEST_MEDIUM)
                .build();

        var rightResult = TaskResultEntity.builder()
                .id(createId(true))
                .status(TestStatus.TS_FAILED)
                .resultType(ResultType.RT_TEST_MEDIUM)
                .build();

        processor.processResults(
                check, aggregate.getId(), checkTask, 1,
                Arrays.asList(leftSuiteResult, rightSuiteResult, leftResult, rightResult)
        );

        var result = this.db.readOnly().run(() -> this.db.chunkAggregates().get(aggregate.getId()));
        var expectedMain = MainStatistics.builder()
                .mediumTests(
                        StageStatistics.builder()
                                .total(1)
                                .passed(1)
                                .passedAdded(1)
                                .build()
                )
                .total(
                        StageStatistics.builder()
                                .total(1)
                                .passed(1)
                                .passedAdded(1)
                                .build()
                )
                .build();

        var expectedExtended = ExtendedStatistics.builder()
                .added(ExtendedStageStatistics.builder().total(1).build())
                .build();

        assertThat(result.getStatistics().getRaw().getAll().getMain()).isEqualTo(expectedMain);
        assertThat(result.getStatistics().getRaw().getAll().getExtended()).isEqualTo(expectedExtended);
    }

    @Test
    public void processesBrokenSuiteAfterTest() {
        var leftSuiteResult = TaskResultEntity.builder()
                .id(
                        new TaskResultEntity.Id(
                                new TestDiffEntity.Id(aggregate.getId(), TEST_ID.toSuiteId()), 0, false, 0
                        )
                )
                .status(TestStatus.TS_FAILED)
                .resultType(ResultType.RT_TEST_SUITE_MEDIUM)
                .build();

        var rightSuiteResult = TaskResultEntity.builder()
                .id(
                        new TaskResultEntity.Id(
                                new TestDiffEntity.Id(aggregate.getId(), TEST_ID.toSuiteId()), 0, true, 0
                        )
                )
                .status(TestStatus.TS_TIMEOUT)
                .resultType(ResultType.RT_TEST_SUITE_MEDIUM)
                .build();

        var leftResult = TaskResultEntity.builder()
                .id(createId(false))
                .status(TestStatus.TS_OK)
                .resultType(ResultType.RT_TEST_MEDIUM)
                .build();

        var rightResult = TaskResultEntity.builder()
                .id(createId(true))
                .name("")
                .path("")
                .status(TestStatus.TS_FAILED)
                .resultType(ResultType.RT_TEST_MEDIUM)
                .build();

        processor.processResults(
                check, aggregate.getId(), checkTask, 1,
                Arrays.asList(leftResult, rightResult, leftSuiteResult, rightSuiteResult)
        );

        var result = this.shardCache.chunkAggregates().get(aggregate.getId());
        assertThat(result).isPresent();
        var expectedMain = MainStatistics.builder()
                .mediumTests(
                        StageStatistics.builder()
                                .total(1)
                                .passed(1)
                                .passedAdded(1)
                                .build()
                )
                .total(
                        StageStatistics.builder()
                                .total(1)
                                .passed(1)
                                .passedAdded(1)
                                .build()
                )
                .build();

        var expectedExtended = ExtendedStatistics.builder()
                .added(ExtendedStageStatistics.builder().total(1).build())
                .build();

        assertThat(result.get().getStatistics().getRaw().getAll().getMain()).isEqualTo(expectedMain);
        assertThat(result.get().getStatistics().getRaw().getAll().getExtended()).isEqualTo(expectedExtended);
    }
     */

    private TestResultEntity.Id createId(
            ChunkAggregateEntity.Id aggregateId,
            TestEntity.Id testId, String taskId
    ) {
        return new TestResultEntity.Id(
                aggregateId.getIterationId(), testId, taskId, 0, 0
        );
    }
}
