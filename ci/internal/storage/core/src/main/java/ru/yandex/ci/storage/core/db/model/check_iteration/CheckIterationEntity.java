package ru.yandex.ci.storage.core.db.model.check_iteration;

import java.time.Instant;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonSetter;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.google.common.base.Preconditions;
import com.google.common.hash.Hashing;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.GlobalIndex;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.common.temporal.BaseTemporalWorkflow;
import ru.yandex.ci.storage.core.CheckIteration.IterationType;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.Common.CheckStatus;
import ru.yandex.ci.storage.core.check.CreateIterationParams;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check.SuspiciousAlert;
import ru.yandex.ci.storage.core.db.model.check_iteration.IterationStatistics.ToolchainStatisticsMutable;
import ru.yandex.ci.storage.core.db.model.check_task.ExpectedTask;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.util.HostnameUtils;

@SuppressWarnings({"ReferenceEquality", "BoxedPrimitiveEquality"})
@Value
@Builder(toBuilder = true, buildMethodName = "buildInternal")
@With
@Table(name = "CheckIterations")
@GlobalIndex(name = CheckIterationEntity.IDX_BY_STATUS_AND_CREATED, fields = {"status", "created"})
@GlobalIndex(name = CheckIterationEntity.IDX_BY_TEST_ENV_ID, fields = "testenvId")
public class CheckIterationEntity implements Entity<CheckIterationEntity> {
    public static final String IDX_BY_STATUS_AND_CREATED = "IDX_BY_STATUS_AND_CREATED";
    public static final String IDX_BY_TEST_ENV_ID = "IDX_BY_TEST_ENV_ID";

    Id id;

    @Nullable
    Common.CheckTaskType tasksType;

    @Column(dbType = DbType.TIMESTAMP)
    Instant created;

    @Nullable
    @Column(dbType = DbType.TIMESTAMP)
    Instant start;

    @Nullable
    @Column(dbType = DbType.TIMESTAMP)
    Instant finish;

    @Nullable
    @Column(dbType = DbType.TIMESTAMP)
    Instant allTasksFinished;

    CheckStatus status;

    IterationStatistics statistics;

    @Column(flatten = false)
    IterationInfo info;

    @Nullable
    String testenvId;

    Set<String> shardOutProcessedBy;

    @Nullable
    Boolean useImportantDiffIndex;

    @Nullable
    Boolean useSuiteDiffIndex;

    @Nullable
    Set<ExpectedTask> expectedTasks;

    @Nullable
    Set<ExpectedTask> registeredExpectedTasks;

    @Nullable
    Integer numberOfTasks;

    @Nullable
    Integer numberOfCompletedTasks;

    @Column(flatten = false)
    @Nullable
    TestTypeStatistics testTypeStatistics;

    @Nullable
    Map<String, String> attributes;

    @Nullable
    Map<Common.CheckTaskType, CheckTaskStatus> checkTaskStatuses;

    @Nullable
    String startedBy; // User started this task (fill it for manual runs)

    @Nullable
    Long chunkShift;

    @Nullable
    Collection<SuspiciousAlert> suspiciousAlerts;

    @Override
    public Id getId() {
        return id;
    }

    public String getTestenvId() {
        return testenvId == null ? "" : testenvId;
    }

    public TestTypeStatistics getTestTypeStatistics() {
        return testTypeStatistics == null ? TestTypeStatistics.EMPTY : testTypeStatistics;
    }

    public Common.CheckTaskType getTasksType() {
        return tasksType == null ? Common.CheckTaskType.CTT_AUTOCHECK : tasksType;
    }

    public Map<String, String> getAttributes() {
        return attributes == null ? Map.of() : attributes;
    }

    public String getAttribute(Common.StorageAttribute attribute) {
        return getAttributes().getOrDefault(attribute.name(), "");
    }

    public Integer getNumberOfCompletedTasks() {
        return numberOfCompletedTasks == null ? 0 : numberOfCompletedTasks;
    }

    public boolean isUseImportantDiffIndex() {
        return useImportantDiffIndex != null && useImportantDiffIndex;
    }

    public Boolean isUseSuiteDiffIndex() {
        return useSuiteDiffIndex != null && useSuiteDiffIndex;
    }

    public Set<ExpectedTask> getExpectedTasks() {
        return expectedTasks == null ? Set.of() : expectedTasks;
    }

    public int getNumberOfTasks() {
        return numberOfTasks == null ? 0 : numberOfTasks;
    }

