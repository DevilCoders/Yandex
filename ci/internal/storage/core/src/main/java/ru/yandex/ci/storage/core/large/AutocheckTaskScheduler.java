package ru.yandex.ci.storage.core.large;

import java.util.List;
import java.util.Map;

import javax.annotation.Nonnull;

import lombok.AllArgsConstructor;

import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementDto;
import ru.yandex.ci.core.pr.ArcanumCheckType;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.cache.StorageCoreCache;
import ru.yandex.ci.storage.core.check.ArcanumCheckStatus;
import ru.yandex.ci.storage.core.check.RequirementsService;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckTaskStatus;

@AllArgsConstructor
public class AutocheckTaskScheduler {

    @Nonnull
    private final LargeStartService largeStartService;

    @Nonnull
    private final RequirementsService requirementsService;

    @Nonnull
    private final StorageCoreCache<?> storageCache;

    public boolean trySchedule(
            CheckIterationEntity.Id iterationId,
            Common.CheckTaskType checkTaskType,
            List<LargeStartService.TestDiffWithPath> diffs
    ) {
        return storageCache.modifyWithDbTxAndGet(cache -> tryScheduleInTx(iterationId, checkTaskType, diffs, cache));
    }

    private boolean tryScheduleInTx(
            CheckIterationEntity.Id iterationId,
            Common.CheckTaskType checkTaskType,
            List<LargeStartService.TestDiffWithPath> diffs,
            StorageCoreCache.Modifiable cache
    ) {
        var check = cache.checks().getFreshOrThrow(iterationId.getCheckId());
        var iteration = cache.iterations().getFreshOrThrow(iterationId);

        var scheduler = largeStartService.getLargeTestsScheduler(cache, check);

        if (iteration.getCheckTaskStatuses().get(checkTaskType) == CheckTaskStatus.SCHEDULED) {
            var scheduled = scheduler.schedule(checkTaskType, diffs);

            cache.iterations().writeThrough(
                    iteration.updateCheckTaskStatuses(Map.of(checkTaskType, CheckTaskStatus.COMPLETE))
            );

            var status = scheduled ? ArcanumCheckStatus.pending() : ArcanumCheckStatus.skipped("No Tests matched");

            requirementsService.scheduleRequirement(
                    cache,
                    check.getId(),
                    checkTaskType == Common.CheckTaskType.CTT_LARGE_TEST ?
                            ArcanumCheckType.CI_LARGE_TESTS : ArcanumCheckType.CI_BUILD_NATIVE,
                    status
            );

            return status.getStatus() != ArcanumMergeRequirementDto.Status.SKIPPED;
        }

        return false;
    }
}
