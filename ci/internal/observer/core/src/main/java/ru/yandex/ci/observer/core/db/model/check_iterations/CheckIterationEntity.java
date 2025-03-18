package ru.yandex.ci.observer.core.db.model.check_iterations;

import java.time.Instant;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;

import javax.annotation.Nullable;

import com.yandex.ydb.table.settings.AutoPartitioningPolicy;
import com.yandex.ydb.table.settings.PartitioningPolicy;
import com.yandex.ydb.table.settings.PartitioningSettings;
import lombok.Builder;
import lombok.Value;
import lombok.With;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.GlobalIndex;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import yandex.cloud.repository.kikimr.client.YdbTableHint;

import ru.yandex.ci.observer.core.db.model.check.CheckEntity;
import ru.yandex.ci.observer.core.db.model.traces.DurationStages;
import ru.yandex.ci.observer.core.db.model.traces.TimestampedTraceStages;
import ru.yandex.ci.observer.core.utils.StageAggregationUtils;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckIteration.IterationType;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common.CheckStatus;
import ru.yandex.ci.storage.core.db.model.check_iteration.TechnicalStatistics;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;
import ru.yandex.ci.ydb.Persisted;

import static ru.yandex.ci.observer.core.utils.StageAggregationUtils.PRE_CREATION_STAGE;

@SuppressWarnings({"ReferenceEquality", "BoxedPrimitiveEquality"})
@Value
@Builder(toBuilder = true, builderClassName = "Builder")
@With
@Table(name = "CheckIterations")
@GlobalIndex(name = CheckIterationEntity.IDX_BY_CREATED, fields = "created")
@GlobalIndex(name = CheckIterationEntity.IDX_BY_RIGHT_REVISION_TIMESTAMP, fields = "rightRevisionTimestamp")
@GlobalIndex(name = CheckIterationEntity.IDX_BY_FINISH, fields = "finish")
@GlobalIndex(name = CheckIterationEntity.IDX_BY_STATUS, fields = "status")
public class CheckIterationEntity implements Entity<CheckIterationEntity> {
    public static final String IDX_BY_CREATED = "IDX_BY_CREATED";
    public static final String IDX_BY_RIGHT_REVISION_TIMESTAMP = "IDX_BY_RIGHT_REVISION_TIMESTAMP";
    public static final String IDX_BY_FINISH = "IDX_BY_FINISH";
    public static final String IDX_BY_STATUS = "IDX_BY_STATUS";
    public static final Set<CheckIteration.IterationType> ITERATION_TYPES_WITH_RECHECK = Set.of(
            CheckIteration.IterationType.FAST, CheckIteration.IterationType.FULL
    );

    @SuppressWarnings("UnusedVariable")
    private static YdbTableHint ydbTableHint;

    static {
        ydbTableHint = new YdbTableHint(
                new PartitioningPolicy().setAutoPartitioning(AutoPartitioningPolicy.AUTO_SPLIT_MERGE),
                new PartitioningSettings()
                        .setPartitioningBySize(true)
                        .setMaxPartitionsCount(20),
                YdbTableHint.TablePreset.LOG_LZ4
        );
    }

    Id id;

    @Column(dbType = DbType.TIMESTAMP)
    Instant created;

    @Nullable
    @Column(dbType = DbType.TIMESTAMP)
    Instant finish;

    CheckOuterClass.CheckType checkType;
    String author;

    @Nullable
    @Column(dbType = DbType.JSON, flatten = false)
    StorageRevision left;

    @Nullable
    @Column(dbType = DbType.JSON, flatten = false)
    StorageRevision right;

    @Nullable   // old values
    @Column(dbType = DbType.TIMESTAMP)
    Instant rightRevisionTimestamp;

    CheckStatus status;
    TechnicalStatistics statistics;

    boolean pessimized;
    @Nullable   // for old values
    Boolean stressTest;

    String advisedPool;
    @Nullable
    String testenvId;

    @Column(dbType = DbType.JSON, flatten = false)
    @Nullable
    Map<String, String> checkRelatedLinks;  // map<link_name, link>

    @Column(dbType = DbType.JSON, flatten = false)
    List<CheckTaskKey> expectedCheckTasks;

    @Column(dbType = DbType.JSON, flatten = false)
    DurationStages stagesAggregation;

