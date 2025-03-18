package ru.yandex.ci.storage.core.proto;

import java.time.Instant;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.Random;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Collectors;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.flow.CiActionReference;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.storage.api.StorageApi;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckIteration.IterationType;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.CheckOuterClass.CheckType;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.Common.CheckStatus;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.ExtendedStageStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.ExtendedStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.IterationInfo;
import ru.yandex.ci.storage.core.db.model.check_iteration.IterationStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.MainStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.StageStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.StrongModePolicy;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateStatistics;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;

import static org.assertj.core.api.Assertions.assertThat;

public class CheckProtoMappersTest {
    @Test
    public void convertsCheck() {
        var entity = getCheck();
        var protoCheck = CheckProtoMappers.toProtoCheck(entity);
        var converted = CheckProtoMappers.toCheck(protoCheck);
        assertThat(converted).isEqualTo(entity);
    }

    @Test
    public void convertsIteration() {
        var entity = getIteration();

        var protoIteration = CheckProtoMappers.toProtoIteration(entity);
        var converted = CheckProtoMappers.toCheckIteration(protoIteration);
        assertThat(converted)
                .usingRecursiveComparison()
                .isEqualTo(entity);
    }

    @Test
    public void convertsTask() {
        var entity = CheckTaskEntity.builder()
                .id(
                        new CheckTaskEntity.Id(
                                CheckIterationEntity.Id.of(CheckEntity.Id.of(1L), IterationType.FULL, 1),
                                "task-id"
                        )
                )
                .right(true)
                .jobName("job_name")
                .completedPartitions(Set.of())
                .numberOfPartitions(2)
                .status(CheckStatus.CREATED)
                .created(Instant.now())
                .build();

        var protoTask = CheckProtoMappers.toProtoCheckTask(entity);
        var converted = CheckProtoMappers.toCheckTask(protoTask);
        assertThat(converted).isEqualTo(entity);
    }

    @Test
    public void convertsChunkAggregate() {
        var random = new Random(1); // same sequence every run
        var statistics = new ChunkAggregateStatistics(
                Map.of(
                        "linux",
                        new ChunkAggregateStatistics.ToolchainStatistics(
                                createEventStatistics(random),
                                ExtendedStatistics.builder()
                                        .added(createExtendedStageStatistics(random))
                                        .deleted(createExtendedStageStatistics(random))
                                        .timeout(createExtendedStageStatistics(random))
                                        .flaky(createExtendedStageStatistics(random))
                                        .muted(createExtendedStageStatistics(random))
                                        .internal(createExtendedStageStatistics(random))
                                        .build()
                        )
                )

        );

        var aggregate = ChunkAggregateEntity.builder()
                .id(new ChunkAggregateEntity.Id(
                        getIteration().getId(), ChunkEntity.Id.of(Common.ChunkType.CT_SMALL_TEST, 1)
                ))
                .state(Common.ChunkAggregateState.CAS_RUNNING)
                .statistics(statistics)
                .created(Instant.EPOCH)
                .finished(null)
                .processedBy(Set.of())
                .build();

        var protoAggregate = CheckProtoMappers.toProtoAggregate(aggregate);
        var converted = CheckProtoMappers.toAggregate(protoAggregate);

        assertThat(converted).isEqualTo(aggregate);
    }

    private ExtendedStageStatistics createExtendedStageStatistics(Random random) {
        var max = 100;
        return new ExtendedStageStatistics(
                random.nextInt(max), random.nextInt(max), random.nextInt(max)
        );
    }

    private StageStatistics createStageStatistics(Random random) {
        var max = 100;
        return new StageStatistics(
                random.nextInt(max), random.nextInt(max), random.nextInt(max),
                random.nextInt(max), random.nextInt(max), random.nextInt(max),
                random.nextInt(max), random.nextInt(max), random.nextInt(max),
                random.nextInt(max), random.nextInt(max)
        );
    }

    private MainStatistics createEventStatistics(Random random) {
        return new MainStatistics(
                createStageStatistics(random), createStageStatistics(random),
                createStageStatistics(random), createStageStatistics(random),
                createStageStatistics(random), createStageStatistics(random),
                createStageStatistics(random), createStageStatistics(random),
                createStageStatistics(random)
        );
    }

