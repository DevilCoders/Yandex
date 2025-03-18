package ru.yandex.ci.storage.reader.check.listeners;

import java.util.Set;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementDto.Status;
import ru.yandex.ci.core.pr.ArcanumCheckType;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.check.ArcanumCheckStatus;
import ru.yandex.ci.storage.core.check.RequirementsService;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_merge_requirements.CheckMergeRequirementsEntity;
import ru.yandex.ci.storage.reader.cache.ReaderCache;
import ru.yandex.ci.storage.reader.check.CheckEventsListener;
import ru.yandex.ci.storage.reader.check.ReaderCheckService;
import ru.yandex.ci.storage.reader.check.listeners.util.CheckReportUtils;
import ru.yandex.lang.NonNullApi;

@NonNullApi
@Slf4j
@RequiredArgsConstructor
// See tests in ArcanumReportingTest
public class ArcanumCheckEventsListener implements CheckEventsListener {

    @Nonnull
    private final RequirementsService requirementsService;

    @Override
    public void onChunkTypeFinalized(
            ReaderCheckService checkService,
            ReaderCache.Modifiable cache,
            CheckIterationEntity.Id iterationId,
            Common.ChunkType chunkType
    ) {
        var iterationType = iterationId.getIterationType();
        var isFull = iterationType == CheckIteration.IterationType.FULL;
        var isHeavy = iterationType == CheckIteration.IterationType.HEAVY;
        if (!isFull && !isHeavy) {
            return;
        }

        var check = cache.checks().getFreshOrThrow(iterationId.getCheckId());

        if (requirementsService.skipMergeRequirements(check, "Merge-requirements")) {
            return;
        }

        var metaIteration = cache.iterations().getFresh(iterationId.toMetaId());
        var iteration = metaIteration.orElseGet(() -> cache.iterations().getFreshOrThrow(iterationId));

        var statusMap = CheckReportUtils.evaluateRequirementStatusMap(
                iteration.getTestTypeStatistics(),
                iteration.getStatistics().getAllToolchain().getMain()
        );

        var buildStatus = statusMap.get(ArcanumCheckType.CI_BUILD);
        var buildNativeStatus = statusMap.get(ArcanumCheckType.CI_BUILD_NATIVE);
        var testStatus = statusMap.get(ArcanumCheckType.CI_TESTS);
        var largeTestStatus = statusMap.get(ArcanumCheckType.CI_LARGE_TESTS);
        var teTestStatus = statusMap.get(ArcanumCheckType.TE_JOBS);

        log.info(
                "Chunk type finished event, iteration: {}, iteration status: {}, chunk type: {}, " +
                        "build status: {}, build native status: {}, test status: {}, " +
                        "large test status: {}, te test status: {}",
                iterationId, iteration.getTestTypeStatistics().printStatus(), chunkType,
                buildStatus, buildNativeStatus, testStatus, largeTestStatus, teTestStatus
        );

        if (isFull) {
            changeNonPendingRequirementsToStatus(cache, check.getId(), ArcanumCheckType.CI_BUILD, buildStatus);
            changeNonPendingRequirementsToStatus(cache, check.getId(), ArcanumCheckType.CI_TESTS, testStatus);
        }

        if (isHeavy) {
            changeNonPendingRequirementsToStatus(
                    cache, check.getId(), ArcanumCheckType.CI_LARGE_TESTS, largeTestStatus
            );

            changeNonPendingRequirementsToStatus(
                    cache, check.getId(), ArcanumCheckType.CI_BUILD_NATIVE, buildNativeStatus
            );

            changeNonPendingRequirementsToStatus(
                    cache, check.getId(), ArcanumCheckType.TE_JOBS, teTestStatus
            );
        }

    }

