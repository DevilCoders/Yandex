package ru.yandex.ci.engine.config;

import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.NavigableMap;
import java.util.Optional;
import java.util.Set;
import java.util.TreeMap;
import java.util.TreeSet;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.github.fge.jsonschema.core.exceptions.ProcessingException;
import com.google.common.base.Preconditions;
import com.google.common.base.Stopwatch;
import com.google.common.collect.Sets;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.yaml.snakeyaml.error.YAMLException;

import ru.yandex.ci.core.abc.AbcService;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.config.ConfigProblem;
import ru.yandex.ci.core.config.Validation;
import ru.yandex.ci.core.config.a.AYamlService;
import ru.yandex.ci.core.config.a.model.AYamlConfig;
import ru.yandex.ci.core.config.a.model.CiConfig;
import ru.yandex.ci.core.config.a.model.JobConfig;
import ru.yandex.ci.core.config.a.validation.AYamlValidationException;
import ru.yandex.ci.core.config.a.validation.AYamlValidationInternalException;
import ru.yandex.ci.core.config.registry.TaskConfig;
import ru.yandex.ci.core.config.registry.TaskId;
import ru.yandex.ci.core.config.registry.TaskRegistryException;
import ru.yandex.ci.core.config.registry.TaskRegistryValidationException;
import ru.yandex.ci.engine.config.validation.InputOutputTaskValidator;
import ru.yandex.ci.engine.registry.TaskRegistry;
import ru.yandex.ci.engine.registry.TaskRegistryImpl;
import ru.yandex.lang.NonNullApi;

@Slf4j
@NonNullApi
@RequiredArgsConstructor
public class ConfigParseService {

    @Nonnull
    private final ArcService arcService;
    @Nonnull
    private final AYamlService aYamlService;
    @Nonnull
    private final TaskRegistry taskRegistry;
    @Nonnull
    private final InputOutputTaskValidator taskValidator;
    @Nonnull
    private final AbcService abcService;
    private final boolean alwaysValidateAbcSlug;


    /**
     * Parse and validate all found configurations
     *
     * @param configPath    path to configs
     * @param revision      arc revision
     * @param taskRevisions can be null. Will be calculated then.
     * @return parse result
     */
    public ConfigParseResult parseAndValidate(
            Path configPath,
            ArcRevision revision,
            @Nullable NavigableMap<TaskId, ArcRevision> taskRevisions
    ) {
        try {
            return doParseAndValidate(configPath, revision, taskRevisions);
        } catch (Exception e) {
            log.error(
                    "Internal CI error while processing config {} on revision {} with taskRevisions {}",
                    configPath, revision, taskRevisions, e
            );
            throw e;
        }
    }