    private CheckEntity getCheck() {
        return CheckEntity.builder()
                .id(CheckEntity.Id.of(1L))
                .left(new StorageRevision("left_branch", "left_revision", 1, Instant.EPOCH))
                .right(new StorageRevision("right_branch", "right_revision", 2, Instant.EPOCH))
                .author("author")
                .tags(Set.of("tag1", "tag2"))
                .diffSetId(1L)
                .type(CheckType.TRUNK_POST_COMMIT)
                .created(Instant.ofEpochSecond(1611651321))
                .diffSetEventCreated(Instant.EPOCH)
                .build();
    }

    private CheckIterationEntity getIteration() {
        return CheckIterationEntity.builder()
                .id(CheckIterationEntity.Id.of(CheckEntity.Id.of(1L), IterationType.FULL, 1))
                .status(CheckStatus.CREATED)
                .created(Instant.ofEpochSecond(1611651321))
                .statistics(IterationStatistics.EMPTY)
                .info(IterationInfo.EMPTY.toBuilder()
                        .testenvCheckId("te")
                        .ciActionReferences(List.of(
                                new CiActionReference(
                                        new FlowFullId("test-dir", "test-id"), 1
                                )
                        ))
                        .build()
                )
                .testenvId("te")
                .useImportantDiffIndex(true)
                .useSuiteDiffIndex(true)
                .build();
    }

    @Test
    void toStrongModePolicy() {
        var result = Arrays.stream(CheckIteration.StrongModePolicy.values())
                .filter(it -> it != CheckIteration.StrongModePolicy.UNRECOGNIZED)
                .collect(Collectors.toMap(
                        Function.identity(),
                        CheckProtoMappers::toStrongModePolicy
                ));
        assertThat(result).isEqualTo(Map.of(
                CheckIteration.StrongModePolicy.BY_A_YAML, StrongModePolicy.BY_A_YAML,
                CheckIteration.StrongModePolicy.FORCE_ON, StrongModePolicy.FORCE_ON,
                CheckIteration.StrongModePolicy.FORCE_OFF, StrongModePolicy.FORCE_OFF
        ));
    }

    @Test
    void toProtoStrongModePolicy() {
        var result = Arrays.stream(StrongModePolicy.values())
                .collect(Collectors.toMap(
                        Function.identity(),
                        CheckProtoMappers::toProtoStrongModePolicy
                ));
        assertThat(result).isEqualTo(Map.of(
                StrongModePolicy.BY_A_YAML, CheckIteration.StrongModePolicy.BY_A_YAML,
                StrongModePolicy.FORCE_ON, CheckIteration.StrongModePolicy.FORCE_ON,
                StrongModePolicy.FORCE_OFF, CheckIteration.StrongModePolicy.FORCE_OFF
        ));
    }

    @Test
    void toCheck() {
        var result = CheckProtoMappers.toCheck(StorageApi.RegisterCheckRequest.newBuilder()
                .setDiffSetId(1)
                .setOwner("owner")
                .addAllTags(List.of("tag1", "tag2"))
                .setLeftRevision(CheckProtoMappers.toProtoRevision(StorageRevision.from(
                        "trunk", TestData.TRUNK_COMMIT_2
                )))
                .setRightRevision(CheckProtoMappers.toProtoRevision(StorageRevision.from(
                        "trunk", TestData.TRUNK_COMMIT_3
                )))
                .setInfo(CheckOuterClass.CheckInfo.getDefaultInstance())
                .setReportStatusToArcanum(true)
                .build()
        );
        assertThat(result)
                .usingRecursiveComparison()
                .ignoringFields("created")
                .isEqualTo(CheckEntity.builder()
                        .id(CheckEntity.Id.of(0L))
                        .diffSetId(1L)
                        .author("owner")
                        .left(new StorageRevision("trunk", "r2", 2, TestData.COMMIT_DATE))
                        .right(new StorageRevision("trunk", "r3", 3, TestData.COMMIT_DATE))
                        .tags(Set.of("tag1", "tag2"))
                        .status(CheckStatus.CREATED)
                        .type(CheckType.TRUNK_POST_COMMIT)
                        .autostartLargeTests(List.of())
                        .reportStatusToArcanum(true)
                        .diffSetEventCreated(Instant.EPOCH)
                        .attributes(Map.of(
                                Common.StorageAttribute.SA_STRESS_TEST, "false",
                                Common.StorageAttribute.SA_NOTIFICATIONS_DISABLED, "false"
                        ))
                        .build()
                );
    }
}
