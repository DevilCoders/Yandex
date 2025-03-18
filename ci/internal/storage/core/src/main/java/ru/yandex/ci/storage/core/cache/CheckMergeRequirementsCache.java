package ru.yandex.ci.storage.core.cache;

import ru.yandex.ci.storage.core.db.model.check_merge_requirements.CheckMergeRequirementsEntity;

public interface CheckMergeRequirementsCache extends EntityCache<
        CheckMergeRequirementsEntity.Id, CheckMergeRequirementsEntity
        > {
    interface Modifiable extends CheckMergeRequirementsCache,
            EntityCache.Modifiable<CheckMergeRequirementsEntity.Id, CheckMergeRequirementsEntity> {
    }

    interface WithModificationSupport extends CheckMergeRequirementsCache,
            EntityCache.ModificationSupport<CheckMergeRequirementsEntity.Id, CheckMergeRequirementsEntity> {
    }

    interface WithCommitSupport extends Modifiable,
            EntityCache.Modifiable.CacheWithCommitSupport<
                    CheckMergeRequirementsEntity.Id, CheckMergeRequirementsEntity
                    > {
    }
}
