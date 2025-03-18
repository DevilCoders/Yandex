package ru.yandex.ci.storage.shard.message;

import java.util.List;
import java.util.function.Function;
import java.util.stream.Collectors;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.logbroker.LbCommitCountdown;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;
import ru.yandex.ci.storage.core.db.model.task_result.TestResult;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultEntity;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffImportantEntity;
import ru.yandex.ci.storage.core.message.shard_out.ShardOutMessageWriter;
import ru.yandex.ci.storage.core.utils.TimeTraceService;
import ru.yandex.ci.storage.shard.ShardStatistics;
import ru.yandex.ci.storage.shard.StorageShardTestBase;
import ru.yandex.ci.storage.shard.task.AggregateProcessor;
import ru.yandex.ci.storage.shard.task.TaskDiffProcessor;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

public class AggregatePoolProcessorTest extends StorageShardTestBase {
    private static final TestEntity.Id TEST_ID_TWO = new TestEntity.Id(
            1L, "b", 4L
    );

    private final ChunkEntity.Id chunkId = ChunkEntity.Id.of(Common.ChunkType.CT_SMALL_TEST, 1);

    @BeforeEach
    public void setup() {
        db.currentOrTx(() -> {
            db.checks().save(
                    CheckEntity.builder()
                            .id(new CheckEntity.Id(1L))
                            .left(StorageRevision.EMPTY)
                            .right(StorageRevision.EMPTY)
                            .build()
            );
        });
    }

    @Test
    public void testWithResultRewrite() {
        var aggregateReporter = mock(AggregateReporter.class);
        var shardOutMessageWriter = mock(ShardOutMessageWriter.class);
        var shardStatistics = mock(ShardStatistics.class);
        var time = mock(TimeTraceService.class);
        var trace = mock(TimeTraceService.Trace.class);
        var postProcessor = mock(PostProcessorDeliveryService.class);
        when(time.createTrace(anyString())).thenReturn(trace);

        var aggregatePoolProcessor = new AggregatePoolProcessor(
                db, new AggregateProcessor(new TaskDiffProcessor(shardStatistics)),
                shardCache, aggregateReporter, shardOutMessageWriter, time, shardStatistics, postProcessor, 0, 0
        );

        var aggregateId = new ChunkAggregateEntity.Id(sampleIterationId, chunkId);
        var lbCommitCountdown = mock(LbCommitCountdown.class);

        var leftResult = TestResult.builder()
                .id(createId(sampleIterationId, TEST_ID, "left"))
                .status(Common.TestStatus.TS_OK)
                .chunkId(chunkId)
                .resultType(Common.ResultType.RT_TEST_MEDIUM)
                .build();

        var rightResult = TestResult.builder()
                .id(createId(sampleIterationId, TEST_ID, "right"))
                .status(Common.TestStatus.TS_OK)
                .chunkId(chunkId)
                .isRight(true)
                .resultType(Common.ResultType.RT_TEST_MEDIUM)
                .build();

        var rightResultTwo = TestResult.builder()
                .id(createId(sampleIterationId, TEST_ID_TWO, "right"))
                .status(Common.TestStatus.TS_OK)
                .chunkId(chunkId)
                .isRight(true)
                .resultType(Common.ResultType.RT_TEST_MEDIUM)
                .build();

        enqueue(aggregatePoolProcessor, aggregateId, lbCommitCountdown, List.of(leftResult, rightResult,
                rightResultTwo));

        var statistics = shardCache.chunkAggregates().getOrThrow(aggregateId).getStatistics().getAll();
        assertThat(statistics.getMain().getTotalOrEmpty().getTotal()).isEqualTo(2);

        aggregatePoolProcessor.enqueue(
                new AggregateMessage(
                        aggregateId,
                        lbCommitCountdown,
                        null,
                        new AggregateMessage.Finish(Common.ChunkAggregateState.CAS_RUNNING)
                )
        );

        statistics = shardCache.chunkAggregates().getOrThrow(aggregateId).getStatistics().getAll();
        assertThat(statistics.getMain().getTotalOrEmpty().getTotal()).isEqualTo(3);
        assertThat(statistics.getMain().getMediumTestsOrEmpty().getTotal()).isEqualTo(3);
        assertThat(statistics.getExtended().getAddedOrEmpty().getTotal()).isEqualTo(1);

        var diffs = db.currentOrReadOnly(() -> db.testDiffs().readTable().collect(Collectors.toList()));

        // 2 test diffs, 2 test @all platform diffs, 1 suite diff, 1 @all platforms suite diff
        assertThat(diffs).hasSize(6);
        assertThat(shardCache.testDiffs().get(aggregateId).getAll()).hasSize(6);
    }

