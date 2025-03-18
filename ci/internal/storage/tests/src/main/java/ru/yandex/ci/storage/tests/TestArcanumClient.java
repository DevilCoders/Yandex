package ru.yandex.ci.storage.tests;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;

import javax.annotation.Nullable;

import ru.yandex.ci.client.arcanum.ArcanumClient;
import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementDto;
import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementId;
import ru.yandex.ci.client.arcanum.ArcanumReviewDataDto;
import ru.yandex.ci.client.arcanum.GroupsDto;
import ru.yandex.ci.client.arcanum.UpdateCheckRequirementRequestDto;
import ru.yandex.ci.client.arcanum.UpdateCheckStatusRequest;
import ru.yandex.ci.core.pr.ArcanumCheckType;

public class TestArcanumClient implements ArcanumClient {
    private final Map<Long, Map<ArcanumCheckType, UpdateCheckStatusRequest>> reportedStatus = new HashMap<>();
    private final Map<Long, Map<ArcanumCheckType, UpdateCheckRequirementRequestDto>> reportedChecks = new HashMap<>();
    private final Map<Long, Map<ArcanumCheckType, Boolean>> reportedRequired = new HashMap<>();

    public void reset() {
        reportedStatus.clear();
        reportedChecks.clear();
        reportedRequired.clear();
    }

    @Override
    public void setMergeRequirementStatus(long pullRequestId, long diffSetId, UpdateCheckStatusRequest request) {
        var pr = reportedStatus.computeIfAbsent(pullRequestId, key -> new HashMap<>());
        pr.put(ArcanumCheckType.of(request.getType()), request);
    }

    @Override
    public void setMergeRequirementsWithCheckingActiveDiffSet(
            long pullRequestId,
            long diffSetId,
            List<UpdateCheckRequirementRequestDto> requirements
    ) {
        var pr = reportedChecks.computeIfAbsent(pullRequestId, key -> new HashMap<>());
        for (var requirement : requirements) {
            pr.put(ArcanumCheckType.of(requirement.getType()), requirement);
        }
    }

    @Override
    public void setMergeRequirement(long pullRequestId, ArcanumMergeRequirementId requirementId, Require require) {
        var pr = reportedRequired.computeIfAbsent(pullRequestId, key -> new HashMap<>());
        pr.put(ArcanumCheckType.of(requirementId.getType()), require.isRequired());
    }

    @Override
    public Optional<ArcanumReviewDataDto> getReviewRequest(long reviewRequestId) {
        return Optional.empty();
    }

    @Override
    public Optional<ArcanumReviewDataDto> getReviewSummaryAndDescription(long reviewRequestId) {
        return Optional.empty();
    }

    @Nullable
    public ArcanumMergeRequirementDto.Status status(long pullRequestId, ArcanumCheckType arcanumCheckType) {
        return getMergeRequirement(pullRequestId, arcanumCheckType)
                .map(UpdateCheckStatusRequest::getStatus)
                .orElse(null);
    }

    public Optional<UpdateCheckStatusRequest> getMergeRequirement(
            long pullRequestId, ArcanumCheckType arcanumCheckType
    ) {
        return Optional.ofNullable(reportedStatus.getOrDefault(pullRequestId, Map.of()).get(arcanumCheckType));
    }

    public Boolean required(long pullRequestId, ArcanumCheckType arcanumCheckType) {
        return reportedRequired.getOrDefault(pullRequestId, Map.of()).get(arcanumCheckType);
    }

    @Override
    public GroupsDto getGroups() {
        return new GroupsDto(List.of());
    }
}
