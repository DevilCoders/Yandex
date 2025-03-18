package ru.yandex.ci.storage.reader.cache;

import java.util.Collection;
import java.util.Set;

import ru.yandex.ci.storage.core.cache.EntityCache;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;

public interface ChunkAggregatesGroupedCache extends EntityCache<ChunkAggregateEntity.Id, ChunkAggregateEntity> {
    Collection<ChunkAggregateEntity> getByIterationId(CheckIterationEntity.Id iterationId);

    Set<ChunkEntity.Id> getNotCompletedChunkIds(CheckIterationEntity.Id iterationId);

    interface Modifiable extends ChunkAggregatesGroupedCache,
            EntityCache.Modifiable<ChunkAggregateEntity.Id, ChunkAggregateEntity> {
        void invalidate(CheckIterationEntity.Id iterationId);
    }

    interface WithModificationSupport extends ChunkAggregatesGroupedCache,
            EntityCache.ModificationSupport<ChunkAggregateEntity.Id, ChunkAggregateEntity> {
    }

    interface WithCommitSupport extends Modifiable,
            EntityCache.Modifiable.CacheWithCommitSupport<ChunkAggregateEntity.Id, ChunkAggregateEntity> {
    }
}
