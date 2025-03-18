package ru.yandex.ci.observer.core.cache;

import java.time.Duration;
import java.util.Optional;

import com.google.common.cache.Cache;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.observer.core.db.CiObserverDb;
import ru.yandex.ci.observer.core.db.model.settings.ObserverSettings;
import ru.yandex.ci.storage.core.cache.impl.EntityCacheImpl;
import ru.yandex.ci.storage.core.cache.impl.ModifiableEntityCacheImpl;

public class ObserverSettingsCacheImpl
        extends EntityCacheImpl<ObserverSettings.Id, ObserverSettings, CiObserverDb>
        implements ObserverSettingsCache, ObserverSettingsCache.WithModificationSupport {

    public ObserverSettingsCacheImpl(CiObserverDb db, MeterRegistry meterRegistry) {
        super(
                ObserverSettings.class,
                EntityCacheImpl.createDefault(1, Duration.ofMinutes(1), null),
                db,
                meterRegistry,
                "observer-settings",
                id -> ObserverSettings.EMTPY
        );

        GuavaCacheMetrics.monitor(meterRegistry, cache, this.cacheName);
    }

    @Override
    protected KikimrTableCi<ObserverSettings> getTable() {
        return this.db.settings();
    }

    @Override
    public WithCommitSupport toModifiable(int maxNumberOfWrites) {
        return new Modifiable(this, this.cache, maxNumberOfWrites);
    }

    @Override
    public ObserverSettings get() {
        return this.getOrDefault(ObserverSettings.ID);
    }

    public static class Modifiable
            extends ModifiableEntityCacheImpl<ObserverSettings.Id, ObserverSettings, CiObserverDb>
            implements WithCommitSupport {

        public Modifiable(ObserverSettingsCacheImpl baseImpl,
                          Cache<ObserverSettings.Id, Optional<ObserverSettings>> cache,
                          int maxNumberOfWrites) {
            super(baseImpl, cache, maxNumberOfWrites);
        }

        @Override
        public ObserverSettings get() {
            return ((ObserverSettingsCacheImpl) this.baseImpl).get();
        }
    }
}
