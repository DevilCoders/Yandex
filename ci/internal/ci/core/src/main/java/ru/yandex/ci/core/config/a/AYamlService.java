package ru.yandex.ci.core.config.a;

import java.nio.file.Path;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.ExecutionException;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.github.fge.jsonschema.core.exceptions.ProcessingException;
import com.google.common.base.Preconditions;
import com.google.common.cache.CacheBuilder;
import com.google.common.cache.CacheLoader;
import com.google.common.cache.LoadingCache;
import com.google.common.util.concurrent.UncheckedExecutionException;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;
import lombok.Value;
import org.yaml.snakeyaml.error.YAMLException;

import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.CommitId;
import ru.yandex.ci.core.config.a.model.AYamlConfig;
import ru.yandex.ci.core.config.a.validation.AYamlValidationException;
import ru.yandex.ci.core.config.a.validation.AYamlValidationInternalException;
import ru.yandex.ci.core.config.a.validation.ValidationReport;
import ru.yandex.ci.util.ExceptionUtils;
import ru.yandex.lang.NonNullApi;

@NonNullApi
public class AYamlService {

    private final ArcService arcService;
    private final LoadingCache<ConfigKey, AYamlConfig> configCache;

    public AYamlService(ArcService arcService,
                        @Nullable MeterRegistry meterRegistry,
                        Duration configCacheExpireAfterAccess,
                        int configCacheMaximumSize) {
        this.arcService = arcService;
        this.configCache = CacheBuilder.newBuilder()
                .expireAfterAccess(configCacheExpireAfterAccess)
                .maximumSize(configCacheMaximumSize)
                .recordStats()
                .build(new CacheLoader<>() {
                    @Override
                    public AYamlConfig load(ConfigKey key) throws Exception {
                        return getConfigFromArc(key.getPath(), key.getRevision());
                    }
                });
        if (meterRegistry != null) {
            GuavaCacheMetrics.monitor(meterRegistry, configCache, "ayaml-configs-cache");
        }
    }

    public boolean isFileNotFound(CommitId revision, Path path) {
        return !arcService.isFileExists(path, revision);
    }

    public AYamlConfig getConfig(Path configPath, CommitId revision) throws ProcessingException,
            JsonProcessingException, AYamlValidationException, AYamlValidationInternalException {
        try {
            return configCache.get(ConfigKey.of(configPath, ArcRevision.of(revision)));
        } catch (UncheckedExecutionException | ExecutionException e) {
            Throwable cause = e.getCause();
            if (cause instanceof ProcessingException ex) {
                throw ex;
            }
            if (cause instanceof JsonProcessingException ex) {
                throw ex;
            }
            if (cause instanceof AYamlValidationException ex) {
                throw ex;
            }
            if (cause instanceof AYamlValidationInternalException ex) {
                throw ex;
            }
            throw ExceptionUtils.unwrap(e);
        }
    }

    private AYamlConfig getConfigFromArc(Path configPath, ArcRevision revision)
            throws ProcessingException, JsonProcessingException,
            AYamlValidationException, AYamlValidationInternalException {

        Preconditions.checkArgument(configPath.getFileName().toString().equals(AffectedAYamlsFinder.CONFIG_FILE_NAME),
                "Trying to process configuration file %s, must be %s",
                configPath.getFileName(), AffectedAYamlsFinder.CONFIG_FILE_NAME);
        Optional<String> content = arcService.getFileContent(configPath, revision);
        var commit = arcService.getCommit(revision);

        if (content.isEmpty()) {
            throw new AYamlNotFoundException("File %s not found on revision %s"
                    .formatted(configPath, revision.getCommitId()));
        }

        ValidationReport validationReport;
        try {
            validationReport = AYamlParser.parseAndValidate(content.get(), commit);
        } catch (ProcessingException | JsonProcessingException | YAMLException e) {
            throw e;
        } catch (Exception e) {
            throw new AYamlValidationInternalException(e);
        }

        if (!validationReport.isSuccess()) {
            throw new AYamlValidationException(validationReport);
        }

        return validationReport.getConfig();
    }


    public static boolean isAYaml(Path path) {
        return path.getFileName().toString().equals(AffectedAYamlsFinder.CONFIG_FILE_NAME);
    }

    public static void verifyIsAYaml(Path path) {
        Preconditions.checkState(isAYaml(path),
                "File in path %s invalid, must be \"%s\"", path, AffectedAYamlsFinder.CONFIG_FILE_NAME);
    }

    public static boolean isAYmlNotYaml(Path path) {
        return path.getFileName().toString().equals("a.yml");
    }

    public static Path dirToConfigPath(Path configDir) {
        return dirToConfigPath(configDir.toString());
    }

    public static Path dirToConfigPath(String configDir) {
        return Path.of(configDir, AffectedAYamlsFinder.CONFIG_FILE_NAME);
    }

    public static String pathToDir(Path configPath) {
        if (configPath.getParent() == null) {
            return "";
        }
        return configPath.getParent().toString();
    }

    public static Path pathToDir(Path path, boolean isDir) {
        return isDir ? path : path.getParent();
    }


    @Value(staticConstructor = "of")
    public static class AffectedConfigs {
        List<AffectedAYaml> yamls;
        Set<Path> aYmlNotYamls;

        public AffectedConfigs merge(Collection<AffectedAYaml> otherYamls) {
            var exists = yamls.stream()
                    .map(AffectedAYaml::getPath)
                    .collect(Collectors.toSet());

            var updated = new ArrayList<>(yamls);

            otherYamls.stream()
                    .filter(yaml -> !exists.contains(yaml.getPath()))
                    .forEach(updated::add);

            if (updated.size() == yamls.size()) {
                return this;
            }

            return AffectedConfigs.of(updated, aYmlNotYamls);
        }
    }

    public void reset() {
        configCache.invalidateAll();
    }

    @Value(staticConstructor = "of")
    private static class ConfigKey {
        Path path;
        ArcRevision revision;
    }
}
