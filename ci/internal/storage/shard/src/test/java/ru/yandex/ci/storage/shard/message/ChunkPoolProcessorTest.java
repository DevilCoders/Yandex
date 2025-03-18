package ru.yandex.ci.storage.shard.message;

import java.time.Instant;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.atomic.AtomicLong;
import java.util.stream.Collectors;

import com.google.common.primitives.UnsignedLong;
import com.google.protobuf.Timestamp;
import lombok.Value;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.ArgumentCaptor;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.boot.test.mock.mockito.MockReset;

import ru.yandex.ci.ayamler.Ayamler;
import ru.yandex.ci.logbroker.LbCommitCountdown;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.ShardIn;
import ru.yandex.ci.storage.core.TaskMessages;
import ru.yandex.ci.storage.core.cache.StorageCoreCache;
import ru.yandex.ci.storage.core.db.constant.Trunk;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.IterationInfo;
import ru.yandex.ci.storage.core.db.model.check_iteration.StrongModePolicy;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateStatistics;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;
import ru.yandex.ci.storage.core.db.model.task_result.TestResult;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffByHashEntity;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.shard.ShardStatistics;
import ru.yandex.ci.storage.shard.StorageShardTestBase;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

public class ChunkPoolProcessorTest extends StorageShardTestBase {
    private static final AtomicLong SEQ_NO = new AtomicLong();
    private static final TestEntity.Id TEST_ID = new TestEntity.Id(1L, "b", 3L);

    ChunkEntity.Id chunkId = ChunkEntity.Id.of(Common.ChunkType.CT_SMALL_TEST, 1);

    private ChunkPoolProcessor processor;

    @Autowired
    private ShardStatistics shardStatistics;

    @MockBean(reset = MockReset.BEFORE)
    private AggregatePoolProcessor aggregatePoolProcessor;

    @BeforeEach
    public void setUp() {
        this.processor = new ChunkPoolProcessor(
                db, shardCache,
                timeTraceService,
                aggregatePoolProcessor,
                shardStatistics,
                0,
                0,
                true
        );
    }

    @Test
    public void notifiesMissingProcessed() {
        var check = prepareCheckTestData();

        var leftResult = createResult(TEST_ID, Common.TestStatus.TS_OK);
        var leftSecondResult = createResult(TEST_ID, Common.TestStatus.TS_OK);
        var rightResult = createResult(TEST_ID, Common.TestStatus.TS_FAILED);

        var leftCountdown = mock(LbCommitCountdown.class);
        var rightCountdown = mock(LbCommitCountdown.class);

        processor.enqueue(createMessage(check.leftTaskId, List.of(leftResult), leftCountdown));
        processor.enqueue(createMessage(check.leftTaskId, List.of(leftSecondResult), leftCountdown));
        processor.enqueue(
                createMessage(
                        new CheckTaskEntity.Id(sampleIterationId, "missing"),
                        List.of(rightResult),
                        rightCountdown
                )
        );

        verify(rightCountdown, times(1)).notifyMessageProcessed();
    }

    private ChunkPoolMessage createMessage(
            CheckTaskEntity.Id taskId, List<TaskMessages.AutocheckTestResult> results, LbCommitCountdown countdown
    ) {
        return new ChunkPoolMessage(
                createMeta(),
                ChunkEntity.Id.of(Common.ChunkType.CT_SMALL_TEST, 1),
                countdown,
                ShardIn.ChunkMessage.newBuilder()
                        .setChunkId(CheckProtoMappers.toProtoChunkId(chunkId))
                        .setResult(
                                ShardIn.ShardResultMessage.newBuilder()
                                        .setFullTaskId(CheckProtoMappers.toProtoCheckTaskId(taskId))
                                        .setPartition(1)
                                        .setAutocheckTestResults(
                                                TaskMessages.AutocheckTestResults.newBuilder()
                                                        .addAllResults(results)
                                                        .build()
                                        )
                                        .build()
                        )
                        .build()
        );
    }

