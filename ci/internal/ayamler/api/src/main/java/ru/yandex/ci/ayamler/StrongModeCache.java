package ru.yandex.ci.ayamler;

import java.nio.file.Path;
import java.util.HashMap;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.base.Preconditions;
import com.google.common.cache.Cache;
import com.google.common.cache.CacheBuilder;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.ayamler.fetcher.FileFetcher;
import ru.yandex.ci.ayamler.fetcher.OidCache;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.config.a.model.StrongModeConfig;

@Slf4j
class StrongModeCache {

    private final FileFetcher fileFetcher;
    private final AbcService abcService;
    private final Cache<Key, Optional<AYaml>> strongModeCache;

    StrongModeCache(
            FileFetcher fileFetcher,
            AbcService abcService,
            MeterRegistry meterRegistry,
            AYamlerServiceProperties properties
    ) {
        this.fileFetcher = fileFetcher;
        this.abcService = abcService;
        this.strongModeCache = CacheBuilder.newBuilder()
                .expireAfterAccess(properties.getStrongModeCacheExpireAfterAccess())
                /* TODO: choose right size and/or use ydb to keep records
                   1000 checks (== 1000 revisions) are running at the same time,
                   2000-5000 paths per check (2000 uniq, 5000 not uniq).
                   When cacheKey is (path,revision), then
                   (256 + 40) bytes * 1000 * 5000 ~= 1Gb
                   (without counting size of java object header/reference size/maps/etc).

                   So when maximumSize == 5 000 000, size > 1Gb
                   So when maximumSize ==   500 000, size > 100Mb */
                .concurrencyLevel(properties.getCacheConcurrencyLevel())
                .maximumSize(properties.getStrongModeCacheMaximumSize())
                .recordStats()
                .build();
        GuavaCacheMetrics.monitor(meterRegistry, strongModeCache, "strong-mode");
    }

    Map<Key, Optional<AYaml>> load(Set<Key> keys) {
        return AYamlerServiceHelper.loadFromCache(strongModeCache, this::computeStrongModeBatch, keys);
    }

    private Map<Key, Optional<AYaml>> computeStrongModeBatch(Set<Key> keys) {
        var foundAYamls = fileFetcher.getFirstMetAYamlForEachPath(
                keys.stream()
                        .map(Key::toOidCacheKey)
                        .collect(Collectors.toSet()),
                aYaml -> {
                    // stop searching if a.yaml is invalid
                    if (!aYaml.isValid()) {
                        return true;
                    }
                    // stop searching if a.yaml has defined strong mode
                    return aYaml.getStrongMode() != null;
                }
        );

        var result = new HashMap<Key, Optional<AYaml>>();
        for (var strongModeKey : keys) {
            var foundAYaml = Objects.requireNonNull(foundAYamls.get(strongModeKey.toOidCacheKey()))
                    .map(aYaml -> evaluateFields(aYaml, strongModeKey));

            log.info(
                    "{} at {} for {}: strong mode {}",
                    strongModeKey.getPath(), strongModeKey.getRevision(), strongModeKey.getLogin(), foundAYaml
            );

            result.put(strongModeKey, foundAYaml);
        }
        return result;
    }

    private AYaml evaluateFields(AYaml aYaml, Key strongModeKey) {
        if (!aYaml.isValid()) {
            log.info("{} at {}: found {}, set strong mode false cause a.yaml is invalid",
                    strongModeKey.getPath(), strongModeKey.getRevision(), aYaml.getPath());
            return aYaml.withStrongMode(new StrongMode(false, Set.of()));
        }

        if (strongModeKey.getLogin() == null) {
            return aYaml;
        }

        var service = aYaml.getService();
        Preconditions.checkState(service != null, "%s is valid, but field 'service' is null", aYaml.getPath());

        var scopes = aYaml.getStrongMode() != null ?
                aYaml.getStrongMode().getAbcScopes() : StrongModeConfig.DEFAULT_ABC_SCOPES;

        var userBelongsToService = abcService.isUserBelongsToService(strongModeKey.getLogin(), service, scopes);

        if (aYaml.getStrongMode() != null && aYaml.getStrongMode().isEnabled()) {
            if (!userBelongsToService) {
                log.info("{} at {}: found {}, set strong mode false cause {} not in abc group {}",
                        strongModeKey.getPath(), strongModeKey.getRevision(), aYaml.getPath(),
                        strongModeKey.getLogin(), service
                );

                return aYaml.withStrongMode(aYaml.getStrongMode().withEnabled(false));
            }
        }

        return aYaml.withOwner(userBelongsToService);
    }

    @VisibleForTesting
    void reset() {
        strongModeCache.invalidateAll();
    }

    @SuppressWarnings("MissingOverride")
    @Value(staticConstructor = "of")
    static class Key implements PathGetter {
        Path path;
        ArcRevision revision;

        @Nullable
        String login;

        static Key of(PathAndLogin pathAndLogin, ArcRevision revision) {
            return Key.of(
                    pathAndLogin.getPath(),
                    revision,
                    pathAndLogin.getLogin()
            );
        }

        public PathAndLogin getPathAndLogin() {
            return PathAndLogin.of(path, login);
        }

        public OidCache.Key toOidCacheKey() {
            return OidCache.Key.of(path, revision);
        }
    }
}
