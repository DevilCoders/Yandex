package ru.yandex.ci.engine.discovery;

import java.util.List;
import java.util.Map;
import java.util.Optional;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.InjectMocks;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;

import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementDto;
import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementId;
import ru.yandex.ci.client.arcanum.ArcanumReviewDataDto;
import ru.yandex.ci.client.arcanum.DisablingPolicyDto;
import ru.yandex.ci.client.arcanum.UpdateCheckStatusRequest;
import ru.yandex.ci.client.base.http.HttpException;
import ru.yandex.ci.core.pr.MergeRequirementsCollector;
import ru.yandex.ci.engine.pr.PullRequestService;

import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyMap;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

@ExtendWith(MockitoExtension.class)
class PullRequestObsoleteMergeRequirementsCleanerTest {

    @Mock
    PullRequestService pullRequestService;
    @InjectMocks
    PullRequestObsoleteMergeRequirementsCleaner obsoleteMergeRequirementsCleaner;

    @Test
    void removeObsoleteCiMergeRequirements() {
        doReturn("CI").when(pullRequestService).getMergeRequirementSystem();
        var requirementsFromPreviousDiffSet = List.of(
                requirement("NOT_CI", "CHECK_1"),
                requirement("NOT_CI", "CHECK_2"),
                requirement("CI", "CHECK_1"),
                requirement("CI", "CHECK_2")
        );
        doReturn(Optional.of(
                ArcanumReviewDataDto.builder()
                        .id(1L)
                        .checks(requirementsFromPreviousDiffSet)
                        .activeDiffSet(ArcanumReviewDataDto.DiffSet.of(100L))
                        .build()
        )).when(pullRequestService).getActiveDiffSetAndMergeRequirements(eq(1L));

        var requirementsCollector = MergeRequirementsCollector.create();
        requirementsCollector.add(requirementId("NOT_CI", "CHECK_2"));
        requirementsCollector.add(requirementId("NOT_CI", "CHECK_3"));
        requirementsCollector.add(requirementId("CI", "CHECK_2"));
        requirementsCollector.add(requirementId("CI", "CHECK_3"));

        obsoleteMergeRequirementsCleaner.clean(1L, 100L, requirementsCollector);

        verify(pullRequestService).setMergeRequirementsWithCheckingActiveDiffSet(anyLong(), anyLong(), anyMap(),
                anyString());
        verify(pullRequestService).setMergeRequirementsWithCheckingActiveDiffSet(eq(1L), eq(100L),
                eq(Map.of(
                        requirementId("CI", "CHECK_1"), false
                )),
                eq("not required in the latest iteration (diffSetId 100)")
        );
        verify(pullRequestService).sendMergeRequirementStatus(eq(1L), eq(100L),
                eq(UpdateCheckStatusRequest.builder()
                        .system("CI")
                        .type("CHECK_1")
                        .status(ArcanumMergeRequirementDto.Status.SKIPPED)
                        .build())
        );
    }

    @Test
    void removeObsoleteCiMergeRequirements_whenDiffSetIsNotActive() {
        var requirementsFromPreviousDiffSet = List.of(
                requirement("NOT_CI", "CHECK_1"),
                requirement("NOT_CI", "CHECK_2"),
                requirement("CI", "CHECK_1"),
                requirement("CI", "CHECK_2")
        );
        doReturn(Optional.of(
                ArcanumReviewDataDto.builder()
                        .id(1L)
                        .checks(requirementsFromPreviousDiffSet)
                        .activeDiffSet(ArcanumReviewDataDto.DiffSet.of(100_500L))
                        .build()
        )).when(pullRequestService).getActiveDiffSetAndMergeRequirements(eq(1L));

        var requirementsCollector = MergeRequirementsCollector.create();
        requirementsCollector.add(requirementId("NOT_CI", "CHECK_2"));
        requirementsCollector.add(requirementId("NOT_CI", "CHECK_3"));
        requirementsCollector.add(requirementId("CI", "CHECK_2"));
        requirementsCollector.add(requirementId("CI", "CHECK_3"));

        obsoleteMergeRequirementsCleaner.clean(1L, 100L, requirementsCollector);

        verify(pullRequestService, times(0)).setMergeRequirementsWithCheckingActiveDiffSet(
                anyLong(), anyLong(), anyMap(), anyString());
    }

