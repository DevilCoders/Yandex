package ru.yandex.ci.ayamler.fetcher;

import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.function.Predicate;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.annotations.VisibleForTesting;
import io.micrometer.core.instrument.MeterRegistry;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.ayamler.AYaml;
import ru.yandex.ci.ayamler.AYamlerServiceProperties;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.config.a.AYamlService;
import ru.yandex.ci.core.config.a.AffectedAYamlsFinder;
import ru.yandex.ci.util.ExceptionUtils;

@Slf4j
public class FileFetcher {

    private final OidCache oidCache;
    private final AYamlCache aYamlCache;

    public FileFetcher(
            ArcService arcService,
            AYamlService aYamlService,
            MeterRegistry meterRegistry,
            AYamlerServiceProperties properties
    ) {
        this.oidCache = new OidCache(arcService, meterRegistry, properties);
        this.aYamlCache = new AYamlCache(aYamlService, arcService, meterRegistry, properties);
    }

    public Map<OidCache.Key, Optional<AYaml>> getFirstMetAYamlForEachPath(Set<OidCache.Key> pathAndRevisionSet,
                                                                          Predicate<AYaml> stopWhenTrue) {
        // we need this map to write to log which a.yaml was found for concrete path
        var aYamlToPotentionalAYamls = new HashMap<OidCache.Key, List<OidCache.Key>>();

        for (var pathAndRevision : pathAndRevisionSet) {
            var aYamlPaths = new ArrayList<Path>();
            /* Call addPotentialConfigsRecursively(isDir=true) cause:
               - CI storage always sends directory paths
               - In worse case we check path like 'ci/a.yaml/a.yaml'. It doesn't break anything */
            AffectedAYamlsFinder.addPotentialConfigsRecursively(pathAndRevision.getPath(), true, aYamlPaths);

            var pathsShouldBeChecked = aYamlToPotentionalAYamls.computeIfAbsent(pathAndRevision,
                    k -> new ArrayList<>());
            aYamlPaths.forEach(path -> pathsShouldBeChecked.add(
                    OidCache.Key.of(path, pathAndRevision.getRevision())
            ));
        }

        // path has oid when it exists
        Map<OidCache.Key, CompletableFuture<Optional<String>>> pathAndRevisionToOid = oidCache.load(
                aYamlToPotentionalAYamls.values()
                        .stream()
                        .flatMap(Collection::stream)
                        .collect(Collectors.toSet())
        );

        var result = new HashMap<OidCache.Key, Optional<AYaml>>();
        aYamlToPotentionalAYamls.forEach((pathAndRevision, pathsShouldBeChecked) -> {
            var aYaml = fetchFirst(pathsShouldBeChecked, stopWhenTrue, pathAndRevisionToOid);
            result.put(pathAndRevision, aYaml);
        });

        return result;
    }

    private Optional<AYaml> fetchFirst(
            List<OidCache.Key> pathsShouldBeChecked,
            Predicate<AYaml> stopWhenTrue,
            Map<OidCache.Key, CompletableFuture<Optional<String>>> pathAndRevisionToOid
    ) {
        for (var pathAndRevision : pathsShouldBeChecked) {
            var aYaml = fetchWithTryCatch(() -> {
                var oid = pathAndRevisionToOid.get(pathAndRevision)
                        // TODO: specify timeout for CompletedFuture just in case,
                        //   despite of that arcService calls have deadline
                        .get();
                if (oid.isEmpty()) {
                    // means that a.yaml at this path doesn't exist
                    return null;
                }
                log.debug("{} at {}: oid {}", pathAndRevision.getPath(), pathAndRevision.getRevision(), oid);
                return aYamlCache.load(
                                pathAndRevision.getPath(),
                                pathAndRevision.getRevision(),
                                oid.get()
                        )
                        // TODO: throw an exception?
                        .orElse(null);
            });
            if (aYaml == null) {
                continue;
            }
            if (stopWhenTrue.test(aYaml)) {
                // find first
                return Optional.of(aYaml);
            }
        }
        return Optional.empty();
    }

    public Set<String> getKnownAbcServices() {
        return aYamlCache.getAYamlCache().asMap().values().stream()
                .map(it -> it.map(AYaml::getService).orElse(null))
                .filter(Objects::nonNull)
                .collect(Collectors.toSet());
    }

    @VisibleForTesting
    public void resetCaches() {
        oidCache.reset();
        aYamlCache.reset();
    }

    @Nullable
    private static AYaml fetchWithTryCatch(FetchFileAction action) {
        try {
            return action.fetch();
        } catch (InterruptedException e) {
            throw new RuntimeException("Interrupted", e);
        } catch (ExecutionException e) {
            var unwrapped = ExceptionUtils.unwrap(e);
            log.error("Some future is completed exceptionally", unwrapped);
            // TODO: don't fail all batch, try to return part values
            throw unwrapped;
        }
    }

    @FunctionalInterface
    private interface FetchFileAction {
        @Nullable
        AYaml fetch() throws InterruptedException, ExecutionException;
    }

}
