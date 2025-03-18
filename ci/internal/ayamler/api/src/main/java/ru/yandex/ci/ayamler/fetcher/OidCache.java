package ru.yandex.ci.ayamler.fetcher;

import java.nio.file.Path;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.CompletableFuture;
import java.util.function.Function;
import java.util.stream.Collectors;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;
import lombok.Value;

import ru.yandex.ci.ayamler.AYamlerServiceHelper;
import ru.yandex.ci.ayamler.AYamlerServiceProperties;
import ru.yandex.ci.ayamler.PathGetter;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.RepoStat;

public class OidCache {

    private final ArcService arcService;
    private final Cache<Key, CompletableFuture<Optional<String>>> pathAndRevisionToOidCache;

    OidCache(
            ArcService arcService,
            MeterRegistry meterRegistry,
            AYamlerServiceProperties properties
    ) {
        this.arcService = arcService;
        this.pathAndRevisionToOidCache = CacheBuilder.newBuilder()
                .expireAfterAccess(properties.getAYamlOidCacheExpireAfterAccess())
                /* TODO: choose right size and/or use ydb to keep records
                   1000 checks (== 1000 revisions) are running at the same time.
                   Предположим, что в Арке всего 10 000 a.yaml-ов (реально их только 1000).

                   Расчёты очень примерные, потому что
                   - не все a.yaml'ы затрагивает проверка (пусть проверка затрагивает 100 a.yaml'ов)
                   - большей части путей будет соответсовать Optional.empty()
                   - проверка затрагивает поярдка 2000 уникальных путей

                   То есть:
                   1000 ревизий * 2000 путей * (100 bytes средняя длина пути + 40 bytes ревизия) = 150 Mb
                   1000 ревизий * 100 затронутых a.yaml-ов * (50 bytes средная длина + 40 ревизия + 40 oid) = 15Мб

                   Суммарно ~200Мб при размере кеша (2 000 000 + 100 000).
                 */
                .concurrencyLevel(properties.getCacheConcurrencyLevel())
                .maximumSize(properties.getAYamlOidCacheMaximumSize())
                .recordStats()
                .build();
        GuavaCacheMetrics.monitor(meterRegistry, pathAndRevisionToOidCache, "a-yaml-oid");
    }

    public Map<Key, CompletableFuture<Optional<String>>> load(Set<Key> keys) {
        return AYamlerServiceHelper.loadFuturesFromCache(
                pathAndRevisionToOidCache,
                this::fetchAYamlOidFromArc,
                keys
        );
    }

    private Map<Key, CompletableFuture<Optional<String>>> fetchAYamlOidFromArc(
            Set<Key> oidKeys
    ) {
        return oidKeys.stream().collect(Collectors.toMap(
                Function.identity(),
                oidKey -> arcService.getStatAsync(oidKey.getPath().toString(), oidKey.getRevision(), false)
                        .thenApply(optionalStat ->
                                optionalStat.filter(it -> it.getType() == RepoStat.EntryType.FILE)
                                        .map(RepoStat::getFileOid)
                        )
        ));
    }

    @VisibleForTesting
    void reset() {
        pathAndRevisionToOidCache.invalidateAll();
    }

    @SuppressWarnings("MissingOverride")
    @Value(staticConstructor = "of")
    public static class Key implements PathGetter {
        Path path;
        ArcRevision revision;
    }
}