    public long getChunkShift() {
        return chunkShift == null ? 0 : chunkShift;
    }

    public Set<ExpectedTask> getRegisteredExpectedTasks() {
        return registeredExpectedTasks == null ? Set.of() : registeredExpectedTasks;
    }

    public Map<Common.CheckTaskType, CheckTaskStatus> getCheckTaskStatuses() {
        return checkTaskStatuses == null ? Map.of() : checkTaskStatuses;
    }

    public CheckTaskStatus getCheckTaskStatus(Common.CheckTaskType checkTaskType) {
        return getCheckTaskStatuses().getOrDefault(checkTaskType, CheckTaskStatus.NOT_REQUIRED);
    }

    public Collection<SuspiciousAlert> getSuspiciousAlerts() {
        return suspiciousAlerts == null ? List.of() : suspiciousAlerts;
    }

    public static CheckIterationEntity create(Id id, CreateIterationParams params) {
        return CheckIterationEntity.builder()
                .id(id)
                .statistics(IterationStatistics.EMPTY)
                .created(Instant.now())
                .status(CheckStatus.CREATED)
                .info(params.getInfo())
                .expectedTasks(params.getExpectedTasks())
                .useImportantDiffIndex(params.isUseImportantDiffIndex())
                .useSuiteDiffIndex(params.isUseSuiteDiffIndex())
                .testenvId(params.getInfo().getTestenvCheckId())
                .attributes(
                        params.getAttributes().entrySet().stream()
                                .collect(Collectors.toMap(e -> e.getKey().name(), Map.Entry::getValue))
                )
                .tasksType(params.getTasksType())
                .startedBy(params.getStartedBy())
                .chunkShift(params.getChunkShift())
                .build();
    }

    public CheckIterationEntity run() {
        var builder = this.toBuilder();
        if (start == null) {
            builder.start(Instant.now());
        }

        var processedBy = new HashSet<>(shardOutProcessedBy);
        processedBy.add(HostnameUtils.getShortHostname());

        return builder
                .status(CheckStatus.RUNNING)
                .shardOutProcessedBy(processedBy)
                .build();
    }

    public CheckIterationEntity complete(CheckStatus status) {
        return this.toBuilder()
                .info(info.toBuilder().progress(100).build())
                .status(status)
                .finish(Instant.now())
                .build();
    }

    public boolean isAllTasksCompleted() {
        return getNumberOfTasks() == getNumberOfCompletedTasks();
    }

    public CheckIterationEntity setAttribute(Common.StorageAttribute attribute, String value) {
        var updatedAttributes = new HashMap<>(this.getAttributes());
        updatedAttributes.put(attribute.name(), value);
        return this.withAttributes(updatedAttributes);
    }

    public String getAttributeOrDefault(Common.StorageAttribute attribute, String defaultValue) {
        return getAttributes().getOrDefault(attribute.name(), defaultValue);
    }

    public boolean isHeavy() {
        return this.id.isHeavy();
    }

    public CheckIterationEntity updateCheckTaskStatuses(Map<Common.CheckTaskType, CheckTaskStatus> statuses) {
        var map = new HashMap<>(this.getCheckTaskStatuses());
        map.putAll(statuses);
        return withCheckTaskStatuses(map);
    }

    public boolean isAllRequiredTasksRegistered() {
        return getExpectedTasks().size() <= getRegisteredExpectedTasks().size();
    }

    public CheckIterationEntity addExpectedTaskRegistered(ExpectedTask task) {
        var value = new HashSet<>(this.getRegisteredExpectedTasks());
        value.add(task);
        return this.toBuilder()
                .registeredExpectedTasks(value)
                .build();
    }

    @SuppressWarnings("UnstableApiUsage")
    @Value
    @AllArgsConstructor
    @lombok.Builder
    @JsonDeserialize(builder = Id.Builder.class)
    public static class Id implements Entity.Id<CheckIterationEntity>, BaseTemporalWorkflow.Id {
        CheckEntity.Id checkId;

        @Column(dbType = DbType.UINT8)
        int iterationType;

        @Column(dbType = DbType.UINT8)
        int number;

        public IterationType getIterationType() {
            return IterationType.forNumber(iterationType);
        }

        @JsonIgnore
        public int getIterationTypeNumber() {
            return iterationType;
        }

        public Id toMetaId() {
            return new Id(checkId, iterationType, 0);
        }

        public Id toIterationId(int number) {
            return new Id(checkId, iterationType, number);
        }

