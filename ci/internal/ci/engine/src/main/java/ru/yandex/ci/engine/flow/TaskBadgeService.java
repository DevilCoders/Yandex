package ru.yandex.ci.engine.flow;

import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.collect.Streams;
import com.google.gson.Gson;
import lombok.RequiredArgsConstructor;
import lombok.Value;
import lombok.With;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.sandbox.api.SandboxTaskOutput;
import ru.yandex.ci.core.config.registry.SandboxTaskBadgesConfig;
import ru.yandex.ci.core.job.JobInstance;
import ru.yandex.ci.engine.SandboxTaskService;
import ru.yandex.ci.flow.engine.runtime.FullJobLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.JobProgressService;
import ru.yandex.ci.flow.engine.runtime.state.model.TaskBadge;

@Slf4j
@RequiredArgsConstructor
public class TaskBadgeService {

    /**
     * Префикс названий output параметров, содержащих ссылки-отчеты.
     * Может использоваться в sandbox задачах.
     */
    private static final String TASK_BADGE_FIELD_PREFIX = "_ci:task-badge:";

    private static final Gson GSON = SandboxTaskService.GSON;

    @Nonnull
    private final JobProgressService jobProgressService;

    public void updateTaskBadges(JobInstance instance, @Nullable List<TaskBadge> taskBadges) {
        if (taskBadges == null || taskBadges.isEmpty()) {
            return;
        }

        var jobLaunchId = FullJobLaunchId.of(instance.getId());
        for (var taskState : taskBadges) {
            jobProgressService.changeTaskBadge(jobLaunchId, taskState);
        }
    }


    public List<TaskBadge> toTaskBadges(SandboxTaskOutput sandboxTaskOutput,
                                        List<SandboxTaskBadgesConfig> badgesConfigs,
                                        String sbTaskUrl) {
        Map<String, SandboxTaskBadgesConfig> taskBadgesConfigByIds = badgesConfigs.isEmpty()
                ? Map.of()
                : getTaskBadgesConfigsByIds(badgesConfigs);

        return List.copyOf(
                Streams.concat(
                        getPredefinedTaskBadges(taskBadgesConfigByIds, sandboxTaskOutput),
                        getAdditionalTaskBadges(sandboxTaskOutput),
                        getTaskBadgesFromReports(taskBadgesConfigByIds, sandboxTaskOutput.getReports(), sbTaskUrl)
                )
                        .collect(Collectors.toMap(TaskBadge::getId, Function.identity(), (pre, add) -> pre))
                        .values()
        );
    }

    private Map<String, SandboxTaskBadgesConfig> getTaskBadgesConfigsByIds(
            @Nullable List<SandboxTaskBadgesConfig> taskBadgesConfigs
    ) {
        if (taskBadgesConfigs == null || taskBadgesConfigs.isEmpty()) {
            return Map.of();
        }

        return taskBadgesConfigs.stream()
                .collect(Collectors.toMap(SandboxTaskBadgesConfig::getId, Function.identity(), (c1, c2) -> c1));
    }

    private Stream<TaskBadge> getTaskBadgesFromReports(Map<String, SandboxTaskBadgesConfig> taskBadgesConfigsByIds,
                                                       List<SandboxTaskOutput.Report> reports,
                                                       String sbTaskUrl) {
        return reports.stream()
                .filter(r -> r.getLabel() != null && !r.getLabel().isEmpty())
                .filter(r -> taskBadgesConfigsByIds.containsKey(r.getLabel()))
                .map(r -> TaskBadge.of(
                        r.getLabel(),
                        Objects.requireNonNullElse(taskBadgesConfigsByIds.get(r.getLabel()).getModule(), "SANDBOX"),
                        sbTaskUrl + "/" + r.getLabel(),
                        TaskBadge.TaskStatus.SUCCESSFUL
                ));
    }

    private Stream<TaskBadge> getPredefinedTaskBadges(Map<String, SandboxTaskBadgesConfig> taskBadgesConfigsByIds,
                                                      SandboxTaskOutput sandboxTaskOutput) {
        Map<String, String> taskBadgeRawMessages = taskBadgesConfigsByIds
                .entrySet().stream()
                .filter(e -> sandboxTaskOutput.getOutputParameter(e.getKey(), Object.class).isPresent())
                .collect(Collectors.toMap(
                        Map.Entry::getKey,
                        e -> GSON.toJson(sandboxTaskOutput.getOutputParameter(e.getKey(), Object.class).orElseThrow())
                ));

        log.info(
                "Found {} predefined task report states in output parameters: {}",
                taskBadgeRawMessages.size(), taskBadgeRawMessages
        );

        return taskBadgeRawMessages.entrySet().stream()
                .map(e -> toTaskBadge(e.getKey(), e.getValue(), taskBadgesConfigsByIds))
                // Пропускаем некорректные TaskBadge (null)
                .filter(Objects::nonNull);
    }

    private Stream<TaskBadge> getAdditionalTaskBadges(SandboxTaskOutput sandboxTaskOutput) {
        Map<String, String> taskBadgeRawMessages = sandboxTaskOutput.getOutputParameters()
                .entrySet().stream()
                .filter(e -> e.getKey().startsWith(TASK_BADGE_FIELD_PREFIX))
                .collect(Collectors.toMap(
                        e -> e.getKey().substring(TASK_BADGE_FIELD_PREFIX.length()),
                        e -> GSON.toJson(e.getValue())
                ));

        log.info(
                "Found {} task report states in output parameters: {}",
                taskBadgeRawMessages.size(), taskBadgeRawMessages
        );

        return taskBadgeRawMessages.entrySet().stream()
                .map(e -> toTaskBadge(e.getKey(), e.getValue(), Map.of()))
                // Пропускаем некорректные TaskBadge (null)
                .filter(Objects::nonNull);
    }

    @Nullable
    private TaskBadge toTaskBadge(String id,
                                  String taskBadgeRawMessage,
                                  Map<String, SandboxTaskBadgesConfig> taskBadgeConfigMap) {
        try {
            TaskBadgeMessage taskBadgeMessage = GSON.fromJson(taskBadgeRawMessage, TaskBadgeMessage.class);

            if (id.isEmpty()) {
                id = TaskBadge.DEFAULT_ID;
            }
            // запрещаем обновлять статусы, которые проставляет сам ci
            TaskBadge.checkNotReservedId(id);

            // Для определенных в реестре sandbox task отчетов
            if (taskBadgeConfigMap.containsKey(id) && taskBadgeConfigMap.get(id).getModule() != null) {
                taskBadgeMessage = taskBadgeMessage.withModule(taskBadgeConfigMap.get(id).getModule());
            }

            return TaskBadge.of(
                    id,
                    taskBadgeMessage.getModule(),
                    taskBadgeMessage.getUrl(),
                    TaskBadge.TaskStatus.valueOf(taskBadgeMessage.getStatus()),
                    taskBadgeMessage.getProgress(),
                    taskBadgeMessage.getText(),
                    false
            );
        } catch (Exception ex) {
            // Логируем ошибки маппинга в TaskBadge, в случае некорректных данных возвращаем null
            log.error(
                    String.format(
                            "Unable map TaskBadgeMessage %s with id %s to TaskBadge, skipping...",
                            taskBadgeRawMessage, id
                    ),
                    ex
            );
        }
        return null;
    }

    @SuppressWarnings("ReferenceEquality")
    @Value
    public static class TaskBadgeMessage {
        @With
        String module;
        String url;
        String status;
        Float progress;
        String text;
    }


}
