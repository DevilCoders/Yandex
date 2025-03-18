package ru.yandex.ci.storage.core.cache;

import java.util.Collection;

import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;

public interface IterationsCache extends EntityCache<CheckIterationEntity.Id, CheckIterationEntity> {
    Collection<CheckIterationEntity> getFreshForCheck(CheckEntity.Id checkId);

    interface Modifiable extends IterationsCache,
            EntityCache.Modifiable<CheckIterationEntity.Id, CheckIterationEntity> {
        void processIterationFinished(CheckIterationEntity.Id iterationId);
    }

    interface WithModificationSupport extends IterationsCache,
            EntityCache.ModificationSupport<CheckIterationEntity.Id, CheckIterationEntity> {
    }

    interface WithCommitSupport extends Modifiable,
            EntityCache.Modifiable.CacheWithCommitSupport<CheckIterationEntity.Id, CheckIterationEntity> {
    }
}
