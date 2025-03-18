package ru.yandex.ci.engine.autocheck;

import java.nio.file.Path;
import java.time.Duration;
import java.util.NavigableSet;
import java.util.Optional;
import java.util.Set;
import java.util.TreeSet;
import java.util.concurrent.ExecutionException;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.cache.CacheBuilder;
import com.google.common.cache.CacheLoader;
import com.google.common.cache.LoadingCache;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.lang3.StringUtils;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.CommitId;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.engine.autocheck.model.AutocheckFeature;
import ru.yandex.ci.engine.autocheck.model.BalancerConfig;

@Slf4j
public class FastCircuitTargetsAutoResolver {

    public static final String CONFIG_PATH = "autocheck/balancing_configs/autocheck-linux.json";

    private final ArcService arc;

    private final CiMainDb db;

    private final LoadingCache<String, Set<String>> cache;

    public FastCircuitTargetsAutoResolver(
            @Nonnull ArcService arc,
            @Nonnull CiMainDb db,
            Duration cacheTtl     // for how long to keep the cached config
    ) {
        this.arc = arc;
        this.db = db;

        cache = CacheBuilder.newBuilder()
                .maximumSize(1)
                .expireAfterWrite(cacheTtl)
                .build(CacheLoader.from(this::loadConfig));
    }

    @Nonnull
    public Optional<String> getFastTarget(@Nonnull Set<Path> affectedPaths) {
        if (!AutocheckFeature.isAutoFastTargetEnabledForPercent(db)) {
            return Optional.empty();
        }

        log.info("Resolving fast targets");
        try {
            Set<String> modules = cache.get(CONFIG_PATH);
            NavigableSet<String> result = affectedPaths.stream()
                    .map(path -> getModuleForPath(modules, path))
                    .filter(Optional::isPresent)
                    .map(Optional::get)
                    .limit(2)
                    .collect(Collectors.toCollection(TreeSet::new));
            switch (result.size()) {
                case 0 -> {
                    log.info("No fast target candidates detected.");
                    return Optional.empty();
                }
                case 1 -> {
                    String target = result.iterator().next();
                    log.info("Detected fast target: {}", target);
                    return Optional.of(target);
                }
                default -> {
                    log.info("Multiple fast targets were detected: {}. Canceling.", String.join(", ", result));
                    return Optional.empty();
                }
            }
        } catch (ExecutionException e) {
            throw new IllegalStateException("Error loading config file", e);
        }
    }

    private Optional<String> getModuleForPath(Set<String> modules, Path path) {
        for (Path dir = path; dir != null; dir = dir.getParent()) {
            String sample = StringUtils.removeStart(dir.toString(), "/");
            if (modules.contains(sample)) {
                log.info("Found module module {} for changed file {}", dir, path);
                return Optional.of(sample);
            }
        }
        return Optional.empty();
    }

    @Nonnull
    private Set<String> loadConfig(@Nonnull String path) {
        CommitId head = arc.getLastRevisionInBranch(ArcBranch.trunk());
        String json = arc.getFileContent(path, head)
                .orElseThrow(() -> new IllegalArgumentException("File " + path + " was not found"));
        try {
            BalancerConfig config = new ObjectMapper().readValue(json, BalancerConfig.class);
            return config.getBiggestPartition();
        } catch (JsonProcessingException e) {
            throw new IllegalStateException("Error converting file '%s' to BalancerConfig".formatted(path));
        }
    }

    public void clearCache() {
        cache.invalidateAll();
    }
}
