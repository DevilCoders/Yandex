package ru.yandex.ci.storage.core.cache.impl;

import java.time.Duration;
import java.util.Optional;

import com.google.common.cache.Cache;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.cache.SettingsCache;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.settings.SettingsEntity;

public class SettingsCacheImpl extends EntityCacheImpl<SettingsEntity.Id, SettingsEntity, CiStorageDb>
        implements SettingsCache, SettingsCache.WithModificationSupport {

    public SettingsCacheImpl(CiStorageDb db, MeterRegistry meterRegistry) {
        super(
                SettingsEntity.class,
                EntityCacheImpl.createDefault(1, Duration.ofMinutes(1), null),
                db,
                meterRegistry,
                "settings",
                id -> SettingsEntity.EMTPY
        );

        GuavaCacheMetrics.monitor(meterRegistry, cache, this.cacheName);
    }

    @Override
    protected KikimrTableCi<SettingsEntity> getTable() {
        return this.db.settings();
    }

    @Override
    public WithCommitSupport toModifiable(int maxNumberOfWrites) {
        return new Modifiable(this, this.cache, maxNumberOfWrites);
    }

    @Override
    public SettingsEntity get() {
        return this.getOrDefault(SettingsEntity.ID);
    }

    public static class Modifiable extends ModifiableEntityCacheImpl<SettingsEntity.Id, SettingsEntity, CiStorageDb>
            implements WithCommitSupport {

        public Modifiable(SettingsCacheImpl baseImpl, Cache<SettingsEntity.Id, Optional<SettingsEntity>> cache,
                          int maxNumberOfWrites) {
            super(baseImpl, cache, maxNumberOfWrites);
        }

        @Override
        public SettingsEntity get() {
            return ((SettingsCacheImpl) this.baseImpl).get();
        }
    }
}
