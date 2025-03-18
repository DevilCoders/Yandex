package ru.yandex.ci.core.config.a.model;

import java.util.List;
import java.util.Objects;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Builder;
import lombok.Singular;
import lombok.Value;

@Value
@Builder
@JsonDeserialize(builder = ManualConfig.Builder.class)
public class ManualConfig {
    @JsonProperty
    boolean enabled;

    @JsonProperty
    String prompt;

    @Singular
    @JsonProperty
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    List<PermissionRule> approvers;

    public List<PermissionRule> getApprovers() {
        return Objects.requireNonNullElse(approvers, List.of());
    }

    public static ManualConfig of(boolean enabled) {
        return new Builder(enabled).build();
    }

    @JsonIgnoreProperties(ignoreUnknown = true)
    public static class Builder {
        public Builder() {
            this.enabled(true);
        }

        public Builder(boolean enabled) {
            this.enabled(enabled);
        }

        public Builder abcService(String abcService) {
            return approver(PermissionRule.ofScopes(abcService));
        }
    }
}
