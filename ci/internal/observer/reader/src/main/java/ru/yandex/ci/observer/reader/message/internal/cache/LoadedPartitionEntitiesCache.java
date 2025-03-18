package ru.yandex.ci.observer.reader.message.internal.cache;

import java.time.Duration;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.ExecutionException;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;
import lombok.Getter;

import ru.yandex.ci.observer.core.db.model.check.CheckEntity;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.observer.reader.message.InternalPartitionGenerator;

public class LoadedPartitionEntitiesCache {
    private final InternalPartitionGenerator partitionGenerator;

    @Getter
    private final Cache<Integer, Set<CheckEntity.Id>> checks;
    @Getter
    private final Cache<Integer, Set<CheckIterationEntity.Id>> iterations;

    public LoadedPartitionEntitiesCache(
            InternalPartitionGenerator partitionGenerator, Duration expireAfter, MeterRegistry meterRegistry
    ) {
        this.partitionGenerator = partitionGenerator;

        checks = CacheBuilder.newBuilder()
                .expireAfterAccess(expireAfter)
                .recordStats()
                .build();
        iterations = CacheBuilder.newBuilder()
                .expireAfterAccess(expireAfter)
                .recordStats()
                .build();

        GuavaCacheMetrics.monitor(meterRegistry, checks, "observer-internal-loaded-checks");
        GuavaCacheMetrics.monitor(meterRegistry, iterations, "observer-internal-loaded-iterations");
    }

    public void onCheckLoaded(CheckEntity.Id checkId) {
        try {
            var cached = checks.get(
                    getPartition(checkId),
                    HashSet::new
            );

            cached.add(checkId);
        } catch (ExecutionException ex) {
            throw new RuntimeException(ex);
        }
    }

    public void onIterationLoaded(CheckIterationEntity.Id iterationId) {
        try {
            var cached = iterations.get(
                    getPartition(iterationId),
                    HashSet::new
            );

            cached.add(iterationId);
        } catch (ExecutionException ex) {
            throw new RuntimeException(ex);
        }
    }

    @VisibleForTesting
    public int getPartition(CheckEntity.Id checkId) {
        return partitionGenerator.generatePartition(checkId);
    }

    @VisibleForTesting
    public int getPartition(CheckIterationEntity.Id iterationId) {
        return getPartition(iterationId.getCheckId());
    }
}
