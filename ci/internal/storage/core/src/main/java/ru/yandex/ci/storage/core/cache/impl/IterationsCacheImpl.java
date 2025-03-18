package ru.yandex.ci.storage.core.cache.impl;

import java.time.Duration;
import java.util.Collection;
import java.util.Optional;
import java.util.function.Function;
import java.util.stream.Collectors;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.cache.Cache;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.storage.core.cache.IterationsCache;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;

@Slf4j
public class IterationsCacheImpl extends EntityCacheImpl<CheckIterationEntity.Id, CheckIterationEntity, CiStorageDb>
        implements IterationsCache, IterationsCache.WithModificationSupport {

    public IterationsCacheImpl(CiStorageDb db, int maxSize, MeterRegistry meterRegistry) {
        super(
                CheckIterationEntity.class,
                // Keep expire time low, to prevent processing of finished results from main stream
                EntityCacheImpl.createDefault(maxSize, Duration.ofMinutes(1)),
                db,
                meterRegistry,
                "iterations"
        );

        GuavaCacheMetrics.monitor(meterRegistry, cache, this.cacheName);
    }

    @Override
    protected KikimrTableCi<CheckIterationEntity> getTable() {
        return this.db.checkIterations();
    }

    @Override
    public IterationsCache.WithCommitSupport toModifiable(int maxNumberOfWrites) {
        return new Modifiable(this, this.cache, maxNumberOfWrites);
    }

    @VisibleForTesting
    public void invalidateAll() {
        cache.invalidateAll();
    }

    @Override
    public Collection<CheckIterationEntity> getFreshForCheck(CheckEntity.Id checkId) {
        return db.currentOrReadOnly(() -> db.checkIterations().findByCheck(checkId));
    }

    public static class Modifiable
            extends ModifiableEntityCacheImpl<CheckIterationEntity.Id, CheckIterationEntity, CiStorageDb>
            implements IterationsCache.WithCommitSupport {

        public Modifiable(
                IterationsCacheImpl baseImpl,
                Cache<CheckIterationEntity.Id, Optional<CheckIterationEntity>> cache,
                int maxNumberOfWrites
        ) {
            super(baseImpl, cache, maxNumberOfWrites);
        }

        @Override
        public void processIterationFinished(CheckIterationEntity.Id iterationId) {
            this.invalidate(iterationId);
        }

        @Override
        public Collection<CheckIterationEntity> getFreshForCheck(CheckEntity.Id checkId) {
            var iterations = baseImpl.db.checkIterations().findByCheck(checkId).stream()
                    .collect(Collectors.toMap(CheckIterationEntity::getId, Function.identity()));

            var fromBuffer = this.writeBuffer.values().stream()
                    .filter(Optional::isPresent)
                    .map(Optional::get)
                    .filter(x -> x.getId().getCheckId().equals(checkId))
                    .collect(Collectors.toMap(CheckIterationEntity::getId, Function.identity()));

            log.info(
                    "Loaded {} iterations for {}, will use {} from write buffer",
                    iterations.size(), checkId, fromBuffer.size()
            );

            iterations.values().stream()
                    .filter(x -> !fromBuffer.containsKey(x.getId()))
                    .forEach(iteration -> this.writeBuffer.put(iteration.getId(), Optional.of(iteration)));

            iterations.putAll(fromBuffer);

            return iterations.values();
        }
    }
}
