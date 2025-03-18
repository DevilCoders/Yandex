package ru.yandex.ci.observer.core.db.model.check;

import java.time.Instant;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.Builder;
import lombok.Value;
import lombok.With;
import lombok.extern.slf4j.Slf4j;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.GlobalIndex;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;
import ru.yandex.ci.storage.core.utils.CiHashCode;

@SuppressWarnings({"ReferenceEquality", "BoxedPrimitiveEquality"})
@Value
@Builder(toBuilder = true, builderClassName = "Builder")
@With
@Table(name = "Checks")
@GlobalIndex(name = CheckEntity.IDX_BY_REVISIONS, fields = {"left.revision", "right.revision"})
@GlobalIndex(name = CheckEntity.IDX_BY_STATUS_AND_CREATED, fields = {"status", "created"})
@Slf4j
public class CheckEntity implements Entity<CheckEntity> {
    public static final Long ID_START = 100000000000L;
    public static final int NUMBER_OF_ID_PARTITIONS = 999;

    public static final String IDX_BY_REVISIONS = "IDX_BY_REVISIONS";
    public static final String IDX_BY_STATUS_AND_CREATED = "IDX_BY_STATUS_AND_CREATED";

    CheckEntity.Id id;

    CheckOuterClass.CheckType type;
    Common.CheckStatus status;

    Set<String> tags;

    Long diffSetId;

    StorageRevision left;

    StorageRevision right;

    String author;

    int shardOutPartition;

    @Column(dbType = DbType.TIMESTAMP)
    Instant created;

    @Column(dbType = DbType.TIMESTAMP)
    @Nullable
    Instant completed;

    @Column(dbType = DbType.TIMESTAMP)
    @Nullable
    Instant diffSetEventCreated;

    @Override
    public CheckEntity.Id getId() {
        return id;
    }

    public Optional<Long> getPullRequestId() {
        var branch = ArcBranch.ofString(right.getBranch());
        if (branch.isPr()) {
            return Optional.of(branch.getPullRequestId());
        }

        return Optional.empty();
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<CheckEntity> {
        Long id;

        @Override
        public String toString() {
            return id.toString();
        }

        @Override
        public boolean equals(@Nullable Object o) {
            if (this == o) {
                return true;
            }
            if (o == null || getClass() != o.getClass()) {
                return false;
            }

            Id other = (Id) o;

            return id.equals(other.id);
        }

        @Override
        public int hashCode() {
            return CiHashCode.hashCode(id);
        }

        public static Id of(String value) {
            try {
                return new Id(Long.parseLong(value));
            } catch (NumberFormatException numberFormatException) {
                log.error("Wrong id: {}", value, numberFormatException);
                return new Id(0L);
            }
        }

        public int distribute(int numberOfShardOutPartitions) {
            return (int) ((id / ID_START) % numberOfShardOutPartitions);
        }
    }

    public static class Builder {
        public CheckEntity build() {
            Preconditions.checkNotNull(id, "id is null");
            Preconditions.checkNotNull(left, "left is null");
            Preconditions.checkNotNull(right, "right is null");

            if (Objects.isNull(status)) {
                status = Common.CheckStatus.CREATED;
            }

            if (Objects.isNull(tags)) {
                tags = Set.of();
            }

            if (Objects.isNull(created)) {
                created = Instant.now();
            }

            return new CheckEntity(
                    id,
                    type,
                    status,
                    tags,
                    diffSetId,
                    left,
                    right,
                    author,
                    shardOutPartition,
                    created,
                    completed,
                    diffSetEventCreated
            );
        }
    }
}