    @Test
    public void ignoresDataOnEmptyStatistics() {
        var check = prepareCheckTestData();

        var rightResult = createResult(TEST_ID, Common.TestStatus.TS_FAILED);
        var rightCountdown = mock(LbCommitCountdown.class);

        var aggregateId = new ChunkAggregateEntity.Id(sampleIterationId, chunkId);

        // null statistics
        this.shardCache.modifyWithDbTx(cache -> {
            cache.testDiffs().writeThrough(
                    exampleTestDiff(aggregateId, new TestEntity.Id(
                            UnsignedLong.MAX_VALUE.longValue(),
                            "toolchain",
                            UnsignedLong.MAX_VALUE.longValue()
                    ))
            );
        });

        this.shardCache.modify(StorageCoreCache.Modifiable::invalidateAll);

        processor.enqueue(createMessage(check.rightTaskId, List.of(rightResult), rightCountdown));

        var diffs = this.shardCache.testDiffs().get(aggregateId).getAll();
        assertThat(diffs).hasSize(0);

        // empty statistics
        this.shardCache.modifyWithDbTx(cache -> {
            cache.chunkAggregates().writeThrough(
                    ChunkAggregateEntity.create(new ChunkAggregateEntity.Id(sampleIterationId, chunkId))
            );

            cache.testDiffs().writeThrough(createDiff(aggregateId));
        });

        this.shardCache.modify(StorageCoreCache.Modifiable::invalidateAll);

        processor.enqueue(createMessage(check.rightTaskId, List.of(rightResult), rightCountdown));

        diffs = this.shardCache.testDiffs().get(aggregateId).getAll();
        assertThat(diffs).hasSize(0);
    }

    private TestDiffByHashEntity createDiff(ChunkAggregateEntity.Id aggregateId) {
        return exampleTestDiff(aggregateId, TEST_ID);
    }

    @Test
    public void reloadsOnNotEmptyStatistics() {
        var check = prepareCheckTestData();

        var rightResult = createResult(TEST_ID, Common.TestStatus.TS_FAILED);
        var rightCountdown = mock(LbCommitCountdown.class);

        var allToolchains = ChunkAggregateStatistics.ToolchainStatistics.EMPTY.toMutable();
        allToolchains.getMain().getTotal().setTotal(1);

        var statistics = new ChunkAggregateStatistics(
                Map.of(TestEntity.ALL_TOOLCHAINS, allToolchains.toImmutable())
        );

        var aggregateId = new ChunkAggregateEntity.Id(sampleIterationId, chunkId);
        this.shardCache.modifyWithDbTx(cache -> {
            cache.chunkAggregates().writeThrough(
                    ChunkAggregateEntity.create(new ChunkAggregateEntity.Id(sampleIterationId, chunkId)).toBuilder()
                            .statistics(statistics)
                            .build()
            );

            cache.testDiffs().writeThrough(
                    exampleTestDiff(aggregateId, new TestEntity.Id(
                            UnsignedLong.MAX_VALUE.longValue(),
                            "toolchain",
                            UnsignedLong.MAX_VALUE.longValue()
                    ))
            );
        });

        this.shardCache.modify(StorageCoreCache.Modifiable::invalidateAll);

        processor.enqueue(createMessage(check.rightTaskId, List.of(rightResult), rightCountdown));

        var diffs = this.shardCache.testDiffs().get(aggregateId).getAll();
        assertThat(diffs).hasSize(1);
    }

    private TestDiffByHashEntity exampleTestDiff(ChunkAggregateEntity.Id aggregateId, TestEntity.Id testId) {
        return TestDiffByHashEntity.builder()
                .id(
                        TestDiffByHashEntity.Id.of(
                                aggregateId,
                                testId
                        )
                )
                .resultType(Common.ResultType.RT_TEST_SMALL)
                .build();
    }

