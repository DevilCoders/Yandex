package ru.yandex.ci.core.config.a.model.auto;

import java.util.List;
import java.util.Objects;

import javax.annotation.Nonnull;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonSetter;
import com.fasterxml.jackson.annotation.Nulls;
import lombok.Value;

import static java.util.Collections.emptyList;

@Value
public class AutoReleaseConfig {
    public static final AutoReleaseConfig EMPTY = new AutoReleaseConfig(false, List.of());
    /* See ManualConfig as an example of realisation that supports two ways of yaml configuration:
      1. manual: true
      2. manual:
           prompt: Точно катим?
           enabled: true
      This approach can be used to add auto release condition fields to AutoReleaseConfig. */
    boolean enabled;

    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    @JsonSetter(nulls = Nulls.AS_EMPTY)
    @Nonnull
    List<Conditions> conditions;

    @JsonCreator
    public AutoReleaseConfig(boolean enabled) {
        this(enabled, emptyList());
    }

    public AutoReleaseConfig(@JsonProperty("enabled") Boolean enabled,
                             @JsonProperty("conditions") Conditions conditions) {
        this(enabled, List.of(conditions));
    }

    @JsonCreator
    public AutoReleaseConfig(@JsonProperty("enabled") Boolean enabled,
                             @JsonProperty("conditions") List<Conditions> conditions) {
        this.enabled = Objects.requireNonNullElse(enabled, true);
        this.conditions = conditions;
    }
}