        @JsonIgnore
        public boolean isMetaIteration() {
            return number == 0;
        }

        @JsonIgnore
        public boolean isHeavy() {
            return iterationType == IterationType.HEAVY.getNumber();
        }

        @JsonIgnore
        public boolean isFirstFullIteration() {
            return iterationType == IterationType.FULL.getNumber() && number == 1;
        }

        public static Id of(CheckEntity.Id checkId, IterationType iterationType, int number) {
            return new Id(checkId, iterationType.getNumber(), number);
        }

        @Override
        public String toString() {
            return String.format("[%s]", toPureString());
        }

        public String toPureString() {
            return String.format("%s/%s/%d", checkId, IterationType.forNumber(iterationType), number);
        }

        @Override
        public String getTemporalWorkflowId() {
            return toPureString();
        }

        public long generateChunkShift() {
            return Hashing.sipHash24().newHasher()
                    .putLong(this.checkId.getId())
                    .putInt(this.iterationType)
                    .hash().asLong();
        }

        public static class Builder {
            public CheckIterationEntity.Id.Builder iterationType(IterationType iterationType) {
                return iterationType(iterationType.getNumber());
            }

            @JsonSetter
            public CheckIterationEntity.Id.Builder iterationType(String iterationType) {
                try {
                    return iterationType(Integer.parseInt(iterationType));
                } catch (NumberFormatException e) {
                    return iterationType(IterationType.valueOf(iterationType));
                }
            }

            public CheckIterationEntity.Id.Builder iterationType(int iterationType) {
                this.iterationType = iterationType;
                return this;
            }
        }
    }

    public static class Builder {
        public CheckIterationEntity build() {
            Preconditions.checkNotNull(id, "id is null");

            if (Objects.isNull(status)) {
                status = CheckStatus.CREATED;
            }

            if (Objects.isNull(created)) {
                created = Instant.now();
            }

            if (Objects.isNull(statistics)) {
                statistics = IterationStatistics.EMPTY;
            }

            if (Objects.isNull(info)) {
                info = IterationInfo.EMPTY;
            }

            if (Objects.isNull(shardOutProcessedBy)) {
                shardOutProcessedBy = Set.of();
            }

            if (Objects.isNull(numberOfCompletedTasks)) {
                numberOfCompletedTasks = 0;
            }

            if (Objects.isNull(numberOfTasks)) {
                numberOfTasks = 0;
            }

            return buildInternal();
        }

        public int getNumberOfTasks() {
            return numberOfTasks == null ? 0 : numberOfTasks;
        }

        public int getNumberOfCompletedTasks() {
            return numberOfCompletedTasks == null ? 0 : numberOfCompletedTasks;
        }

        public Set<ExpectedTask> getExpectedTasks() {
            return expectedTasks == null ? Set.of() : expectedTasks;
        }

        public TestTypeStatistics getTestTypeStatistics() {
            return testTypeStatistics == null ? TestTypeStatistics.EMPTY : testTypeStatistics;
        }

        public IterationInfo getInfo() {
            return info;
        }

        public Id getId() {
            return id;
        }
    }

    public static CheckIterationEntity sumOf(
            CheckIterationEntity iteration, Collection<ChunkAggregateEntity> parts
    ) {
        var statistics = iteration.getStatistics().toMutable();
        var toolchains = sumToolchains(parts, statistics.getToolchains().size());
        statistics.setToolchains(toolchains);
        return iteration.withStatistics(statistics.toImmutable());
    }

    private static Map<String, ToolchainStatisticsMutable> sumToolchains(
            Collection<ChunkAggregateEntity> parts, int initialCapacity
    ) {

        var toolchains = new HashMap<String, ToolchainStatisticsMutable>(initialCapacity);

        for (var part : parts) {
            for (var toolchainEntry : part.getStatistics().getToolchains().entrySet()) {
                var toolchainName = toolchainEntry.getKey();
                var toolchain = toolchains.computeIfAbsent(toolchainName, t -> ToolchainStatisticsMutable.newEmpty());
                toolchain.getExtendedStatistics().add(toolchainEntry.getValue().getExtended().toMutable(), 1);
                toolchain.getMainStatistics().add(toolchainEntry.getValue().getMain().toMutable(), 1);
            }
        }
        return toolchains;
    }

    @Override
    public String toString() {
        return "CheckIterationEntity{id=%s, numberOfTasks=%d}".formatted(id, numberOfTasks);
    }
}
