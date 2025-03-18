package ru.yandex.ci.core.config;

import java.util.List;
import java.util.Map;

import javax.annotation.Nonnull;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Builder;
import lombok.Singular;
import lombok.Value;

import ru.yandex.ci.core.config.a.model.PermissionRule;
import ru.yandex.ci.core.config.a.model.Permissions;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
@Builder
@JsonDeserialize(builder = ConfigPermissions.Builder.class)
public class ConfigPermissions {

    @Nonnull
    String project;

    @Singular
    @JsonProperty
    List<PermissionRule> approvals;

    @Nonnull
    @Singular
    @JsonProperty
    Map<String, Permissions> actions;

    @Nonnull
    @Singular
    @JsonProperty
    Map<String, Permissions> releases;

    @Nonnull
    @Singular
    @JsonProperty
    Map<String, FlowPermissions> flows;

    public static ConfigPermissions of(String project) {
        return builder().project(project).build();
    }


    @Persisted
    @Value
    @lombok.Builder
    @JsonDeserialize(builder = FlowPermissions.Builder.class)
    public static class FlowPermissions {

        @Nonnull
        @Singular
        @JsonProperty
        Map<String, List<PermissionRule>> jobApprovers;

        public static FlowPermissions of(Map<String, List<PermissionRule>> jobApprovers) {
            return builder().jobApprovers(jobApprovers).build();
        }
    }
}
