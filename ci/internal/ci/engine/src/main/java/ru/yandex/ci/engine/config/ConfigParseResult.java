package ru.yandex.ci.engine.config;

import java.util.Collections;
import java.util.List;
import java.util.NavigableMap;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.Value;

import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.config.ConfigProblem;
import ru.yandex.ci.core.config.a.model.AYamlConfig;
import ru.yandex.ci.core.config.registry.TaskConfig;
import ru.yandex.ci.core.config.registry.TaskId;

@Value
@AllArgsConstructor(access = AccessLevel.PRIVATE)
public class ConfigParseResult {
    @Nullable
    AYamlConfig aYamlConfig;
    @Nonnull
    NavigableMap<TaskId, TaskConfig> taskConfigs;
    @Nonnull
    NavigableMap<TaskId, ArcRevision> taskRevisions;
    @Nonnull
    List<ConfigProblem> problems;
    @Nonnull
    Status status;

    public static ConfigParseResult create(
            @Nullable AYamlConfig aYamlConfig,
            NavigableMap<TaskId, TaskConfig> taskConfigs,
            NavigableMap<TaskId, ArcRevision> taskRevisions,
            List<ConfigProblem> problems) {

        Status status = ConfigProblem.isValid(problems) ? Status.VALID : Status.INVALID;

        if (status == Status.VALID) {
            Preconditions.checkState(aYamlConfig != null);
        }

        return new ConfigParseResult(
                aYamlConfig,
                taskConfigs,
                taskRevisions,
                problems,
                status
        );
    }

    public static ConfigParseResult singleCrit(
            String title,
            String description) {
        return new ConfigParseResult(
                null,
                Collections.emptyNavigableMap(),
                Collections.emptyNavigableMap(),
                List.of(ConfigProblem.crit(title, description)),
                Status.INVALID
        );
    }

    public static ConfigParseResult crit(
            List<ConfigProblem> configProblems) {
        Preconditions.checkArgument(!ConfigProblem.isValid(configProblems));

        return new ConfigParseResult(
                null,
                Collections.emptyNavigableMap(),
                Collections.emptyNavigableMap(),
                List.copyOf(configProblems),
                Status.INVALID
        );
    }

    public static ConfigParseResult notCi(
            AYamlConfig aYamlConfig) {
        return new ConfigParseResult(
                aYamlConfig,
                Collections.emptyNavigableMap(),
                Collections.emptyNavigableMap(),
                List.of(),
                Status.NOT_CI
        );
    }

    public enum Status {
        VALID,
        INVALID,
        NOT_CI
    }
}
