package ru.yandex.ci.storage.core.cache;


import ru.yandex.ci.storage.core.db.model.settings.SettingsEntity;

public interface SettingsCache extends EntityCache<SettingsEntity.Id, SettingsEntity> {
    SettingsEntity get();

    interface Modifiable extends SettingsCache, EntityCache.Modifiable<SettingsEntity.Id, SettingsEntity> {
    }

    interface WithModificationSupport extends SettingsCache, EntityCache.ModificationSupport<SettingsEntity.Id,
            SettingsEntity> {
    }

    interface WithCommitSupport extends Modifiable, EntityCache.Modifiable.CacheWithCommitSupport<SettingsEntity.Id,
            SettingsEntity> {
    }
}
