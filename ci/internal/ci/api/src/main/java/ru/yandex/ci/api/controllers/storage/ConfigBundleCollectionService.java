package ru.yandex.ci.api.controllers.storage;

import java.nio.file.Path;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.RequiredArgsConstructor;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.a.AffectedAYamlsFinder;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.flow.db.CiDb;

@Slf4j
@RequiredArgsConstructor
public class ConfigBundleCollectionService {

    @Nonnull
    private final ConfigurationService configurationService;

    private final CiDb db;

    // TODO: Extend ayamler with CI config state and move related code directly to ayamler
    public List<PrefixedDir> getLastValidConfigs(long revision, List<String> prefixDirs) {
        if (prefixDirs.isEmpty()) {
            return List.of();
        }

        var orderedRev = OrderedArcRevision.fromHash("r" + revision, ArcBranch.trunk(), revision, 0);

        // Make it simple - load all non-deleted configs, match against paths then try to discover last valid config
        var dirs = db.scan().run(() -> db.configStates().findVisiblePaths()).stream()
                .map(Path::of)
                .map(Path::getParent) // Parent of a.yaml -> i.e. dir
                .filter(Objects::nonNull) // Exclude a.yaml from Arcadia root
                .collect(Collectors.toSet());

        log.info("Loaded {} configs to match", dirs.size());
        return new ConfigCollector(orderedRev, dirs).getLastValidConfigs(prefixDirs);
    }


    @RequiredArgsConstructor
    private class ConfigCollector {
        private final OrderedArcRevision orderedRev; //orderedArcRevision;
        private final Set<Path> dirs;
        private final Map<Path, ConfigBundle> configCache = new HashMap<>();

        public List<PrefixedDir> getLastValidConfigs(List<String> prefixDirs) {
            var configMap = new LinkedHashMap<Path, List<String>>();
            for (var prefixDir : prefixDirs) {
                var matched = matchAYamlDir(Path.of(prefixDir));
                if (matched != null) {
                    var path = matched.resolve(AffectedAYamlsFinder.CONFIG_FILE_NAME);
                    configMap.computeIfAbsent(path, p -> new ArrayList<>()).add(prefixDir);
                }
            }

            if (configMap.isEmpty()) {
                log.info("No matched configs to lookup for latest configuration");
                return List.of();
            } else {
                log.info("Matched {} configs to lookup for latest configuration", configMap.keySet().size());
                return db.currentOrReadOnly(() -> getLastValidConfigsImplInTx(configMap));
            }
        }

        private List<PrefixedDir> getLastValidConfigsImplInTx(Map<Path, List<String>> configMap) {
            var result = new ArrayList<PrefixedDir>();
            for (var entry : configMap.entrySet()) {
                var path = entry.getKey();
                var lastConfig = lookupConfigBundleRecursive(path);
                if (lastConfig == null) {
                    continue; // ---
                }

                path = lastConfig.getConfigPath();

                var rev = lastConfig.getRevision();

                var prefixes = entry.getValue();
                Preconditions.checkState(!prefixes.isEmpty());

                log.info("Include match, path: {}, rev: {}, prefixes: {}", path, rev, prefixes);
                for (var prefix : prefixes) {
                    result.add(PrefixedDir.of(prefix, path.toString(), rev));
                }
            }
            return result;
        }

        @Nullable
        private ConfigBundle lookupConfigBundleRecursive(Path path) {
            var bundle = configCache.get(path);
            if (bundle != null) {
                return bundle;
            }

            var validConfig = configurationService.getLastReadyOrNotCiConfig(path, orderedRev);
            if (validConfig.isEmpty()) {
                var parentDir = path.getParent(); // Path = a.yaml
                if (parentDir != null) {
                    parentDir = matchAYamlDir(parentDir.getParent());
                    if (parentDir != null) {
                        var parentPath = parentDir.resolve(AffectedAYamlsFinder.CONFIG_FILE_NAME);
                        log.info("Last config is not CI for {}, parent is {}", path, parentPath);
                        return lookupConfigBundleRecursive(parentPath);
                    }
                }
                return null;
            } else {
                var loadedBundle = validConfig.get();
                configCache.put(path, loadedBundle);
                return loadedBundle;
            }
        }

        @Nullable
        private Path matchAYamlDir(Path path) {
            while (path != null) {
                if (dirs.contains(path)) {
                    return path;
                }
                path = path.getParent();
            }
            return null;
        }

    }

    @Value(staticConstructor = "of")
    public static class PrefixedDir {
        @Nonnull
        String prefix;
        @Nonnull
        String path;
        @Nonnull
        OrderedArcRevision configRevision;
    }
}
