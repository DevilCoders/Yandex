package ru.yandex.ci.client.arcanum;

import java.time.Duration;
import java.util.List;
import java.util.Optional;
import java.util.Set;

import javax.annotation.Nullable;

import lombok.AccessLevel;
import lombok.RequiredArgsConstructor;
import lombok.Value;

import ru.yandex.ci.client.base.http.RetryPolicies;
import ru.yandex.ci.client.base.http.RetryPolicy;

public interface ArcanumClient {

    static RetryPolicy defaultRetryPolicy() {
        return RetryPolicies.requireAll(
                RetryPolicies.nonRetryableResponseCodes(Set.of(400, 404)),
                RetryPolicies.idempotentMethodsOnly(),
                RetryPolicies.retryWithSleep(3, Duration.ofSeconds(1))
        );
    }

    void setMergeRequirementStatus(
            long reviewRequestId,
            long diffSetId,
            UpdateCheckStatusRequest build
    );

    void setMergeRequirementsWithCheckingActiveDiffSet(
            long reviewRequestId,
            long diffSetId,
            List<UpdateCheckRequirementRequestDto> requirements);

    void setMergeRequirement(long reviewRequestId,
                             ArcanumMergeRequirementId requirementId,
                             Require require);

    Optional<ArcanumReviewDataDto> getReviewRequest(long reviewRequestId);

    GroupsDto getGroups();

    Optional<ArcanumReviewDataDto> getReviewSummaryAndDescription(long reviewRequestId);

    @Value
    @RequiredArgsConstructor(access = AccessLevel.PRIVATE)
    class Require {
        boolean required;
        @Nullable
        String disableReason;

        public static Require notRequired(String disableReason) {
            return new Require(false, disableReason);
        }

        public static Require required() {
            return new Require(true, null);
        }
    }
}
