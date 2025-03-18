package ru.yandex.ci.observer.core.cache;

import ru.yandex.ci.observer.core.db.model.settings.ObserverSettings;
import ru.yandex.ci.storage.core.cache.EntityCache;

public interface ObserverSettingsCache extends EntityCache<ObserverSettings.Id, ObserverSettings> {
    ObserverSettings get();

    interface Modifiable
            extends ObserverSettingsCache, EntityCache.Modifiable<ObserverSettings.Id, ObserverSettings> {
    }

    interface WithModificationSupport extends ObserverSettingsCache {
        WithCommitSupport toModifiable(int maxNumberOfWrites);
    }

    interface WithCommitSupport extends
            Modifiable, EntityCache.Modifiable.CacheWithCommitSupport<ObserverSettings.Id, ObserverSettings> {
    }
}