    @Test
    public void testWithIterationRestart() {
        var aggregateReporter = mock(AggregateReporter.class);
        var shardOutMessageWriter = mock(ShardOutMessageWriter.class);
        var shardStatistics = mock(ShardStatistics.class);
        var time = mock(TimeTraceService.class);
        var trace = mock(TimeTraceService.Trace.class);
        var postProcessor = mock(PostProcessorDeliveryService.class);
        when(time.createTrace(anyString())).thenReturn(trace);

        var aggregatePoolProcessor = new AggregatePoolProcessor(
                db, new AggregateProcessor(new TaskDiffProcessor(shardStatistics)),
                shardCache, aggregateReporter, shardOutMessageWriter, time, shardStatistics, postProcessor, 0, 0
        );

        var aggregateId = new ChunkAggregateEntity.Id(sampleIterationId, chunkId);
        var lbCommitCountdown = mock(LbCommitCountdown.class);

        var leftResult = TestResult.builder()
                .id(createId(sampleIterationId, TEST_ID, "left"))
                .status(Common.TestStatus.TS_OK)
                .chunkId(chunkId)
                .resultType(Common.ResultType.RT_TEST_MEDIUM)
                .build();

        var rightResult = TestResult.builder()
                .id(createId(sampleIterationId, TEST_ID, "right"))
                .status(Common.TestStatus.TS_FAILED)
                .chunkId(chunkId)
                .resultType(Common.ResultType.RT_TEST_MEDIUM)
                .isRight(true)
                .build();

        enqueue(aggregatePoolProcessor, aggregateId, lbCommitCountdown, List.of(leftResult, rightResult));

        var rightResultRestart = TestResult.builder()
                .id(createId(sampleIterationId.toIterationId(2), TEST_ID, "right"))
                .status(Common.TestStatus.TS_OK)
                .chunkId(chunkId)
                .resultType(Common.ResultType.RT_TEST_MEDIUM)
                .isRight(true)
                .build();

        enqueue(aggregatePoolProcessor, aggregateId.toIterationId(2), lbCommitCountdown, List.of(rightResultRestart));

        var statistics = shardCache.chunkAggregates().getOrThrow(aggregateId.toIterationMetaId()).getStatistics()
                .getAll();

        assertThat(statistics.getMain().getTotalOrEmpty().getTotal()).isEqualTo(1);

        /* todo
        aggregatePoolProcessor.enqueue(
                new AggregateMessage(
                        aggregateId,
                        lbCommitCountdown,
                        null,
                        new AggregateMessage.Finish()
                )
        );
        */
        assertThat(statistics.getMain().getTotalOrEmpty().getTotal()).isEqualTo(1);
        assertThat(statistics.getExtended().getFlakyOrEmpty().getTotal()).isEqualTo(1);

        // todo check second iteration
    }

    private void enqueue(
            AggregatePoolProcessor aggregatePoolProcessor,
            ChunkAggregateEntity.Id aggregateId,
            LbCommitCountdown lbCommitCountdown,
            List<TestResult> results
    ) {
        aggregatePoolProcessor.enqueue(
                new AggregateMessage(
                        aggregateId,
                        lbCommitCountdown,
                        new ChunkMessageWithResults(
                                sampleCheck,
                                sampleTask,
                                ArcBranch.ofBranchName("test"),
                                1,
                                aggregateId,
                                lbCommitCountdown,
                                results

                        ),
                        null
                )
        );
    }

