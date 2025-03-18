package ru.yandex.ci.storage.core.cache;

import ru.yandex.ci.storage.core.db.model.skipped_check.SkippedCheckEntity;

public interface SkippedChecksCache extends EntityCache<SkippedCheckEntity.Id, SkippedCheckEntity> {
    interface Modifiable extends SkippedChecksCache, EntityCache.Modifiable<SkippedCheckEntity.Id, SkippedCheckEntity> {
    }

    interface WithModificationSupport extends SkippedChecksCache,
            EntityCache.ModificationSupport<SkippedCheckEntity.Id, SkippedCheckEntity> {
    }

    interface WithCommitSupport extends Modifiable,
            EntityCache.Modifiable.CacheWithCommitSupport<SkippedCheckEntity.Id, SkippedCheckEntity> {
    }
}
