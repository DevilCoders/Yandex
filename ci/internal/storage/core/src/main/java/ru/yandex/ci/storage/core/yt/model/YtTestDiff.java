package ru.yandex.ci.storage.core.yt.model;

import java.time.Instant;

import javax.annotation.Nullable;

import lombok.Value;

import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffStatistics;
import ru.yandex.inside.yt.kosher.impl.ytree.object.NullSerializationStrategy;
import ru.yandex.inside.yt.kosher.impl.ytree.object.annotation.YTreeField;
import ru.yandex.inside.yt.kosher.impl.ytree.object.annotation.YTreeFlattenField;
import ru.yandex.inside.yt.kosher.impl.ytree.object.annotation.YTreeObject;
import ru.yandex.misc.lang.number.UnsignedLong;

@Value
@YTreeObject(nullSerializationStrategy = NullSerializationStrategy.SERIALIZE_NULL_TO_EMPTY)
public class YtTestDiff {
    @YTreeFlattenField
    Id id;

    long autocheckChunkId;

    String name;
    String subtestName;
    String processedBy;
    String strongModeAYaml;
    String oldSuiteId;

    boolean uidChanged;

    String tags;
    String requirements;

    Common.TestDiffType diffType;

    Common.TestStatus left;
    Common.TestStatus right;

    Integer leftIterationNumber;
    Integer rightIterationNumber;

    Integer clonedFromIteration;

    boolean isMuted;
    Boolean isStrongMode;
    boolean isLast;
    Boolean isLaunchable;

    Integer aggregateIdHash;

    Boolean important;

    Instant created;

    TestDiffStatistics statistics;

    @Nullable
    Boolean isOwner;

    Common.ChunkType storageChunkType;
    Integer storageChunkNumber;

    @YTreeObject(nullSerializationStrategy = NullSerializationStrategy.SERIALIZE_NULL_TO_EMPTY)
    public static class Id {
        @YTreeField(key = "id_checkId")
        long checkId;

        @YTreeField(key = "id_iterationType")
        CheckIteration.IterationType iterationType;

        @YTreeField(key = "id_resultType")
        Common.ResultType resultType;

        @YTreeField(key = "id_toolchain")
        String toolchain;

        @YTreeField(key = "id_path")
        String path;

        @YTreeField(key = "id_suiteId")
        UnsignedLong suiteId;

        @YTreeField(key = "id_testId")
        UnsignedLong testId;

        @YTreeField(key = "id_iterationNumber")
        int iterationNumber;

        public Id(TestDiffEntity.Id entityId) {
            this.checkId = entityId.getCheckId().getId();
            this.iterationType = entityId.getIterationId().getIterationType();
            this.resultType = entityId.getResultType();
            this.toolchain = entityId.getToolchain();
            this.path = entityId.getPath();
            this.suiteId = UnsignedLong.valueOf(entityId.getSuiteId());
            this.testId = UnsignedLong.valueOf(entityId.getTestId());
            this.iterationNumber = entityId.getIterationNumber();
        }

        public TestDiffEntity.Id toEntityId() {
            return new TestDiffEntity.Id(
                    CheckEntity.Id.of(checkId),
                    this.iterationType.getNumber(),
                    this.resultType,
                    this.toolchain,
                    this.path,
                    this.suiteId.longValue(),
                    this.testId.longValue(),
                    this.iterationNumber
            );
        }
    }

    public YtTestDiff(TestDiffEntity entity) {
        this.id = new Id(entity.getId());
        this.autocheckChunkId = entity.getAutocheckChunkId();
        //noinspection ConstantConditions
        this.storageChunkType = entity.getChunkId().getChunkType();
        this.storageChunkNumber = entity.getChunkId().getNumber();
        this.name = entity.getName();
        this.subtestName = entity.getSubtestName();
        this.processedBy = entity.getProcessedBy();
        this.tags = entity.getTags();
        this.requirements = entity.getRequirements();
        this.created = entity.getCreated();
        this.isStrongMode = entity.isStrongMode();
        this.strongModeAYaml = entity.getStrongModeAYaml();
        this.oldSuiteId = entity.getOldSuiteId();
        this.uidChanged = entity.isUidChanged();
        this.diffType = entity.getDiffType();
        this.left = entity.getLeft();
        this.right = entity.getRight();
        this.leftIterationNumber = entity.getLeftIterationNumber();
        this.rightIterationNumber = entity.getRightIterationNumber();
        this.clonedFromIteration = entity.getClonedFromIteration();
        this.isMuted = entity.isMuted();
        this.isLast = entity.isLast();
        this.isLaunchable = entity.getIsLaunchable();
        this.aggregateIdHash = entity.getAggregateIdHash();
        this.important = entity.getImportant();
        this.statistics = entity.getStatistics();
        this.isOwner = entity.getIsOwner();
    }

    public TestDiffEntity toEntity() {
        return TestDiffEntity.builder()
                .id(this.id.toEntityId())
                .autocheckChunkId(this.autocheckChunkId)
                .chunkId(ChunkEntity.Id.of(this.storageChunkType, this.storageChunkNumber))
                .name(this.name)
                .subtestName(this.subtestName)
                .processedBy(this.processedBy)
                .tags(this.tags)
                .requirements(this.requirements)
                .created(this.created)
                .isStrongMode(this.isStrongMode)
                .strongModeAYaml(this.strongModeAYaml)
                .oldSuiteId(this.oldSuiteId)
                .uidChanged(this.uidChanged)
                .diffType(this.diffType)
                .left(this.left)
                .right(this.right)
                .leftIterationNumber(this.leftIterationNumber)
                .rightIterationNumber(this.rightIterationNumber)
                .clonedFromIteration(this.clonedFromIteration)
                .isMuted(this.isMuted)
                .isLast(this.isLast)
                .isLaunchable(this.isLaunchable)
                .aggregateIdHash(this.aggregateIdHash)
                .important(this.important)
                .statistics(this.statistics)
                .isOwner(this.isOwner)
                .build();
    }
}
