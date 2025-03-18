package ru.yandex.ci.core.project;

import java.util.List;
import java.util.Objects;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.google.common.collect.Multimap;
import lombok.Builder;
import lombok.Singular;
import lombok.Value;

import ru.yandex.ci.core.config.a.model.ActionConfig;
import ru.yandex.ci.core.config.a.model.BinarySearchConfig;
import ru.yandex.ci.core.config.a.model.FlowConfig;
import ru.yandex.ci.core.config.a.model.TrackerWatchConfig;
import ru.yandex.ci.core.config.a.model.TriggerConfig;
import ru.yandex.ci.ydb.Persisted;
import ru.yandex.lang.NonNullApi;

@SuppressWarnings("ReferenceEquality")
@JsonIgnoreProperties(ignoreUnknown = true) // permissions
@Persisted
@Value
@Builder(toBuilder = true)
@NonNullApi
public class ActionConfigState {
    @Nonnull
    String flowId; // Could be flowId or actionId

    @Nonnull
    String title;

    @Nullable
    String description;

    @Nonnull
    @Singular
    List<TriggerConfig> triggers;

    @Nullable
    Boolean showInActions;

    @Nullable
    Integer maxActiveCount;

    @Nullable
    Integer maxStartPerMinute;

    @Nullable
    BinarySearchConfig binarySearchConfig;

    @Nullable
    TrackerWatchConfig trackerWatchConfig;

    @Nullable
    TestId testId;

    public static List<ActionConfigState> of(FlowConfig flowConfig, Multimap<String, ActionConfig> flowToActions) {
        var actions = flowToActions.get(flowConfig.getId());
        if (actions.isEmpty()) {
            return List.of(ActionConfigState.builder()
                    .flowId(flowConfig.getId())
                    .title(flowConfig.getTitle())
                    .showInActions(flowConfig.getShowInActions())
                    .triggers(List.of())
                    .build());
        } else {
            return actions.stream()
                    .map(action -> ActionConfigState.builder()
                            .flowId(action.getId())
                            .title(Objects.requireNonNullElse(action.getTitle(), flowConfig.getTitle()))
                            .description(action.getDescription())
                            .showInActions(action.isVirtual()
                                    ? flowConfig.getShowInActions()
                                    : Boolean.TRUE)
                            .triggers(action.getTriggers())
                            .maxActiveCount(action.getMaxActiveCount())
                            .maxStartPerMinute(action.getMaxStartPerMinute())
                            .binarySearchConfig(action.getBinarySearchConfig())
                            .trackerWatchConfig(action.getTrackerWatchConfig())
                            .build())
                    .toList();
        }
    }

    @Persisted
    @Value(staticConstructor = "of")
    public static class TestId {
        String suiteId;
        String toolchain;
    }
}
