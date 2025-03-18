package ru.yandex.ci.storage.core.check;

import java.time.Instant;
import java.util.List;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.pr.ArcanumCheckType;
import ru.yandex.ci.storage.core.cache.StorageCoreCache;
import ru.yandex.ci.storage.core.check.tasks.ArcanumCheckStatusReporterTask;
import ru.yandex.ci.storage.core.db.constant.CheckTypeUtils;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_merge_requirements.CheckMergeRequirementsEntity;
import ru.yandex.ci.storage.core.db.model.check_merge_requirements.MergeInterval;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Slf4j
@RequiredArgsConstructor
public class RequirementsService {

    @Nonnull
    private final BazingaTaskManager bazingaTaskManager;

    @Nonnull
    private final String environment;

    public boolean skipCheck(CheckEntity check, String actionName) {
        if (!check.getReportStatusToArcanum()) {
            log.info(
                    "Skipped {} check {} due to reportStatusToArcanum={}",
                    actionName, check.getId().getId(), check.getReportStatusToArcanum()
            );
            return true;
        }

        if (!check.isFromEnvironment(environment)) {
            log.info(
                    "Skipped {} check {} because it is from different environment: {}",
                    actionName, check.getId(), check.getEnvironment()
            );
            return true;
        }

        return false;
    }

    public boolean skipMergeRequirements(CheckEntity check, String actionName) {
        if (skipCheck(check, actionName)) {
            return true;
        }

        if (!CheckTypeUtils.isPrecommitCheck(check.getType())) {
            log.info(
                    "Skipped {} check {} due to check type {}",
                    actionName, check.getId().getId(), check.getType()
            );
            return true;
        }

        return check.isNotificationsDisabled();
    }

    public void scheduleRequirement(
            StorageCoreCache.Modifiable cache,
            CheckEntity.Id checkId,
            ArcanumCheckType checkType,
            ArcanumCheckStatus checkStatus
    ) {
        scheduleRequirement(cache, checkId, checkType, checkStatus, List.of());
    }

    public void scheduleRequirement(
            StorageCoreCache.Modifiable cache,
            CheckEntity.Id checkId,
            ArcanumCheckType checkType,
            ArcanumCheckStatus checkStatus,
            List<MergeInterval> mergeIntervals
    ) {
        var check = cache.checks().getOrThrow(checkId);
        if (skipMergeRequirements(check, "Check " + checkType)) {
            return;
        }

        var requirementId = new CheckMergeRequirementsEntity.Id(check.getId(), checkType);
        var merge = CheckMergeRequirementsEntity.builder()
                .id(requirementId)
                .value(checkStatus)
                .updatedAt(Instant.now())
                .mergeIntervals(mergeIntervals)
                .build();

        cache.mergeRequirements().writeThrough(merge);

        log.info(
                "Set merge requirement status for {} to '{}'",
                requirementId,
                checkStatus.getStatus()
        );

        var jobId = bazingaTaskManager.schedule(
                new ArcanumCheckStatusReporterTask(
                        requirementId.getCheckId().getId(), requirementId.getArcanumCheckType()
                )
        );
        log.info("Bazinga job scheduled: id {}", jobId);
    }
}
