package ru.yandex.ci.ayamler;

import java.nio.file.Path;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.annotations.VisibleForTesting;
import io.micrometer.core.instrument.MeterRegistry;

import ru.yandex.ci.ayamler.fetcher.FileFetcher;
import ru.yandex.ci.ayamler.fetcher.OidCache;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.config.a.AYamlService;

public class AYamlerService {

    private final FileFetcher fileFetcher;
    private final StrongModeCache strongModeCache;
    private final AbcService abcService;

    @SuppressWarnings("HiddenField")
    public AYamlerService(
            ArcService arcService,
            AYamlService aYamlService, AbcService abcService, MeterRegistry meterRegistry,
            AYamlerServiceProperties properties
    ) {
        var fileFetcher = new FileFetcher(arcService, aYamlService, meterRegistry, properties);

        this.abcService = abcService;
        this.fileFetcher = fileFetcher;
        this.strongModeCache = new StrongModeCache(fileFetcher, abcService, meterRegistry, properties);
    }

    public Optional<AYaml> getStrongMode(
            Path path, ArcRevision revision, @Nullable String login
    ) {
        var pathAndLogin = PathAndLogin.of(path, login);
        return getStrongMode(revision, Set.of(pathAndLogin))
                .get(pathAndLogin);
    }

    public Map<PathAndLogin, Optional<AYaml>> getStrongMode(ArcRevision revision, Set<PathAndLogin> paths) {
        var revisionCorrected = AYamlerServiceHelper.correctRevisionFormat(revision);
        Map<StrongModeCache.Key, Optional<AYaml>> values = strongModeCache.load(
                paths.stream()
                        .map(pathAndLogin -> StrongModeCache.Key.of(pathAndLogin, revisionCorrected))
                        .collect(Collectors.toSet())
        );
        return values.entrySet().stream().collect(Collectors.toMap(
                it -> it.getKey().getPathAndLogin(),
                Map.Entry::getValue
        ));
    }

    public Map<Path, Optional<AYaml>> getAbcServiceSlugBatch(ArcRevision revision, Set<Path> paths) {
        var revisionCorrected = AYamlerServiceHelper.correctRevisionFormat(revision);
        var aYamls = fileFetcher.getFirstMetAYamlForEachPath(
                paths.stream()
                        .map(path -> OidCache.Key.of(path, revisionCorrected))
                        .collect(Collectors.toSet()),
                AYaml::isValid
        );
        return aYamls.entrySet().stream().collect(Collectors.toMap(
                it -> it.getKey().getPath(),
                Map.Entry::getValue
        ));
    }

    public void refreshAbcCacheForKnownServices() {
        fileFetcher.getKnownAbcServices()
                .forEach(abcService::refreshServiceMembersCache);
    }

    @VisibleForTesting
    void resetCaches() {
        fileFetcher.resetCaches();
        strongModeCache.reset();
    }

}
