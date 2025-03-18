package ru.yandex.ci.core.config.a.model;

import java.util.Set;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Builder;
import lombok.Singular;
import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
@Builder
@JsonDeserialize(builder = CleanupConditionConfig.Builder.class)
public class CleanupConditionConfig {

    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    @JsonProperty
    @Singular
    Set<CleanupReason> reasons;

    @JsonProperty
    boolean cleanup;

    @JsonProperty
    boolean interrupt;

    public static class Builder {
        {
            cleanup = true;
        }
    }

    public static CleanupConditionConfig ofCleanup(CleanupReason cleanupReason) {
        return CleanupConditionConfig.builder()
                .reasons(Set.of(cleanupReason))
                .cleanup(true)
                .interrupt(false)
                .build();
    }

    public static CleanupConditionConfig ofInterrupt(CleanupReason cleanupReason) {
        return CleanupConditionConfig.builder()
                .reasons(Set.of(cleanupReason))
                .cleanup(false)
                .interrupt(true)
                .build();
    }
}