    private ConfigParseResult doParseAndValidate(
            Path configPath,
            ArcRevision revision,
            @Nullable NavigableMap<TaskId, ArcRevision> taskRevisions
    ) {
        var stopWatch = Stopwatch.createStarted();
        AYamlConfig aYamlConfig;
        try {
            aYamlConfig = aYamlService.getConfig(configPath, revision);
        } catch (ProcessingException e) {
            return ConfigParseResult.singleCrit("Failed to validate a.yaml", e.getMessage());
        } catch (JsonProcessingException | YAMLException e) {
            return ConfigParseResult.singleCrit("Failed to parse a.yaml", e.getMessage());
        } catch (AYamlValidationException e) {
            List<String> configProblems = new ArrayList<>(
                    e.getValidationReport().getSchemaReportMessages()
            );
            configProblems.addAll(e.getValidationReport().getStaticErrors());

            e.getValidationReport().getFlowReport().getJobErrors()
                    .forEach((jobId, errors) ->
                            configProblems.add(String.format(
                                    "invalid job '%s' configuration: %s", jobId, errors
                            )));

            return ConfigParseResult.crit(
                    configProblems.stream()
                            .map(it -> ConfigProblem.crit("Invalid a.yaml configuration", it))
                            .collect(Collectors.toList())
            );
        } catch (AYamlValidationInternalException e) {
            log.error("Internal error while processing config {} on revision {}", configPath, revision, e);
            return ConfigParseResult.singleCrit(
                    "Internal error while validating config. Please contact CI team",
                    e.toString()
            );
        } finally {
            log.info("Configuration {} parsed within {} msec",
                    configPath, stopWatch.stop().elapsed(TimeUnit.MILLISECONDS));
            stopWatch.start();
        }

        if (aYamlConfig.getCi() == null) {
            return ConfigParseResult.notCi(aYamlConfig);
        }

        // Always check if config has a valid ABC slug - do not accept configurations without it
        if (abcService.getService(aYamlConfig.getService()).isEmpty()) {
            // TODO: all this nonsense with commit vs revision must be fixed
            var commit = arcService.getCommit(revision);
            if (alwaysValidateAbcSlug || Validation.applySince(commit, 9030000)) {
                return ConfigParseResult.singleCrit(
                        "ABC service is invalid",
                        "Unable to find ABC service: " + aYamlConfig.getService());
            }
        }

        if (taskRevisions != null) {
            // External task list validation (make sure we didn't break the CI logic)
            Set<TaskId> missedTaskRevisions = Sets.difference(
                    getTasks(aYamlConfig.getCi()),
                    taskRevisions.keySet()
            );
            Preconditions.checkState(
                    missedTaskRevisions.isEmpty(),
                    "Not all taskRevisions provided, not found %s",
                    missedTaskRevisions
            );
        }
        if (taskRevisions == null) {
            taskRevisions = getTaskRevisions(aYamlConfig.getCi(), revision);
        }

        var tasks = loadRegistryTasks(taskRevisions);
        var problems = tasks.problems;
        var taskConfigs = tasks.taskConfigs;

        if (ConfigProblem.isValid(problems)) {
            problems.addAll(validate(configPath, aYamlConfig, taskConfigs));
        }
        problems.addAll(validateRegistryTasks(taskConfigs));

        AYamlConfig postProcessedConfig;
        if (ConfigProblem.isValid(problems)) {
            postProcessedConfig = new ConfigParsePostProcessor(aYamlConfig, taskConfigs, problems).postProcess();
        } else {
            postProcessedConfig = aYamlConfig;
        }

        var result = ConfigParseResult.create(
                postProcessedConfig,
                taskConfigs,
                taskRevisions,
                problems);
        log.info("Configuration {} prepared within {} msec in total",
                configPath, stopWatch.stop().elapsed(TimeUnit.MILLISECONDS));
        return result;
    }

    private List<ConfigProblem> validate(
            Path configPath,
            AYamlConfig aYamlConfig,
            Map<TaskId, TaskConfig> taskConfigs
    ) {
        var report = taskValidator.validate(configPath, aYamlConfig, taskConfigs);

        if (report.isValid()) {
            return List.of();
        }

        List<ConfigProblem> problems = new ArrayList<>();

        report.getViolationsByFlows().forEach(
                (reportId, violationsByJobs) -> {
                    violationsByJobs.getViolationsByJobs().forEach((job, violations) ->
                            violations.forEach(violation -> problems.add(ConfigProblem.crit(
                                    String.format(
                                            "Invalid job %s configuration in %s",
                                            job,
                                            reportId
                                    ),
                                    violation
                            )))
                    );
                    violationsByJobs.getOtherViolations().forEach(violation -> problems.add(ConfigProblem.crit(
                            String.format(
                                    "Invalid flow configuration in %s",
                                    reportId
                            ),
                            violation
                    )));
                }
        );

        var outdatedTasklets = report.getViolationsByFlows().values().stream()
                .flatMap(t -> t.getOutdatedTasklets().stream())
                .collect(Collectors.toCollection(TreeSet::new));

        if (!outdatedTasklets.isEmpty()) {
            var message = "Config uses outdated tasklets which require glycine_token." +
                    " Please update following tasklets: " +
                    String.join(", ", outdatedTasklets) + " (https://nda.ya.ru/t/UpQ8C0r54AMcui)";

            problems.add(ConfigProblem.warn("Outdated tasklets are used", message));
        }

        if (!report.getDeprecatedTasks().isEmpty()) {
            var message = report.getDeprecatedTasks().entrySet().stream()
                    .map(e -> " * Task " + e.getKey() + " is deprecated. " + e.getValue())
                    .collect(Collectors.joining("\n"));
            problems.add(ConfigProblem.warn("Deprecated tasks found", message));
        }

        return problems;
    }

