package ru.yandex.ci.engine.registry;

import java.nio.file.Path;
import java.time.Duration;
import java.time.Instant;
import java.util.HashMap;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.ExecutionException;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.google.common.base.Preconditions;
import com.google.common.cache.CacheBuilder;
import com.google.common.cache.CacheLoader;
import com.google.common.cache.LoadingCache;
import com.google.common.util.concurrent.UncheckedExecutionException;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;
import org.yaml.snakeyaml.error.YAMLException;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.CommitId;
import ru.yandex.ci.core.config.a.AffectedAYamlsFinder;
import ru.yandex.ci.core.config.registry.TaskConfig;
import ru.yandex.ci.core.config.registry.TaskConfigYamlParser;
import ru.yandex.ci.core.config.registry.TaskId;
import ru.yandex.ci.core.config.registry.TaskRegistryException;
import ru.yandex.ci.core.config.registry.Type;
import ru.yandex.ci.core.launch.TaskVersion;
import ru.yandex.ci.core.registry.RegistryTask;
import ru.yandex.ci.core.tasklet.TaskletMetadata;
import ru.yandex.ci.core.tasklet.TaskletRuntime;
import ru.yandex.ci.core.taskletv2.TaskletV2Metadata;
import ru.yandex.ci.util.ExceptionUtils;

@Slf4j
public class TaskRegistryImpl implements TaskRegistry {
    private static final String REGISTRY_VCS_ROOT = "ci/registry";
    private static final String SUPPORTED_EXTENSION = ".yaml";

    private static final Duration TASK_CONFIG_CACHE_EXPIRATION = Duration.ofDays(1);
    private static final int TASK_CONFIG_CACHE = 10_000;

    public static final String COMMON_REGISTRY_PREFIX = "common/";
    private static final Instant DO_VALIDATION_SINCE = Instant.parse("2022-06-16T12:00:00.00Z");

    private final ArcService arcService;
    private final LoadingCache<ConfigKey, TaskConfig> configCache;

    public TaskRegistryImpl(ArcService arcService, @Nullable MeterRegistry meterRegistry) {
        this.arcService = arcService;
        this.configCache = CacheBuilder.newBuilder()
                .expireAfterAccess(TASK_CONFIG_CACHE_EXPIRATION)
                .maximumSize(TASK_CONFIG_CACHE)
                .recordStats()
                .build(new CacheLoader<>() {
                    @Override
                    public TaskConfig load(@Nonnull ConfigKey key) throws Exception {
                        return loadConfig(key.getRevision(), key.getTaskId());
                    }
                });
        if (meterRegistry != null) {
            GuavaCacheMetrics.monitor(meterRegistry, configCache, "task-registry-cache");
        }
    }

    @Override
    public TaskConfig lookup(ArcRevision revision, TaskId taskId) throws JsonProcessingException {
        try {
            return configCache.get(ConfigKey.of(revision, taskId));
        } catch (UncheckedExecutionException | ExecutionException e) {
            Throwable cause = e.getCause();
            if (cause instanceof JsonProcessingException) {
                throw (JsonProcessingException) cause;
            }
            if (cause instanceof TaskRegistryException) {
                throw (TaskRegistryException) cause;
            }
            throw ExceptionUtils.unwrap(e);
        }
    }

    @Override
    public Map<TaskId, TaskConfig> loadRegistry(CommitId commitId) {
        var registryYamls = arcService.listDir(REGISTRY_VCS_ROOT, commitId, true, true)
                .stream()
                .filter(p -> p.toString().endsWith(SUPPORTED_EXTENSION))
                .filter(p -> !p.endsWith(AffectedAYamlsFinder.CONFIG_FILE_NAME))
                .filter(p -> !p.endsWith("example-a.yaml"))
                .toList();
        log.info("Loaded {} yaml paths in {}", registryYamls.size(), REGISTRY_VCS_ROOT);

        Map<TaskId, TaskConfig> configs = new HashMap<>(registryYamls.size());
        for (Path path : registryYamls) {
            var taskId = TaskRegistryImpl.taskIdFromPath(Path.of(REGISTRY_VCS_ROOT).resolve(path));
            try {
                var config = loadConfig(commitId, taskId);
                configs.put(taskId, config);
            } catch (JsonProcessingException | TaskRegistryException | YAMLException e) {
                log.info("Failed to parse task {}", taskId, e);
            }
        }
        return configs;
    }

