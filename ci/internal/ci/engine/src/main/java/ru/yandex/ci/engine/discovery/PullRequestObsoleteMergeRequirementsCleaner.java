package ru.yandex.ci.engine.discovery;

import java.util.List;
import java.util.function.Function;
import java.util.stream.Collectors;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementDto;
import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementId;
import ru.yandex.ci.client.arcanum.ArcanumReviewDataDto;
import ru.yandex.ci.client.arcanum.DisablingPolicyDto;
import ru.yandex.ci.client.arcanum.UpdateCheckStatusRequest;
import ru.yandex.ci.client.base.http.HttpException;
import ru.yandex.ci.core.pr.MergeRequirementsCollector;
import ru.yandex.ci.engine.pr.PullRequestService;

@Slf4j
public class PullRequestObsoleteMergeRequirementsCleaner {

    private final PullRequestService pullRequestService;

    public PullRequestObsoleteMergeRequirementsCleaner(PullRequestService pullRequestService) {
        this.pullRequestService = pullRequestService;
    }

    public void clean(long pullRequestId, long diffSetId, MergeRequirementsCollector mergeRequirementsCollector) {
        var pullRequestData = pullRequestService.getActiveDiffSetAndMergeRequirements(pullRequestId);
        if (pullRequestData.isEmpty()) {
            throw new IllegalStateException("not found pull request %d in arcanum api"
                    .formatted(pullRequestId));
        }
        long activeDiffSetId = pullRequestData.map(ArcanumReviewDataDto::getActiveDiffSet)
                .map(ArcanumReviewDataDto.DiffSet::getId)
                .orElseThrow(() -> new IllegalStateException(
                        "active diff set is null in pull request %d".formatted(pullRequestId)
                ));
        if (activeDiffSetId != diffSetId) {
            log.info("skip removing obsolete ci merge requirements for diff set {}, cause active diff set is {}",
                    diffSetId, activeDiffSetId);
            return;
        }

        String ciSystemInMergeRequirements = pullRequestService.getMergeRequirementSystem();

        var mergeRequirements = mergeRequirementsCollector.getMergeRequirements();

        log.info("collected actual merge requirements: pull request {}, diff set {}, {}",
                pullRequestId, diffSetId, mergeRequirements);

        var actualMergeRequirements = mergeRequirementsCollector.getMergeRequirements()
                .stream()
                .filter(it -> ciSystemInMergeRequirements.equals(it.getSystem()))
                .collect(Collectors.toSet());

        log.info("actual merge requirements: pull request {}, diff set {}, {}",
                pullRequestId, diffSetId, actualMergeRequirements);

        var currentPrRequirements = pullRequestData.map(ArcanumReviewDataDto::getChecks).orElse(List.of());
        var obsoleteRequirementsCanBeDisabled = currentPrRequirements.stream()
                .filter(it -> ciSystemInMergeRequirements.equals(it.getSystem()))
                .filter(it -> it.getDisablingPolicy() != DisablingPolicyDto.DENIED)
                .map(it -> ArcanumMergeRequirementId.of(it.getSystem(), it.getType()))
                .filter(it -> !actualMergeRequirements.contains(it))
                .collect(Collectors.toMap(
                        Function.identity(), it -> false
                ));

        log.info("found obsolete merge requirements (can be disabled): pull request {}, diff set {}, {}",
                pullRequestId, diffSetId, obsoleteRequirementsCanBeDisabled.keySet());
        if (!obsoleteRequirementsCanBeDisabled.isEmpty()) {
            try {
                pullRequestService.setMergeRequirementsWithCheckingActiveDiffSet(pullRequestId, diffSetId,
                        obsoleteRequirementsCanBeDisabled,
                        "not required in the latest iteration (diffSetId " + diffSetId + ")");
            } catch (HttpException e) {
                if (e.getHttpCode() == 400) {
                    log.info("failed disable obsolete merge requirements: {}", e.getMessage());
                } else {
                    throw e;
                }
            }
        }

        var obsoleteRequirementsCanBeSkipped = currentPrRequirements.stream()
                .filter(it -> ciSystemInMergeRequirements.equals(it.getSystem()))
                .map(it -> ArcanumMergeRequirementId.of(it.getSystem(), it.getType()))
                .filter(it -> !actualMergeRequirements.contains(it))
                .collect(Collectors.toSet());

        log.info("found obsolete merge requirements (can be skipped): pull request {}, diff set {}, {}",
                pullRequestId, diffSetId, obsoleteRequirementsCanBeDisabled.keySet());
        if (!obsoleteRequirementsCanBeSkipped.isEmpty()) {
            for (var requirementId : obsoleteRequirementsCanBeSkipped) {
                pullRequestService.sendMergeRequirementStatus(
                        pullRequestId, diffSetId,
                        UpdateCheckStatusRequest.builder()
                                .requirementId(requirementId)
                                .status(ArcanumMergeRequirementDto.Status.SKIPPED)
                                .build()
                );
            }
        }
    }

}