    /**
     * Get revision for each task in this config
     *
     * @param ciConfig ci config
     * @param revision arc revision
     * @return null in key if task don't exists;
     */
    public NavigableMap<TaskId, ArcRevision> getTaskRevisions(CiConfig ciConfig, ArcRevision revision) {
        var taskIds = getTasks(ciConfig);
        NavigableMap<TaskId, ArcRevision> taskRevisions = new TreeMap<>();
        for (TaskId taskId : taskIds) {
            Optional<ArcCommit> commit = arcService.getLastCommit(TaskRegistryImpl.getVcsPath(taskId), revision);
            taskRevisions.put(taskId, commit.map(ArcCommit::getRevision).orElse(null));
        }
        return taskRevisions;
    }

    private RegistryTasks loadRegistryTasks(NavigableMap<TaskId, ArcRevision> taskRevisions) {
        var tasks = new RegistryTasks();
        for (Map.Entry<TaskId, ArcRevision> taskEntry : taskRevisions.entrySet()) {
            TaskId taskId = taskEntry.getKey();
            ArcRevision taskRevision = taskEntry.getValue();
            if (!taskId.requireTaskConfig()) {
                continue;
            }
            if (taskRevision == null) {
                tasks.problems.add(ConfigProblem.crit("Task " + taskId.getId() + " not found"));
                continue;
            }
            try {
                tasks.taskConfigs.put(taskId, taskRegistry.lookup(taskRevision, taskId));
            } catch (TaskRegistryValidationException e) {
                e.getValidationReport().getErrorMessages()
                        .stream()
                        .map(m ->
                                ConfigProblem.crit(
                                        "Invalid task configuration %s at %s".formatted(taskId, taskRevision), m)
                        )
                        .forEach(tasks.problems::add);
            } catch (JsonProcessingException | YAMLException | TaskRegistryException e) {
                var msg = String.format(
                        "Failed to parse Task configuration for task %s on revision %s",
                        taskId.getId(), taskRevision
                );
                log.error(msg, e);
                tasks.problems.add(ConfigProblem.crit(msg, e.getMessage()));
            }
        }
        return tasks;
    }

    private List<ConfigProblem> validateRegistryTasks(Map<TaskId, TaskConfig> taskConfigs) {
        List<ConfigProblem> problems = new ArrayList<>();

        for (var entry : taskConfigs.entrySet()) {
            var config = entry.getValue();
            var task = entry.getKey();
            if (config.getTaskType() == TaskConfig.TaskType.SANDBOX_TEMPLATE) {
                if (task.getId().startsWith(TaskRegistryImpl.COMMON_REGISTRY_PREFIX)) {
                    // Can't use sandbox templates in ci/registry/common/ block
                    problems.add(ConfigProblem.crit(String.format(
                            "Invalid registry task %s, all tasks located in %s must not use Sandbox Templates",
                            task.getId(), TaskRegistryImpl.COMMON_REGISTRY_PREFIX)));
                }
            }
        }
        return problems;
    }

    private static Set<TaskId> getTasks(CiConfig ciConfig) {
        return ciConfig.getFlows().values().stream()
                .flatMap(flow -> {
                    var allJobs = flow.getJobs();
                    var cleanupJobs = flow.getCleanupJobs();
                    if (cleanupJobs == null || cleanupJobs.isEmpty()) {
                        return allJobs.values().stream();
                    } else {
                        return Stream.of(allJobs, cleanupJobs).map(LinkedHashMap::values).flatMap(Collection::stream);
                    }
                })
                .map(JobConfig::getTaskId)
                .filter(TaskId::requireTaskConfig)
                .collect(Collectors.toSet());
    }

    private static class RegistryTasks {
        List<ConfigProblem> problems = new ArrayList<>();
        NavigableMap<TaskId, TaskConfig> taskConfigs = new TreeMap<>();
    }

}