    private Common.MessageMeta createMeta() {
        return Common.MessageMeta.newBuilder()
                .setTimestamp(Timestamp.newBuilder().build())
                .setId("id")
                .build();
    }

    @Test
    void process_whenCheckIterationHasStrongModeForcedOn() {
        var check = prepareCheckTestData(StrongModePolicy.FORCE_ON);
        var strongModeInAYaml = Ayamler.StrongModeStatus.OFF;

        var leftResult = createResult(TEST_ID, Common.TestStatus.TS_OK, "ci/tms/src/main/test");
        var rightResult = createResult(TEST_ID, Common.TestStatus.TS_TIMEOUT, "ci/tms/src/main/test");

        reset(aYamlerClient);
        mockAYamlerResponse(Map.of("ci/tms/src/main/test", strongModeInAYaml));

        processor.process(List.of(
                createMessage(check.leftTaskId, List.of(leftResult), mock(LbCommitCountdown.class)),
                createMessage(check.rightTaskId, List.of(rightResult), mock(LbCommitCountdown.class))
        ));

        // check that result has strongMode==true, despite of that a.yaml has strongMode=false
        verify(aggregatePoolProcessor, times(2)).enqueue(argThat(message ->
                message.getResult().getResults()
                        .stream()
                        .allMatch(TestResult::isStrongMode)
        ));
    }

    @Test
    void process_whenCheckIterationHasStrongModeForcedOff() {
        var check = prepareCheckTestData(StrongModePolicy.FORCE_OFF);
        var strongModeInAYaml = Ayamler.StrongModeStatus.ON;

        var leftResult = createResult(TEST_ID, Common.TestStatus.TS_OK, "ci/tms/src/main/test");
        var rightResult = createResult(TEST_ID, Common.TestStatus.TS_TIMEOUT, "ci/tms/src/main/test");

        reset(aYamlerClient);
        mockAYamlerResponse(Map.of("ci/tms/src/main/test", strongModeInAYaml));

        processor.process(List.of(
                createMessage(check.leftTaskId, List.of(leftResult), mock(LbCommitCountdown.class)),
                createMessage(check.rightTaskId, List.of(rightResult), mock(LbCommitCountdown.class))
        ));

        // check that result has strongMode==true, despite of that a.yaml has strongMode=false
        verify(aggregatePoolProcessor, times(2)).enqueue(argThat(message ->
                message.getResult().getResults()
                        .stream()
                        .noneMatch(TestResult::isStrongMode)
        ));
    }

    @Test
    void process_whenCheckIterationsHaveMixedStrongModePolicies() {
        var strongModeByPath = new HashMap<String, Ayamler.StrongModeStatus>();

        var aCheck = prepareCheckTestData(StrongModePolicy.FORCE_OFF);
        var aTestId = new TestEntity.Id(1L, "b", 3L);
        var aLeftResult = createResult(aTestId, Common.TestStatus.TS_OK, "ci/tms/src/main/test");
        var aRightResult = createResult(aTestId, Common.TestStatus.TS_TIMEOUT, "ci/tms/src/main/test");
        strongModeByPath.put("ci/tms/src/main/test", Ayamler.StrongModeStatus.OFF);

        var bCheck = prepareCheckTestData(StrongModePolicy.BY_A_YAML);
        var bTestId = new TestEntity.Id(4L, "b", 6L);
        var bLeftResult = createResult(bTestId, Common.TestStatus.TS_OK, "ci/storage/src/main/test");
        var bRightResult = createResult(bTestId, Common.TestStatus.TS_TIMEOUT, "ci/storage/src/main/test");
        strongModeByPath.put("ci/storage/src/main/test", Ayamler.StrongModeStatus.ON);

        reset(aYamlerClient);
        mockAYamlerResponse(strongModeByPath);

        processor.process(List.of(
                createMessage(aCheck.leftTaskId, List.of(aLeftResult), mock(LbCommitCountdown.class)),
                createMessage(aCheck.rightTaskId, List.of(aRightResult), mock(LbCommitCountdown.class)),
                createMessage(bCheck.leftTaskId, List.of(bLeftResult), mock(LbCommitCountdown.class)),
                createMessage(bCheck.rightTaskId, List.of(bRightResult), mock(LbCommitCountdown.class))
        ));

        var aggregateMessageCaptor = ArgumentCaptor.forClass(AggregateMessage.class);
        verify(aggregatePoolProcessor, times(4)).enqueue(aggregateMessageCaptor.capture());

        Map<String, Set<Boolean>> pathToStrongMode = aggregateMessageCaptor.getAllValues().stream()
                .flatMap(it -> it.getResult().getResults().stream())
                .collect(Collectors.groupingBy(
                        TestResult::getPath,
                        Collectors.mapping(TestResult::isStrongMode, Collectors.toSet())
                ));

        assertThat(pathToStrongMode).isEqualTo(Map.of(
                "ci/tms/src/main/test", Set.of(false),
                "ci/storage/src/main/test", Set.of(true)
        ));
    }

