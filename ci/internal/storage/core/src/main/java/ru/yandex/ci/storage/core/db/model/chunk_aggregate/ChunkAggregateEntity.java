package ru.yandex.ci.storage.core.db.model.chunk_aggregate;

import java.time.Instant;
import java.util.HashSet;
import java.util.Set;
import java.util.function.Function;

import javax.annotation.Nullable;

import com.google.common.hash.Hashing;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Data;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.util.HostnameUtils;

@Value
@Builder(toBuilder = true)
@Table(name = "ChunkAggregates")
public class ChunkAggregateEntity implements Entity<ChunkAggregateEntity> {
    Id id;

    Common.ChunkAggregateState state;

    ChunkAggregateStatistics statistics;

    @Column(dbType = DbType.TIMESTAMP)
    Instant created;

    @Column(dbType = DbType.TIMESTAMP)
    @Nullable
    Instant finished;

    @Nullable
    Set<String> processedBy;

    public static ChunkAggregateEntity create(ChunkAggregateEntity.Id id) {
        return ChunkAggregateEntity.builder()
                .id(id)
                .statistics(ChunkAggregateStatistics.EMPTY)
                .created(Instant.now())
                .state(Common.ChunkAggregateState.CAS_RUNNING)
                .processedBy(Set.of(HostnameUtils.getShortHostname()))
                .build();
    }

    @Override
    public Id getId() {
        return id;
    }

    public Set<String> getProcessedBy() {
        return processedBy == null ? Set.of() : processedBy;
    }

    public ChunkAggregateEntityMutable toMutable() {
        var updatedProcessedBy = new HashSet<>(getProcessedBy());
        updatedProcessedBy.add(HostnameUtils.getShortHostname());

        return new ChunkAggregateEntityMutable(
                id,
                state,
                statistics.toMutable(),
                created,
                finished,
                updatedProcessedBy
        );
    }

    public boolean isFinished() {
        return !state.equals(Common.ChunkAggregateState.CAS_RUNNING);
    }

    public ChunkAggregateEntity finish(Common.ChunkAggregateState state) {
        return this.toBuilder()
                .finished(Instant.now())
                .state(state)
                .build();
    }

    @SuppressWarnings("UnstableApiUsage")
    @Value
    public static class Id implements Entity.Id<ChunkAggregateEntity> {
        CheckIterationEntity.Id iterationId;
        ChunkEntity.Id chunkId;

        public Id toIterationMetaId() {
            return new ChunkAggregateEntity.Id(iterationId.toMetaId(), chunkId);
        }

        public Id toFirstIterationId() {
            return new ChunkAggregateEntity.Id(iterationId.toIterationId(1), chunkId);
        }

        public Id toIterationId(int number) {
            return new ChunkAggregateEntity.Id(iterationId.toIterationId(number), chunkId);
        }

        @Override
        public String toString() {
            return String.format("[%s/%s]", iterationId, chunkId);
        }

        public int externalHashCode() {
            return Hashing.sipHash24().newHasher()
                    .putLong(iterationId.getCheckId().getId())
                    .putInt(iterationId.getIterationTypeNumber())
                    .putInt(chunkId.getChunkType().getNumber())
                    .putInt(chunkId.getNumber())
                    .hash().asInt();
        }
    }

    @Data
    @AllArgsConstructor
    public static class ChunkAggregateEntityMutable {
        Id id;

        Common.ChunkAggregateState state;

        ChunkAggregateStatistics.ChunkAggregateStatisticsMutable statistics;

        Instant created;

        Instant finished;

        @Nullable
        Set<String> processedBy;

        public ChunkAggregateEntity toImmutable() {
            return new ChunkAggregateEntity(
                    id,
                    state,
                    statistics.toImmutable(),
                    created,
                    finished,
                    processedBy
            );
        }

        public boolean isFinished() {
            return !state.equals(Common.ChunkAggregateState.CAS_RUNNING);
        }
    }

    public static Function<ChunkAggregateEntity.Id, ChunkAggregateEntity> defaultProvider() {
        return ChunkAggregateEntity::create;
    }
}
