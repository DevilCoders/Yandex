package ru.yandex.ci.storage.core.yt.model;

import java.time.Instant;
import java.util.List;
import java.util.Map;
import java.util.Set;

import lombok.Value;

import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.task_result.ResultOwners;
import ru.yandex.ci.storage.core.db.model.task_result.TestOutput;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultEntity;
import ru.yandex.inside.yt.kosher.impl.ytree.object.NullSerializationStrategy;
import ru.yandex.inside.yt.kosher.impl.ytree.object.annotation.YTreeField;
import ru.yandex.inside.yt.kosher.impl.ytree.object.annotation.YTreeFlattenField;
import ru.yandex.inside.yt.kosher.impl.ytree.object.annotation.YTreeObject;
import ru.yandex.misc.lang.number.UnsignedLong;

@Value
@YTreeObject(nullSerializationStrategy = NullSerializationStrategy.SERIALIZE_NULL_TO_EMPTY)
public class YtTestResult {
    @YTreeFlattenField
    Id id;

    long autocheckChunkId;

    String oldTestId;

    Common.ChunkType storageChunkType;
    Integer storageChunkNumber;

    Map<String, List<String>> links;
    Map<String, Double> metrics;
    Map<String, TestOutput> testOutputs;

    String name;

    String branch;
    String path;
    String snippet;
    String subtestName;
    String processedBy;

    Set<String> tags;
    String requirements;

    String uid;

    long revisionNumber;

    boolean isRight;

    Instant created;

    Common.TestStatus status;

    Common.ResultType resultType;

    boolean isStrongMode;

    public YtTestResult(TestResultEntity result) {
        this.id = new Id(result.getId());
        this.autocheckChunkId = result.getAutocheckChunkId();
        this.oldTestId = result.getOldTestId();
        this.storageChunkType = result.getChunkId().getChunkType();
        this.storageChunkNumber = result.getChunkId().getNumber();
        this.links = result.getLinks();
        this.metrics = result.getMetrics();
        this.testOutputs = result.getTestOutputs();
        this.name = result.getName();
        this.branch = result.getBranch();
        this.path = result.getPath();
        this.snippet = result.getSnippet();
        this.subtestName = result.getSubtestName();
        this.processedBy = result.getProcessedBy();
        this.tags = result.getTags();
        this.requirements = result.getRequirements();
        this.uid = result.getUid();
        this.revisionNumber = result.getRevisionNumber();
        this.isRight = result.isRight();
        this.created = result.getCreated();
        this.status = result.getStatus();
        this.resultType = result.getResultType();
        this.isStrongMode = result.isStrongMode();
    }

    public TestResultEntity toEntity() {
        return TestResultEntity.builder()
                .id(this.id.toEntityId())
                .autocheckChunkId(this.autocheckChunkId)
                .oldTestId(this.oldTestId)
                .chunkId(ChunkEntity.Id.of(this.storageChunkType, this.storageChunkNumber))
                .links(this.links)
                .metrics(this.metrics)
                .testOutputs(this.testOutputs)
                .name(this.name)
                .branch(this.branch)
                .path(this.path)
                .snippet(this.snippet)
                .subtestName(this.subtestName)
                .processedBy(this.processedBy)
                .tags(this.tags)
                .requirements(this.requirements)
                .uid(this.uid)
                .revisionNumber(this.revisionNumber)
                .isRight(this.isRight)
                .created(this.created)
                .status(this.status)
                .resultType(this.resultType)
                .isStrongMode(this.isStrongMode)
                .owners(ResultOwners.EMPTY)
                .build();
    }

    @Value
    @YTreeObject
    public static class Id {
        @YTreeField(key = "id_checkId")
        long checkId;

        @YTreeField(key = "id_iterationType")
        CheckIteration.IterationType iterationType;

        @YTreeField(key = "id_suiteId")
        UnsignedLong suiteId;

        @YTreeField(key = "id_testId")
        UnsignedLong testId;

        @YTreeField(key = "id_toolchain")
        String toolchain;

        @YTreeField(key = "id_iterationNumber")
        int iterationNumber;

        @YTreeField(key = "id_taskId")
        String taskId;

        @YTreeField(key = "id_partition")
        int partition;

        @YTreeField(key = "id_retryNumber")
        int retryNumber;


        public Id(TestResultEntity.Id entityId) {
            this.checkId = entityId.getCheckId().getId();
            this.iterationType = entityId.getIterationId().getIterationType();
            this.suiteId = UnsignedLong.valueOf(entityId.getSuiteId());
            this.testId = UnsignedLong.valueOf(entityId.getTestId());
            this.toolchain = entityId.getToolchain();
            this.iterationNumber = entityId.getIterationNumber();
            this.taskId = entityId.getTaskId();
            this.partition = entityId.getPartition();
            this.retryNumber = entityId.getRetryNumber();
        }

        public TestResultEntity.Id toEntityId() {
            return new TestResultEntity.Id(
                    CheckEntity.Id.of(this.checkId),
                    this.iterationType.getNumber(),
                    this.suiteId.longValue(),
                    this.testId.longValue(),
                    this.toolchain,
                    this.iterationNumber,
                    this.taskId,
                    this.partition,
                    this.retryNumber
            );
        }
    }
}
