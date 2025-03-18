package ru.yandex.ci.storage.core.export;

import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.task_result.TestResult;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultEntity;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;

@Value
@Builder
public class ExportedTaskResult {
    TestEntity.Id testId;

    String taskId;
    int partition;
    int retryNumber;

    boolean isRight;
    boolean isStrongMode;

    Common.ResultType resultType;
    Common.TestStatus status;

    public static ExportedTaskResult of(TestResultEntity entity) {
        return ExportedTaskResult.builder()
                .testId(entity.getTestId())
                .taskId(entity.getId().getTaskId())
                .partition(entity.getId().getPartition())
                .retryNumber(entity.getId().getRetryNumber())
                .isRight(entity.isRight())
                .isStrongMode(entity.isStrongMode())
                .resultType(entity.getResultType())
                .status(entity.getStatus())
                .build();
    }

    public TestResult toEntity(ChunkAggregateEntity.Id aggregateId) {
        return
                TestResult.builder()
                        .id(
                                new TestResultEntity.Id(
                                        aggregateId.getIterationId(),
                                        testId,
                                        taskId, partition, retryNumber
                                )
                        )
                        .chunkId(aggregateId.getChunkId())
                        .resultType(resultType)
                        .status(status)
                        .isRight(isRight)
                        .isStrongMode(isStrongMode)
                        // we don't fill `.strongModeAYaml()` here, cause we cant
                        .build();
    }
}