    private static TaskMessages.AutocheckTestResult createResult(
            TestEntity.Id testId, Common.TestStatus status
    ) {
        return createResult(testId, status, "");
    }

    private static TaskMessages.AutocheckTestResult createResult(
            TestEntity.Id testId, Common.TestStatus status, String path
    ) {
        return TaskMessages.AutocheckTestResult.newBuilder()
                .setId(
                        TaskMessages.AutocheckTestId.newBuilder()
                                .setSuiteHid(testId.getSuiteId())
                                .setToolchain(testId.getToolchain())
                                .setHid(testId.getId())

                                .build()
                )
                .setOldSuiteId(testId.getSuiteIdString())
                .setOldId(testId.toString())
                .setResultType(Common.ResultType.RT_TEST_SMALL)
                .setTestStatus(status)
                .setPath(path)
                .build();
    }

    private static CheckIterationEntity createIteration(
            CheckIterationEntity.Id aCheckIterId,
            StrongModePolicy strongModePolicy
    ) {
        return CheckIterationEntity.builder()
                .id(aCheckIterId)
                .status(Common.CheckStatus.CREATED)
                .created(Instant.now())
                .info(IterationInfo.builder()
                        .strongModePolicy(strongModePolicy)
                        .build()
                )
                .build();
    }

    private CheckTestData prepareCheckTestData() {
        return prepareCheckTestData(StrongModePolicy.BY_A_YAML);
    }

    private CheckTestData prepareCheckTestData(StrongModePolicy strongModePolicy) {
        var check = CheckEntity.builder().id(CheckEntity.Id.of(SEQ_NO.incrementAndGet()))
                .left(new StorageRevision(Trunk.name(), "", 1, Instant.EPOCH))
                .right(new StorageRevision(Trunk.name(), "", 1, Instant.EPOCH))
                .author("check-author")
                .build();

        var checkIteration = CheckIterationEntity.Id.of(check.getId(), CheckIteration.IterationType.FULL, 1);

        var leftTaskId = new CheckTaskEntity.Id(checkIteration, "task-id-left-" + SEQ_NO.incrementAndGet());
        var rightTaskId = new CheckTaskEntity.Id(checkIteration, "task-id-right-" + SEQ_NO.incrementAndGet());

        db.currentOrTx(() -> {
            db.checks().save(check);
            db.checkIterations().save(createIteration(checkIteration, strongModePolicy));
            db.checkTasks().save(CheckTaskEntity.builder().id(leftTaskId).build());
            db.checkTasks().save(CheckTaskEntity.builder().id(rightTaskId).right(true).build());
        });
        return new CheckTestData(check, checkIteration, leftTaskId, rightTaskId);
    }

    @Value
    private static class CheckTestData {
        CheckEntity check;
        CheckIterationEntity.Id checkIteration;
        CheckTaskEntity.Id leftTaskId;
        CheckTaskEntity.Id rightTaskId;
    }
}
