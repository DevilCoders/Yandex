package ru.yandex.ci.engine.discovery.tier0;

import java.nio.file.Path;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.function.BiConsumer;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.RequiredArgsConstructor;
import lombok.Value;
import lombok.experimental.NonFinal;

import ru.yandex.ci.core.db.table.ConfigDiscoveryDirTable;
import ru.yandex.ci.core.discovery.GraphDiscoveryTask;
import ru.yandex.ci.engine.discovery.TriggeredProcesses;

class AffectedAbsPathMatcher implements AffectedPathMatcher {

    @Nonnull
    private final Set<TriggeredProcesses.Triggered> triggered;
    @Nonnull
    private final Buffer buffer;
    @Nonnull
    private final Set<String> yaMakeSubDirs;

    AffectedAbsPathMatcher(
            AYamlFilterChecker aYamlFilterChecker,
            ConfigDiscoveryDirCache configDiscoveryDirCache,
            int bufferLimit
    ) {
        triggered = new HashSet<>();
        yaMakeSubDirs = new HashSet<>(bufferLimit * 5);

        buffer = new Buffer(bufferLimit, (platform, yaMakes) -> {
            yaMakeSubDirs.clear();

            for (var yaMake : yaMakes) {
                ConfigDiscoveryDirTable.fillSubDirs(yaMake, yaMakeSubDirs);
            }

            var prefixPathToConfigMap = configDiscoveryDirCache.getPrefixPathToConfigMap();
            for (var yaMakeSubDir : yaMakeSubDirs) {
                var configPath = prefixPathToConfigMap.get(yaMakeSubDir);
                if (configPath != null) {
                    var processes = aYamlFilterChecker.findProcessesTriggeredByPathAndPlatform(
                            Path.of(configPath), platform, yaMakes, true
                    );
                    triggered.addAll(processes.getTriggered());
                }
            }
        });
    }

    @Override
    public void accept(GraphDiscoveryTask.Platform affectedPlatform, Path affectedPath) throws GraphDiscoveryException {
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
        buffer.add(affectedPlatform, affectedYaMake);
    }

    @Override
    public void flush() {
        buffer.flush();
    }

    @Override
    public Set<TriggeredProcesses.Triggered> getTriggered() {
        return triggered;
    }

    @Value
    @RequiredArgsConstructor
    private static class Buffer {

        @Nonnull
        Set<String> yaMakes = new HashSet<>(2000);

        int limit;

        BiConsumer<GraphDiscoveryTask.Platform, List<String>> consumer;

        @Nullable
        @NonFinal
        GraphDiscoveryTask.Platform lastPlatform;

        void add(GraphDiscoveryTask.Platform platform, Path yaMake) {
            if (isNeedFlush(platform)) {
                flush();
            }
            yaMakes.add(yaMake.toString());
            lastPlatform = platform;
        }

        void flush() {
            if (!yaMakes.isEmpty()) {
                consumer.accept(lastPlatform, List.copyOf(yaMakes));
            }
            yaMakes.clear();
        }

        boolean isNeedFlush(GraphDiscoveryTask.Platform newPlatform) {
            return yaMakes.size() > limit ||
                    (lastPlatform != null && lastPlatform != newPlatform);
        }
    }
}
