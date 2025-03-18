package ru.yandex.ci.engine.discovery.tier0;

import java.nio.file.Path;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.TimeUnit;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.common.collect.ArrayListMultimap;
import com.google.common.collect.ListMultimap;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.discovery.GraphDiscoveryTask;
import ru.yandex.ci.engine.discovery.TriggeredProcesses;
import ru.yandex.lang.NonNullApi;

@Slf4j
class AffectedAYamlsFinder implements AffectedPathMatcher {

    @Nonnull
    private final OrderedArcRevision revision;
    @Nonnull
    private final LruCache<Path, Boolean> fileExistsCache;
    @Nonnull
    private final ArcService arcService;
    @Nonnull
    private final AYamlFilterChecker aYamlFilterChecker;

    private final int batchSize;

    @Nonnull
    private final Set<TriggeredProcesses.Triggered> triggered = new HashSet<>();
    // class Batch was created to reduce number of logs, written by DiscoveryService
    @Nullable
    private Batch pendingBatch;

    private long consumedPathCount = 0;

    AffectedAYamlsFinder(
            OrderedArcRevision revision,
            ArcService arcService,
            int fileExistsCacheSize,
            AYamlFilterChecker aYamlFilterChecker,
            int batchSize
    ) {
        this.revision = revision;
        this.arcService = arcService;
        fileExistsCache = new LruCache<>(fileExistsCacheSize, this::fileExists);
        this.aYamlFilterChecker = aYamlFilterChecker;
        this.batchSize = batchSize;
    }

    @Override
    // the method puts platform/path to aYamlPathToPlatforms
    public void accept(GraphDiscoveryTask.Platform affectedPlatform, Path affectedPath) throws GraphDiscoveryException {
        var targetAYamls = new ArrayList<Path>();
        Preconditions.checkArgument(
                !affectedPath.endsWith("ya.make"),
                "affected path '%s' should not end with 'ya.make'", affectedPath
        );
        ru.yandex.ci.core.config.a.AffectedAYamlsFinder.addPotentialConfigsRecursively(
                affectedPath, true, targetAYamls
        );
        for (var aYaml : targetAYamls) {
            try {
                boolean fileExists = fileExistsCache.get(aYaml);
                if (!fileExists) {
                    continue;
                }
                /* Sandbox task CHANGES_DETECTOR returns directories, which contains ya.make file.
                   If `ci/__init__.py` is edited, CHANGES_DETECTOR returns `ci`.
                   To make filtering easier, we should add to all directories `ya.make`,
                   so we pretend that CHANGES_DETECTOR returns `ci/ya.make`. This hack allows to say
                   `run this flow when "ci" or any its subdirectory is affected by build dependencies`
                   with one filter 'absPath: ci/**'. Without that hack
                   we have to set two filters: absPath: `[ci, ci/**]` */
                Preconditions.checkArgument(
                        !affectedPath.endsWith("ya.make"),
                        "affected path '%s' should not end with 'ya.make'", affectedPath
                );
                var affectedYaMake = affectedPath.resolve("ya.make");
                collectTriggeredAYamls(affectedPlatform, affectedYaMake, aYaml);
            } catch (Exception e) {
                throw new GraphDiscoveryException(
                        "Can't process: platform %s, aYaml %s, affectedPath %s)"
                                .formatted(affectedPlatform, aYaml, affectedPath),
                        e
                );
            }
        }
        consumedPathCount++;

        // processedPath % 4096 == 0
        if ((consumedPathCount & 4095) == 0) {
            log.info("Consumed paths: {}, a.yaml's triggered {}", consumedPathCount, triggered.size());
        }
    }

    void collectTriggeredAYamls(GraphDiscoveryTask.Platform affectedPlatform, Path affectedYaMake, Path foundAYaml) {
        if (pendingBatch == null) {
            pendingBatch = new Batch(foundAYaml, affectedPlatform, affectedYaMake, batchSize);
            return;
        }
        if (affectedPlatform == pendingBatch.affectedPlatform) {
            if (pendingBatch.isLimitReached()) {
                processPendingBatchAndClearIt("limit reached");
            }
            pendingBatch.add(foundAYaml, affectedYaMake);
            return;
        }
        processPendingBatchAndClearIt("new platform " + affectedPlatform);
        pendingBatch.set(foundAYaml, affectedPlatform, affectedYaMake);
    }

    void processPendingBatchAndClearIt(String reason) {
        if (pendingBatch == null) {
            return;
        }
        pendingBatch.logInfoAboutPendingBatch(reason);
        for (var aYaml : pendingBatch.aYamlToAffectedYaMakes.keySet()) {
            var yaMakes = pendingBatch.aYamlToAffectedYaMakes.get(aYaml);
            if (!yaMakes.isEmpty()) {
                var triggeredForPath = aYamlFilterChecker.findProcessesTriggeredByPathAndPlatform(
                        aYaml, pendingBatch.affectedPlatform, yaMakes, false
                );
                if (!triggeredForPath.isEmpty()) {
                    triggered.addAll(triggeredForPath.getTriggered());
                }
            }
        }

        pendingBatch.clear();
    }

    void logStats() {
        aYamlFilterChecker.logStats();
        log.info("{}, fileExistsCache: hit rate {}, miss {}, total {}, size {}, elapsed {}s",
                revision, fileExistsCache.cacheHitRate(), fileExistsCache.getMissCount(),
                fileExistsCache.getTotalCount(), fileExistsCache.getMaxSize(),
                fileExistsCache.getStopwatch().elapsed(TimeUnit.SECONDS)
        );
    }

    private boolean fileExists(Path targetPath) {
        return arcService.isFileExists(targetPath, revision);
    }

    @Override
    public void flush() {
        processPendingBatchAndClearIt("flushing buffer");
        logStats();
    }

    @Override
    public Set<TriggeredProcesses.Triggered> getTriggered() {
        return triggered;
    }

    @Slf4j
    @NonNullApi
    // without batching DiscoveryService will write too many logs
    private static class Batch {
        @Nonnull
        ListMultimap<Path, String> aYamlToAffectedYaMakes;
        @Nonnull
        GraphDiscoveryTask.Platform affectedPlatform;
        @Nonnull
        Set<String> affectedYaMakes;

        private final int limit;

        private Batch(Path foundAYaml, GraphDiscoveryTask.Platform affectedPlatform, Path affectedYaMake, int limit) {
            this.aYamlToAffectedYaMakes = ArrayListMultimap.create(3, 2000);
            this.affectedYaMakes = new HashSet<>(2000);
            this.affectedPlatform = affectedPlatform;
            this.limit = limit;
            add(foundAYaml, affectedYaMake);
        }

        private void add(Path foundAYaml, Path affectedYaMake) {
            this.aYamlToAffectedYaMakes.put(foundAYaml, affectedYaMake.toString());
            this.affectedYaMakes.add(affectedYaMake.toString());
        }

        private void set(Path foundAYaml, GraphDiscoveryTask.Platform affectedPlatform, Path affectedYaMake) {
            this.affectedPlatform = affectedPlatform;
            clear();
            add(foundAYaml, affectedYaMake);
        }

        private void clear() {
            aYamlToAffectedYaMakes.clear();
            affectedYaMakes.clear();
        }

        private boolean isLimitReached() {
            return affectedYaMakes.size() == limit;
        }

        private void logInfoAboutPendingBatch(String reason) {
            for (var aYaml : aYamlToAffectedYaMakes.keySet()) {
                log.info("process pendingBatch due [{}]: size {}, aYaml: {}",
                        reason, aYamlToAffectedYaMakes.get(aYaml).size(), aYaml);
            }
        }

    }

}
