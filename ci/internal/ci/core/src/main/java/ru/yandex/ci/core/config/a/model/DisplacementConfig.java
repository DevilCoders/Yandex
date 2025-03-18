package ru.yandex.ci.core.config.a.model;

import java.util.Set;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.AllArgsConstructor;
import lombok.Value;

import ru.yandex.ci.core.launch.LaunchState.Status;

@Value
@AllArgsConstructor
public class DisplacementConfig {
    // Supported options from ru.yandex.ci.flow.engine.runtime.state.model.LaunchState only (except IDLE)
    private static final Set<Status> DEFAULT_OPTIONS = Set.of(
            Status.WAITING_FOR_MANUAL_TRIGGER,
            Status.WAITING_FOR_STAGE);

    @JsonProperty("on-status")
    Set<Status> onStatus;

    // No property "enabled" defined, has to explicitly provide this mode
    @JsonCreator(mode = JsonCreator.Mode.DELEGATING)
    public DisplacementConfig(boolean enabled) {
        this(enabled ? DEFAULT_OPTIONS : Set.of());
    }

}