    @Override
    public Map<TaskId, TaskConfig> loadRegistry() {
        var trunkHead = arcService.getLastRevisionInBranch(ArcBranch.trunk());
        return loadRegistry(trunkHead);
    }

    public static TaskId taskIdFromPath(Path pathToYaml) {
        Preconditions.checkArgument(pathToYaml.toString().endsWith(SUPPORTED_EXTENSION));
        Preconditions.checkArgument(pathToYaml.startsWith(REGISTRY_VCS_ROOT));

        var relative = Path.of(REGISTRY_VCS_ROOT).relativize(pathToYaml);
        var filename = relative.getFileName().toString();
        var taskPath = relative.resolveSibling(filename.substring(0, filename.length() - SUPPORTED_EXTENSION.length()));
        return TaskId.of(taskPath.toString());
    }

    private TaskConfig loadConfig(CommitId revision, TaskId taskId) throws JsonProcessingException {
        Path vcsPath = getVcsPath(taskId);

        Optional<String> contentOptional;
        try {
            contentOptional = arcService.getFileContent(vcsPath, revision);
        } catch (RuntimeException e) {
            throw new RuntimeException(
                    "yaml for " + taskId.getId() +
                            " expected at " + vcsPath +
                            " in revision " + revision.getCommitId() +
                            " failed to load", e
            );
        }

        var content = contentOptional.orElseThrow(() -> new TaskRegistryException(
                "yaml for " + taskId.getId() +
                        " expected at " + vcsPath +
                        " in revision " + revision.getCommitId() +
                        " not found"
        ));

        var commit = arcService.getCommit(revision);
        var skipValidation = commit.getCreateTime().isBefore(DO_VALIDATION_SINCE);
        return TaskConfigYamlParser.parse(content, skipValidation);
    }

    public static Path getVcsPath(TaskId taskId) {
        return Path.of(REGISTRY_VCS_ROOT + "/" + taskId.getId() + SUPPORTED_EXTENSION);
    }

    public static TaskletMetadata.Id getTaskletKey(TaskConfig taskConfig, TaskVersion version) {
        var sandboxResourceIdStr = Preconditions.checkNotNull(
                taskConfig.getVersions().get(version),
                "Version %s not found in config %s", version, taskConfig
        );
        long sandboxResourceId;
        try {
            sandboxResourceId = Long.parseLong(sandboxResourceIdStr);
        } catch (NumberFormatException e) {
            throw new RuntimeException("Unable to parse sandbox resource as number: " + sandboxResourceIdStr, e);
        }
        var tasklet = taskConfig.getTasklet();
        Preconditions.checkState(tasklet != null, "tasklet must not be null");
        return TaskletMetadata.Id.of(
                tasklet.getImplementation(),
                TaskletRuntime.SANDBOX,
                sandboxResourceId
        );
    }

    public static TaskletV2Metadata.Description getTaskletV2Description(TaskConfig taskConfig, TaskVersion version) {
        var label = Preconditions.checkNotNull(
                taskConfig.getVersions().get(version),
                "Version %s not found in config %s", version, taskConfig
        );
        var taskletV2 = taskConfig.getTaskletV2();
        Preconditions.checkState(taskletV2 != null, "tasklet-v2 must not be null");
        return TaskletV2Metadata.Description.of(
                taskletV2.getNamespace(),
                taskletV2.getTasklet(),
                label
        );
    }

    public static RegistryTask from(TaskId taskId, TaskVersion version, TaskConfig config) {
        var type = Type.of(config, taskId);
        var builder = RegistryTask.builder()
                .id(RegistryTask.Id.of(taskId, version))
                .type(type)
                .rollbackMode(config.getAutoRollbackMode());
        switch (type) {
            case TASKLET -> builder.taskletMetadataId(getTaskletKey(config, version));
            case TASKLET_V2 -> builder.taskletV2MetadataDescription(getTaskletV2Description(config, version));
            case SANDBOX_TASK -> {
                Preconditions.checkNotNull(config.getSandboxTask());
                builder.sandboxTaskName(config.getSandboxTask().getName());
                builder.sandboxTemplateName(config.getSandboxTask().getTemplate());
            }
            default -> throw new IllegalArgumentException(
                    "unexpected task type %s. Expected only tasklets and sandbox tasks".formatted(type)
            );
        }
        return builder.build();
    }

    @Value(staticConstructor = "of")
    private static class ConfigKey {
        @Nonnull
        ArcRevision revision;
        @Nonnull
        TaskId taskId;
    }
}
