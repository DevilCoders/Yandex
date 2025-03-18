package ru.yandex.ci.storage.core.cache;

import java.util.List;

import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;

public interface ChunksGroupedCache extends EntityCache<ChunkEntity.Id, ChunkEntity> {
    List<ChunkEntity> getAll();

    interface Modifiable extends ChunksGroupedCache,
            EntityCache.Modifiable<ChunkEntity.Id, ChunkEntity> {
    }

    interface WithModificationSupport extends ChunksGroupedCache, EntityCache.ModificationSupport<ChunkEntity.Id,
            ChunkEntity> {
    }

    interface WithCommitSupport extends Modifiable, EntityCache.Modifiable.CacheWithCommitSupport<ChunkEntity.Id,
            ChunkEntity> {
    }
}
