package ru.yandex.ci.client.arcanum;

import java.util.List;

import javax.annotation.Nullable;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;

@Value
@Builder
@AllArgsConstructor
public class UpdateCheckStatusRequest {

    String system;
    String type;

    @Nullable
    ArcanumMergeRequirementDto.Status status;

    @Nullable
    String description;

    @Nullable
    String systemCheckUri;

    @Nullable
    String systemCheckId;

    @Nullable
    List<MergeIntervalDto> mergeIntervalsUtc;

    public static class Builder {
        public Builder requirementId(ArcanumMergeRequirementId requirementId) {
            this.system = requirementId.getSystem();
            this.type = requirementId.getType();
            return this;
        }
    }

}