    @Column(dbType = DbType.JSON, flatten = false)
    @Nullable
    TimestampedTraceStages timestampedStagesAggregation;

    @Nullable
    Integer parentIterationNumber;
    boolean hasUnfinishedRechecks;
    Set<Integer> unfinishedRecheckIterationNumbers;

    @Column(dbType = DbType.JSON, flatten = false)
    Map<Integer, CheckIterationNumberStagesAggregation> recheckAggregationsByNumber;
    @Nullable
    Long totalDurationSeconds;
    @Nullable
    CheckStatus finalStatus;

    @Column(dbType = DbType.TIMESTAMP)
    @Nullable
    Instant diffSetEventCreated;

    @Override
    public Id getId() {
        return id;
    }

    public Map<String, String> getCheckRelatedLinks() {
        return Objects.requireNonNullElse(checkRelatedLinks, Map.of());
    }

    public boolean getStressTest() {
        return Boolean.TRUE.equals(stressTest);
    }

    @Value
    public static class Id implements Entity.Id<CheckIterationEntity> {
        CheckEntity.Id checkId;
        IterationType iterType;
        int number;

        @Override
        public String toString() {
            return String.format("%s/%s/%d", checkId, iterType, number);
        }
    }

    public static class Builder {

        public Builder right(@Nullable StorageRevision right) {
            this.right = right;
            if (right != null) {
                this.rightRevisionTimestamp = right.getTimestamp();
            }
            return this;
        }

        private Builder rightRevisionTimestamp(@Nullable Instant rightRevisionTimestamp) {
            this.rightRevisionTimestamp = rightRevisionTimestamp;
            return this;
        }

        public Builder mergeCheckRelatedLinks(Map<String, String> checkRelatedLinks) {
            var result = new HashMap<>(Objects.requireNonNullElse(checkRelatedLinks, Map.of()));
            result.putAll(this.checkRelatedLinks);  // old values override new values
            this.checkRelatedLinks = result;
            return this;
        }

        public CheckIterationEntity build() {
            if (stagesAggregation == null) {
                stagesAggregation = initStagesAggregation(checkType, created, diffSetEventCreated,
                        rightRevisionTimestamp);
            }

            return new CheckIterationEntity(
                    Objects.requireNonNull(id, "id is null"),
                    Objects.requireNonNull(created, "created time is null"),
                    finish,
                    checkType,
                    author,
                    left,
                    right,
                    rightRevisionTimestamp,
                    Objects.requireNonNullElse(status, CheckStatus.CREATED),
                    Objects.requireNonNullElse(statistics, TechnicalStatistics.EMPTY),
                    pessimized,
                    Objects.requireNonNullElse(stressTest, false),
                    advisedPool,
                    testenvId,
                    Objects.requireNonNullElseGet(checkRelatedLinks, HashMap::new),
                    Objects.requireNonNullElse(expectedCheckTasks, List.of()),
                    stagesAggregation,
                    Objects.requireNonNullElseGet(timestampedStagesAggregation, TimestampedTraceStages::new),
                    parentIterationNumber,
                    hasUnfinishedRechecks,
                    Objects.requireNonNullElse(unfinishedRecheckIterationNumbers, Set.of()),
                    Objects.requireNonNullElseGet(recheckAggregationsByNumber, HashMap::new),
                    Objects.requireNonNullElse(totalDurationSeconds, 0L),
                    finalStatus,
                    diffSetEventCreated
            );
        }

        private static DurationStages initStagesAggregation(
                CheckOuterClass.CheckType checkType,
                Instant created,
                @Nullable Instant diffSetEventCreated,
                @Nullable Instant rightRevisionTimestamp
        ) {
            var stagesAggregation = new DurationStages();

            var preCreationStartedAt = StageAggregationUtils.getPreCreationStartedAt(checkType, created,
                    diffSetEventCreated, rightRevisionTimestamp);

            long preCreationDurationSeconds = created.getEpochSecond() - preCreationStartedAt.getEpochSecond();
            stagesAggregation.putStageDuration(PRE_CREATION_STAGE, Math.max(0, preCreationDurationSeconds));
            return stagesAggregation;
        }
    }

    @Value
    @Persisted
    public static class CheckTaskKey {
        String jobName;
        boolean right;
    }

}