    @Override
    public void onCheckFatalError(
            ReaderCheckService checkService,
            ReaderCache.Modifiable cache,
            CheckEntity.Id checkId,
            Set<CheckIterationEntity.Id> iterationIds,
            CheckIterationEntity.Id brokenIterationId,
            Set<CheckIterationEntity.Id> runningIterationsIds
    ) {
        var check = cache.checks().getFreshOrThrow(checkId);

        if (requirementsService.skipMergeRequirements(check, "Merge-requirements")) {
            return;
        }

        var brokenIteration = cache.iterations().getFreshOrThrow(brokenIterationId);
        var fatalErrorMessage = brokenIteration.getInfo().getFatalErrorInfo().getMessage();

        changePendingRequirementsToStatus(
                cache,
                check,
                iterationIds,
                ArcanumCheckStatus.error(fatalErrorMessage)
        );
    }

    @Override
    public void onCheckCancelled(
            ReaderCheckService checkService,
            ReaderCache.Modifiable cache,
            CheckEntity.Id checkId,
            Set<CheckIterationEntity.Id> iterationIds
    ) {
        var check = cache.checks().getFreshOrThrow(checkId);

        if (requirementsService.skipMergeRequirements(check, "Merge-requirements")) {
            return;
        }

        changePendingRequirementsToStatus(
                cache,
                check,
                iterationIds,
                ArcanumCheckStatus.cancelled()
        );
    }

    private void changePendingRequirementsToStatus(
            ReaderCache.Modifiable cache,
            CheckEntity check,
            Set<CheckIterationEntity.Id> iterationIds,
            ArcanumCheckStatus status
    ) {
        if (hasIterationType(iterationIds, CheckIteration.IterationType.FULL)) {
            changePendingRequirementsToStatus(cache, check, ArcanumCheckType.CI_BUILD, status);
            changePendingRequirementsToStatus(cache, check, ArcanumCheckType.CI_TESTS, status);
        }
        if (hasIterationType(iterationIds, CheckIteration.IterationType.HEAVY)) {
            changePendingRequirementsToStatus(cache, check, ArcanumCheckType.CI_LARGE_TESTS, status);
            changePendingRequirementsToStatus(cache, check, ArcanumCheckType.CI_BUILD_NATIVE, status);
        }
    }

    private void changePendingRequirementsToStatus(
            ReaderCache.Modifiable cache,
            CheckEntity check,
            ArcanumCheckType checkType,
            ArcanumCheckStatus status
    ) {
        var requirementsId = new CheckMergeRequirementsEntity.Id(check.getId(), checkType);
        var current = cache.mergeRequirements().getFresh(requirementsId);
        if (current.isEmpty() || current.get().getStatus().equals(Status.PENDING)) {
            current.ifPresent(checkMergeRequirementsEntity ->
                    log.info("Changing pending status '{}'", checkMergeRequirementsEntity.getStatus()));
            requirementsService.scheduleRequirement(cache, check.getId(), checkType, status);
        } else {
            log.info(
                    "Not changing status for {} from {}",
                    requirementsId.getCheckId(), current.get().getStatus()
            );
        }
    }

    private void changeNonPendingRequirementsToStatus(
            ReaderCache.Modifiable cache,
            CheckEntity.Id checkId,
            ArcanumCheckType checkType,
            @Nullable ArcanumCheckStatus status) {
        if (status == null) {
            return;
        }
        if (status.getStatus() == Status.PENDING) {
            return;
        }
        var requirementsId = new CheckMergeRequirementsEntity.Id(checkId, checkType);
        var current = cache.mergeRequirements().getFresh(requirementsId);
        if (current.isEmpty() || !current.get().getStatus().equals(status.getStatus())) {
            current.ifPresent(checkMergeRequirementsEntity ->
                    log.info("Changing non pending from status '{}'", checkMergeRequirementsEntity.getStatus()));
            requirementsService.scheduleRequirement(cache, checkId, checkType, status);
        } else {
            log.info(
                    "Not changing status for {} from {}",
                    requirementsId.getCheckId(), current.get().getStatus()
            );
        }
    }

    private boolean hasIterationType(
            Set<CheckIterationEntity.Id> iterationIds,
            CheckIteration.IterationType expectType) {
        return iterationIds.stream()
                .anyMatch(id -> id.getIterationType() == expectType);
    }
}
