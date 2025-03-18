package ru.yandex.ci.core.sandbox;

import java.util.List;
import java.util.Objects;
import java.util.Set;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import lombok.Value;

import ru.yandex.ci.client.sandbox.api.ResourceState;
import ru.yandex.ci.core.config.registry.SandboxTaskBadgesConfig;
import ru.yandex.ci.ydb.Persisted;

/**
 * Данные, необходимые для запуска sandbox задачи.
 * Содержит только информацию, получаемую непосредственно из конфигурационных файлов.
 */
@JsonIgnoreProperties(ignoreUnknown = true) // ignore 'template'
@Persisted
@Value(staticConstructor = "of")
public class SandboxExecutorContext {

    @Nonnull
    String taskType;

    @Nonnull
    List<SandboxTaskBadgesConfig> badgesConfigs;

    @Nullable
    Set<ResourceState> acceptResourceStates;

    @Nullable
    Long resourceId;

    @Nullable
    SandboxTaskClass taskClass;

    public SandboxTaskClass getTaskClass() {
        return Objects.requireNonNullElse(taskClass, SandboxTaskClass.TASK);
    }

    public static SandboxExecutorContext of(String taskType) {
        return of(taskType, List.of(), Set.of(), null, SandboxTaskClass.TASK);
    }

    @Persisted
    public enum SandboxTaskClass {
        TASK, TEMPLATE
    }
}
