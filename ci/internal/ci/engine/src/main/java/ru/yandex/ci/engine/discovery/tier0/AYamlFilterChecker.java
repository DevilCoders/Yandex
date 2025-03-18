package ru.yandex.ci.engine.discovery.tier0;

import java.nio.file.Path;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.TimeUnit;

import com.google.common.base.Stopwatch;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.discovery.GraphDiscoveryTask;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.engine.discovery.DiscoveryServicePostCommits;
import ru.yandex.ci.engine.discovery.TriggeredProcesses;
import ru.yandex.lang.NonNullApi;

@Slf4j
@NonNullApi
class AYamlFilterChecker {

    private final LruCache<Path, Optional<ConfigBundle>> configBundleCache;
    private final OrderedArcRevision revision;
    private final ArcRevision previousRevision;

    private final DiscoveryServicePostCommits discoveryServicePostCommits;

    private final ArcCommit commit;
    private final Stopwatch findTriggeredProcessesStopwatch = Stopwatch.createUnstarted();

    AYamlFilterChecker(
            OrderedArcRevision revision,
            ArcCommit commit,
            ArcRevision previousRevision,
            DiscoveryServicePostCommits discoveryServicePostCommits,
            LruCache<Path, Optional<ConfigBundle>> configBundleCache
    ) {
        this.revision = revision;
        this.commit = commit;
        this.previousRevision = previousRevision;
        this.discoveryServicePostCommits = discoveryServicePostCommits;
        this.configBundleCache = configBundleCache;
    }

    TriggeredProcesses findProcessesTriggeredByPathAndPlatform(
            Path aYamlPath,
            GraphDiscoveryTask.Platform platform,
            List<String> affectedPaths,
            boolean aYamlShouldContainAbsPathFilter
    ) {
        log.info("Started checking triggered flows for {} {}", aYamlPath, platform);
        if (affectedPaths.isEmpty()) {
            log.info("Finished checking triggered flows for {} {}, cause affectedPaths are empty", aYamlPath, platform);
            return TriggeredProcesses.empty();
        }

        var configBundle = configBundleCache.get(aYamlPath).orElse(null);
        if (configBundle == null) {
            log.info("No config bundle found for: {} {}", aYamlPath, platform);
            // no valid config bundle exists
            return TriggeredProcesses.empty();
        }

        if (!configBundle.getStatus().isValidCiConfig()) {
            log.info("Config {} not in valid state ({}), skipping.", aYamlPath, configBundle.getStatus());
            return TriggeredProcesses.empty();
        }

        var context = GraphDiscoveryHelper.discoveryContextBuilder(aYamlShouldContainAbsPathFilter)
                .revision(revision)
                .previousRevision(previousRevision)
                .commit(commit)
                .configBundle(configBundle)
                .affectedPaths(affectedPaths)
                .build();

        var ciConfig = configBundle.getValidAYamlConfig().getCi();
        TriggeredProcesses triggeredProcesses;
        findTriggeredProcessesStopwatch.start();
        try {
            triggeredProcesses = discoveryServicePostCommits.findTriggeredProcesses(context, ciConfig);
        } finally {
            findTriggeredProcessesStopwatch.stop();
        }
        log.info("For {} {} triggered proceses {}", aYamlPath, platform, triggeredProcesses);
        log.info("Finished checking triggered flows for {} {}", aYamlPath, platform);
        return triggeredProcesses;
    }

    void logStats() {
        log.info("{}, findTriggeredProcessesStopwatch elapsed {}s",
                revision, findTriggeredProcessesStopwatch.elapsed(TimeUnit.SECONDS));
    }

}