    @Test
    void removeObsoleteCiMergeRequirements_whenArcanumClientThrowsBadRequestException() {
        doReturn("CI").when(pullRequestService).getMergeRequirementSystem();
        var requirementsFromPreviousDiffSet = List.of(
                requirement("NOT_CI", "CHECK_1"),
                requirement("NOT_CI", "CHECK_2"),
                requirement("CI", "CHECK_1"),
                requirement("CI", "CHECK_2")
        );
        doReturn(Optional.of(
                ArcanumReviewDataDto.builder()
                        .id(1L)
                        .checks(requirementsFromPreviousDiffSet)
                        .activeDiffSet(ArcanumReviewDataDto.DiffSet.of(100L))
                        .build()
        )).when(pullRequestService).getActiveDiffSetAndMergeRequirements(eq(1L));
        doThrow(new HttpException("", 1, 400, null)).when(pullRequestService)
                .setMergeRequirementsWithCheckingActiveDiffSet(eq(1L), eq(100L), anyMap(), anyString());

        var requirementsCollector = MergeRequirementsCollector.create();
        requirementsCollector.add(requirementId("NOT_CI", "CHECK_2"));
        requirementsCollector.add(requirementId("NOT_CI", "CHECK_3"));
        requirementsCollector.add(requirementId("CI", "CHECK_2"));
        requirementsCollector.add(requirementId("CI", "CHECK_3"));

        obsoleteMergeRequirementsCleaner.clean(1L, 100L, requirementsCollector);

        verify(pullRequestService).setMergeRequirementsWithCheckingActiveDiffSet(eq(1L), eq(100L),
                eq(Map.of(
                        requirementId("CI", "CHECK_1"), false
                )),
                eq("not required in the latest iteration (diffSetId 100)")
        );
    }

    @Test
    void removeObsoleteCiMergeRequirements_whenSomeHasDeniedDisablingPolicy() {
        doReturn("CI").when(pullRequestService).getMergeRequirementSystem();
        var requirementsFromPreviousDiffSet = List.of(
                arcanumMergeRequirementDto("CHECK_1", DisablingPolicyDto.DENIED),
                arcanumMergeRequirementDto("CHECK_2", DisablingPolicyDto.NEED_REASON)
        );
        doReturn(Optional.of(
                ArcanumReviewDataDto.builder()
                        .id(1L)
                        .checks(requirementsFromPreviousDiffSet)
                        .activeDiffSet(ArcanumReviewDataDto.DiffSet.of(100L))
                        .build()
        )).when(pullRequestService).getActiveDiffSetAndMergeRequirements(eq(1L));

        obsoleteMergeRequirementsCleaner.clean(1L, 100L, MergeRequirementsCollector.create());

        verify(pullRequestService).setMergeRequirementsWithCheckingActiveDiffSet(anyLong(), anyLong(), anyMap(),
                anyString());

        verify(pullRequestService).setMergeRequirementsWithCheckingActiveDiffSet(eq(1L), eq(100L),
                eq(Map.of(
                        requirementId("CI", "CHECK_2"), false
                )),
                eq("not required in the latest iteration (diffSetId 100)")
        );

        verify(pullRequestService).sendMergeRequirementStatus(eq(1L), eq(100L),
                eq(UpdateCheckStatusRequest.builder()
                        .system("CI")
                        .type("CHECK_1")
                        .status(ArcanumMergeRequirementDto.Status.SKIPPED)
                        .build())
        );
        verify(pullRequestService).sendMergeRequirementStatus(eq(1L), eq(100L),
                eq(UpdateCheckStatusRequest.builder()
                        .system("CI")
                        .type("CHECK_2")
                        .status(ArcanumMergeRequirementDto.Status.SKIPPED)
                        .build())
        );
    }

    private static ArcanumMergeRequirementDto requirement(String system, String type) {
        return new ArcanumMergeRequirementDto(requirementId(system, type), true, null);
    }

    private static ArcanumMergeRequirementId requirementId(String system, String type) {
        return ArcanumMergeRequirementId.of(system, type);
    }

    private static ArcanumMergeRequirementDto arcanumMergeRequirementDto(
            String type, DisablingPolicyDto needReason
    ) {
        return new ArcanumMergeRequirementDto(
                "CI", type, true, false, ArcanumMergeRequirementDto.Status.PENDING,
                null, null, needReason, null
        );
    }

}
