package ru.yandex.ci.client.arcanum;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonEnumDefaultValue;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.AllArgsConstructor;
import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@AllArgsConstructor
@Value
public class ArcanumMergeRequirementDto {
    String system;
    String type;
    boolean required;
    boolean satisfied;

    @Nullable
    Status status;

    @Nullable
    String systemCheckUri;
    @Nullable
    String systemCheckId;
    @Nullable
    DisablingPolicyDto disablingPolicy;
    @Nullable
    String disableReason;

    public ArcanumMergeRequirementDto(ArcanumMergeRequirementId requirementId,
                                      boolean required,
                                      boolean satisfied,
                                      @Nullable Status status,
                                      @Nullable String systemCheckUri,
                                      @Nullable String systemCheckId) {
        this(
                requirementId.getSystem(),
                requirementId.getType(),
                required,
                satisfied,
                status,
                systemCheckUri,
                systemCheckId,
                null,
                null
        );
    }

    public ArcanumMergeRequirementDto(ArcanumMergeRequirementId requirementId,
                                      boolean required,
                                      @Nullable String disableReason) {
        this(
                requirementId.getSystem(),
                requirementId.getType(),
                required,
                false,
                null,
                null,
                null,
                null,
                disableReason
        );
    }

    public ArcanumMergeRequirementDto(ArcanumMergeRequirementId requirementId, @Nullable Status status) {
        this(requirementId, false, false, status, null, null);
    }

    public ArcanumMergeRequirementDto(ArcanumMergeRequirementId requirementId,
                                      @Nullable Status status, @Nullable String systemCheckUri) {
        this(requirementId, false, false, status, systemCheckUri, null);
    }

    @Persisted
    public enum Status {
        @JsonProperty("cancelled")
        CANCELLED,
        @JsonProperty("success")
        SUCCESS,
        @JsonProperty("pending")
        PENDING,
        @JsonProperty("error")
        ERROR,
        @JsonProperty("failure")
        FAILURE,
        @JsonProperty("skipped")
        SKIPPED,

        @JsonEnumDefaultValue
        @JsonProperty("unknown")
        UNKNOWN
    }
}
