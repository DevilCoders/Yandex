package ru.yandex.ci.storage.shard.cache;

import java.util.Set;

import ru.yandex.ci.storage.core.cache.EntityCache;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;

public interface ChunkAggregatesCache extends EntityCache<ChunkAggregateEntity.Id, ChunkAggregateEntity> {
    interface Modifiable extends ChunkAggregatesCache,
            EntityCache.Modifiable<ChunkAggregateEntity.Id, ChunkAggregateEntity> {
        void invalidate(Set<ChunkEntity.Id> chunks);
    }

    interface WithModificationSupport extends
            ChunkAggregatesCache,
            EntityCache.ModificationSupport<ChunkAggregateEntity.Id, ChunkAggregateEntity> {

    }

    interface WithCommitSupport extends Modifiable,
            EntityCache.Modifiable.CacheWithCommitSupport<ChunkAggregateEntity.Id, ChunkAggregateEntity> {
    }
}
