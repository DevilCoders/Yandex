package ru.yandex.ci.storage.core.check.tasks;

import java.time.Duration;
import java.time.Instant;

import javax.annotation.Nonnull;

import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.arcanum.ArcanumClient;
import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementId;
import ru.yandex.ci.client.arcanum.MergeIntervalDto;
import ru.yandex.ci.client.arcanum.UpdateCheckStatusRequest;
import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.ci.core.pr.ArcanumCheckType;
import ru.yandex.ci.storage.core.cache.StorageCoreCache;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_merge_requirements.CheckMergeRequirementsEntity;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.misc.bender.annotation.BenderBindAllFields;

import static ru.yandex.ci.client.arcanum.ArcanumClient.Require.required;
import static ru.yandex.ci.client.arcanum.ArcanumMergeRequirementDto.Status.PENDING;
import static ru.yandex.ci.core.pr.ArcanumCheckType.AUTOCHECK_PESSIMIZED;
import static ru.yandex.ci.core.pr.ArcanumCheckType.CI_BUILD_NATIVE;
import static ru.yandex.ci.core.pr.ArcanumCheckType.CI_LARGE_TESTS;

@Slf4j
public class ArcanumCheckStatusReporterTask extends AbstractOnetimeTask<ArcanumCheckStatusReporterTask.Params> {

    private StorageCoreCache<?> storageCache;
    private ArcanumClient arcanumClient;

    public ArcanumCheckStatusReporterTask(
            StorageCoreCache<?> storageCache,
            ArcanumClient arcanumClient
    ) {
        super(Params.class);
        this.storageCache = storageCache;
        this.arcanumClient = arcanumClient;
    }

    public ArcanumCheckStatusReporterTask(
            long checkId,
            @Nonnull ArcanumCheckType arcanumCheckType
    ) {
        super(new Params(checkId, arcanumCheckType));
    }

    @Override
    protected void execute(Params params, ExecutionContext context) {
        log.info("Processing: {}", params);

        var check = storageCache.checks().getFreshOrThrow(CheckEntity.Id.of(params.getCheckId()));
        var requirementsId = new CheckMergeRequirementsEntity.Id(check.getId(), params.getArcanumCheckType());
        var mergeRequirement = storageCache.mergeRequirements().getFreshOrThrow(requirementsId);

        var checkType = params.getArcanumCheckType();
        var pullRequestId = check.getPullRequestId().orElseThrow();
        var diffSetId = check.getDiffSetId();
        var status = mergeRequirement.getValue();

        log.info(
                "Sending {}={} to arcanum (pr:{},diffSet:{}) for check {}",
                checkType, status, pullRequestId, diffSetId, params.getCheckId()
        );

        arcanumClient.setMergeRequirementStatus(
                pullRequestId,
                diffSetId,
                UpdateCheckStatusRequest.builder()
                        .system(checkType.getSystem())
                        .type(checkType.getType())
                        .status(status.getStatus())
                        .description(status.getMessage())
                        .systemCheckId(Long.toString(params.getCheckId()))
                        .mergeIntervalsUtc(
                                mergeRequirement.getMergeIntervals().stream()
                                        .map(r -> new MergeIntervalDto(r.getFrom(), r.getTo()))
                                        .toList()
                        )
                        .build()
        );

        log.info(
                "Sent {}={} to arcanum (pr:{},diffSet:{}) for check {}",
                checkType, status, pullRequestId, diffSetId, params.getCheckId()
        );

        var required = ((checkType == CI_LARGE_TESTS || checkType == CI_BUILD_NATIVE)
                && status.getStatus() == PENDING) || checkType == AUTOCHECK_PESSIMIZED;
        if (required) {
            arcanumClient.setMergeRequirement(
                    pullRequestId,
                    ArcanumMergeRequirementId.of(checkType.getSystem(), checkType.getType()),
                    required()
            );
        }

        storageCache.modifyWithDbTx(
                cache -> {
                    var freshMergeRequirements = cache.mergeRequirements().getFreshOrThrow(requirementsId);
                    if (freshMergeRequirements.getStatus().equals(mergeRequirement.getStatus())) {
                        cache.mergeRequirements().writeThrough(freshMergeRequirements.toBuilder()
                                .reportedAt(Instant.now())
                                .build()
                        );
                    } else {
                        log.info(
                                "Skipping state update for {} because status has changed to {}",
                                check.getId(), freshMergeRequirements.getStatus()
                        );
                    }
                }
        );
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofMinutes(1);
    }

    @Value
    @BenderBindAllFields
    public static class Params {
        long checkId;
        ArcanumCheckType arcanumCheckType;
    }
}
