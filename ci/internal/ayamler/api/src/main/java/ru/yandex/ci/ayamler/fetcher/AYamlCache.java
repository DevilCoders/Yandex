package ru.yandex.ci.ayamler.fetcher;

import java.nio.file.Path;
import java.util.Optional;
import java.util.concurrent.ExecutionException;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.cache.CacheBuilder;
import com.google.common.cache.CacheLoader;
import com.google.common.cache.LoadingCache;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;
import lombok.EqualsAndHashCode;
import lombok.ToString;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.ayamler.AYaml;
import ru.yandex.ci.ayamler.AYamlerServiceProperties;
import ru.yandex.ci.ayamler.PathGetter;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.RepoStat;
import ru.yandex.ci.core.config.a.AYamlNotFoundException;
import ru.yandex.ci.core.config.a.AYamlService;
import ru.yandex.ci.core.config.a.model.AYamlConfig;

@Slf4j
class AYamlCache {

    private final AYamlService aYamlService;
    private final ArcService arcService;
    private final LoadingCache<Key, Optional<AYaml>> aYamlCache;


    AYamlCache(
            AYamlService aYamlService,
            ArcService arcService,
            MeterRegistry meterRegistry,
            AYamlerServiceProperties properties
    ) {
        this.aYamlService = aYamlService;
        this.arcService = arcService;
        this.aYamlCache = CacheBuilder.newBuilder()
                .expireAfterAccess(properties.getAYamlCacheExpireAfterAccess())
                /* TODO: choose right size and/or use ydb to keep records
                   1000 checks (== 1000 revisions) are running at the same time,
                   but we assume that a.yaml files are changed very rarely,
                   so we need to keep only 1 revision instead of 1000
                   Assume that there are 10 000 a.yamls (but now we have 1000 a.yamls).
                   10 000 * (sizeof(AYamlCacheKey) + sizeof(AYaml)) =
                      = 10Kb * (64 (path) + 40 (revision) + 1 000 bytes)
                      ~= 10Mb */
                .maximumSize(properties.getAYamlCacheMaximumSize())
                .concurrencyLevel(properties.getCacheConcurrencyLevel())
                .recordStats()
                .build(CacheLoader.from(this::fetchAYamlFromArc));
        GuavaCacheMetrics.monitor(meterRegistry, aYamlCache, "a-yaml");
    }

    Optional<AYaml> load(Path path, ArcRevision revision, String oid) throws ExecutionException {
        return aYamlCache.get(AYamlCache.Key.of(path, oid, revision));
    }

    private Optional<AYaml> fetchAYamlFromArc(Path aYamlPath, ArcRevision revision, String oid) {
        AYamlConfig config;
        try {
            var lastChangedRevision = arcService.getStat(aYamlPath.toString(), revision, true)
                    .map(RepoStat::getLastChanged)
                    .orElseThrow(() -> new IllegalArgumentException(
                            "Can get last changed revision for %s at %s (oid %s)".formatted(aYamlPath, revision, oid)
                    ))
                    .getRevision();

            /* - Passing oid instead of lastChangedRevision into `aYamlService.getConfig` can break something inside.
               - We use oid an analogue of 'last changed revision' (getting oid is faster) for caching a.yaml content */
            config = aYamlService.getConfig(aYamlPath, lastChangedRevision);
        } catch (AYamlNotFoundException e) {
            return Optional.empty();
        } catch (Exception e) {
            log.info("{} at {} (oid {}): failed to process ayaml", aYamlPath, revision, oid, e);
            return Optional.of(AYaml.invalid(aYamlPath, e.getMessage(), false));
        }
        return Optional.of(AYaml.valid(aYamlPath, config));
    }

    private Optional<AYaml> fetchAYamlFromArc(Key key) {
        return fetchAYamlFromArc(key.getPath(), key.getRevision(), key.getOid());
    }

    LoadingCache<Key, Optional<AYaml>> getAYamlCache() {
        return aYamlCache;
    }

    @VisibleForTesting
    void reset() {
        aYamlCache.invalidateAll();
    }

    @SuppressWarnings("MissingOverride")
    @Value(staticConstructor = "of")
    public static class Key implements PathGetter {
        Path path;
        String oid; // oid is an analogue of 'last changed revision' for file, but works faster

        @ToString.Exclude
        @EqualsAndHashCode.Exclude
        ArcRevision revision;   // revision changes from one request to another, can't be part of cache key
    }
}
