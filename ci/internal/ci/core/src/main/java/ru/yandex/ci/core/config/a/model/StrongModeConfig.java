package ru.yandex.ci.core.config.a.model;

import java.util.Set;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Builder;
import lombok.Value;

@Value
@Builder
@JsonDeserialize(builder = StrongModeConfig.Builder.class)
public class StrongModeConfig {
    public static final Set<String> DEFAULT_ABC_SCOPES = Set.of("development");

    @JsonProperty
    boolean enabled;

    @JsonProperty("abc-scopes")
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    Set<String> abcScopes;

    public static StrongModeConfig of(boolean enabled) {
        return new Builder(enabled).build();
    }

    public static class Builder {
        {
            abcScopes = DEFAULT_ABC_SCOPES;
        }

        public Builder() {
            this.enabled(true);
        }

        public Builder(boolean enabled) {
            this.enabled(enabled);
        }
    }
}