    @Test
    public void testFinishMissingResult() {
        var aggregateReporter = mock(AggregateReporter.class);
        var shardOutMessageWriter = mock(ShardOutMessageWriter.class);
        var shardStatistics = mock(ShardStatistics.class);
        var time = mock(TimeTraceService.class);
        var trace = mock(TimeTraceService.Trace.class);
        var postProcessor = mock(PostProcessorDeliveryService.class);
        when(time.createTrace(anyString())).thenReturn(trace);

        var aggregatePoolProcessor = new AggregatePoolProcessor(
                db, new AggregateProcessor(new TaskDiffProcessor(shardStatistics)),
                shardCache, aggregateReporter, shardOutMessageWriter, time, shardStatistics, postProcessor, 0, 0
        );

        var aggregateId = new ChunkAggregateEntity.Id(sampleIterationId, chunkId);
        var lbCommitCountdown = mock(LbCommitCountdown.class);

        var leftResult = TestResult.builder()
                .id(createId(sampleIterationId, TEST_ID, "left"))
                .status(Common.TestStatus.TS_OK)
                .chunkId(chunkId)
                .path("path")
                .resultType(Common.ResultType.RT_TEST_MEDIUM)
                .name("test-name")
                .build();

        enqueue(aggregatePoolProcessor, aggregateId, lbCommitCountdown, List.of(leftResult));

        aggregatePoolProcessor.enqueue(
                new AggregateMessage(
                        aggregateId,
                        lbCommitCountdown,
                        null,
                        new AggregateMessage.Finish(Common.ChunkAggregateState.CAS_COMPLETED)
                )
        );

        var statistics = shardCache.chunkAggregates().getOrThrow(aggregateId).getStatistics().getAll();
        assertThat(statistics.getExtended().getDeletedOrEmpty().getTotal()).isEqualTo(1);

        var diffs = db.currentOrReadOnly(
                () -> db.testDiffs().readTable()
                        .collect(Collectors.toMap(TestDiffEntity::getId, Function.identity()))
        );

        assertThat(diffs).hasSize(4);
        assertThat(
                diffs.get(
                        new TestDiffEntity.Id(sampleIterationId, Common.ResultType.RT_TEST_MEDIUM, "path", TEST_ID)
                ).getName()
        )
                .isEqualTo("test-name");

        assertThat(
                diffs.get(
                        new TestDiffEntity.Id(
                                sampleIterationId, Common.ResultType.RT_TEST_MEDIUM, "path", TEST_ID.toAllToolchainsId()
                        )
                ).getName()
        )
                .isEqualTo("test-name");

        var importantDiffs = db.currentOrReadOnly(
                () -> db.importantTestDiffs().readTable()
                        .collect(Collectors.toMap(TestDiffImportantEntity::getId, Function.identity()))
        );

        assertThat(importantDiffs.keySet()).containsOnly(
                new TestDiffImportantEntity.Id(
                        sampleIterationId, Common.ResultType.RT_TEST_SUITE_MEDIUM, "path", TEST_ID.toSuiteId()
                ),
                new TestDiffImportantEntity.Id(
                        sampleIterationId,
                        Common.ResultType.RT_TEST_SUITE_MEDIUM, "path",
                        TEST_ID.toSuiteId().toAllToolchainsId()
                )

        );

    }

    @Test
    public void noneToFailedThenRestartOkToOk() {
        var aggregateReporter = mock(AggregateReporter.class);
        var shardOutMessageWriter = mock(ShardOutMessageWriter.class);
        var shardStatistics = mock(ShardStatistics.class);
        var time = mock(TimeTraceService.class);
        var trace = mock(TimeTraceService.Trace.class);
        var postProcessor = mock(PostProcessorDeliveryService.class);
        when(time.createTrace(anyString())).thenReturn(trace);

        var aggregatePoolProcessor = new AggregatePoolProcessor(
                db, new AggregateProcessor(new TaskDiffProcessor(shardStatistics)),
                shardCache, aggregateReporter, shardOutMessageWriter, time, shardStatistics, postProcessor, 0, 0
        );

        var aggregateId = new ChunkAggregateEntity.Id(sampleIterationId, chunkId);
        var lbCommitCountdown = mock(LbCommitCountdown.class);

        var rightResult = TestResult.builder()
                .id(createId(sampleIterationId, TEST_ID, "right"))
                .status(Common.TestStatus.TS_FAILED)
                .isRight(true)
                .chunkId(chunkId)
                .path("path")
                .resultType(Common.ResultType.RT_TEST_MEDIUM)
                .name("test-name")
                .build();

        enqueue(aggregatePoolProcessor, aggregateId, lbCommitCountdown, List.of(rightResult));

        aggregatePoolProcessor.enqueue(
                new AggregateMessage(
                        aggregateId,
                        lbCommitCountdown,
                        null,
                        new AggregateMessage.Finish(Common.ChunkAggregateState.CAS_COMPLETED)
                )
        );

        var iterationIdTwo = sampleIterationId.toIterationId(2);
        var aggregateIdTwo = new ChunkAggregateEntity.Id(iterationIdTwo, chunkId);

        var leftResultTwo = TestResult.builder()
                .id(createId(iterationIdTwo, TEST_ID, "left"))
                .status(Common.TestStatus.TS_OK)
                .chunkId(chunkId)
                .path("path")
                .resultType(Common.ResultType.RT_TEST_MEDIUM)
                .name("test-name")
                .build();

        var rightResultTwo = TestResult.builder()
                .id(createId(iterationIdTwo, TEST_ID, "right"))
                .status(Common.TestStatus.TS_OK)
                .isRight(true)
                .chunkId(chunkId)
                .path("path")
                .resultType(Common.ResultType.RT_TEST_MEDIUM)
                .name("test-name")
                .build();

        enqueue(aggregatePoolProcessor, aggregateIdTwo, lbCommitCountdown, List.of(rightResultTwo, leftResultTwo));

        var statistics = shardCache.chunkAggregates().getOrThrow(aggregateId.toIterationMetaId())
                .getStatistics().getAll();
        assertThat(statistics.getExtended().getFlakyOrEmpty().getTotal()).isEqualTo(1);
    }

    private TestResultEntity.Id createId(
            CheckIterationEntity.Id iterationId, TestEntity.Id testId, String taskId
    ) {
        return new TestResultEntity.Id(
                iterationId, testId, taskId, 0, 0
        );
    }
}
