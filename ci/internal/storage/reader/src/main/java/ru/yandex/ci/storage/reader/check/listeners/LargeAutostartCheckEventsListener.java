package ru.yandex.ci.storage.reader.check.listeners;

import java.util.Map;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.Common.CheckTaskType;
import ru.yandex.ci.storage.core.check.RequirementsService;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckTaskStatus;
import ru.yandex.ci.storage.core.large.LargeAutostartBootstrapTask;
import ru.yandex.ci.storage.core.large.MarkDiscoveredCommitTask;
import ru.yandex.ci.storage.reader.cache.ReaderCache;
import ru.yandex.ci.storage.reader.check.CheckEventsListener;
import ru.yandex.ci.storage.reader.check.ReaderCheckService;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Slf4j
@RequiredArgsConstructor
public class LargeAutostartCheckEventsListener implements CheckEventsListener {

    @Nonnull
    private final BazingaTaskManager bazingaTaskManager;

    @Nonnull
    private final RequirementsService requirementsService;

    @Override
    public void onChunkTypeFinalized(
            ReaderCheckService checkService,
            ReaderCache.Modifiable cache,
            CheckIterationEntity.Id iterationId,
            Common.ChunkType chunkType
    ) {
        if (!iterationId.isFirstFullIteration()) {
            return;
        }

        this.scheduleLargeAutostart(cache, iterationId, chunkType);
    }

    private void scheduleLargeAutostart(
            ReaderCache.Modifiable cache,
            CheckIterationEntity.Id iterationId,
            Common.ChunkType chunkType
    ) {
        var check = cache.checks().getOrThrow(iterationId.getCheckId());

        if (requirementsService.skipCheck(check, "Large-tests")) {
            return;
        }

        // TODO: split large tests and native builds by different requirements after ARCANUM-4924

        var iteration = cache.iterations().getFreshOrThrow(iterationId);

        var largeTestStatus = iteration.getCheckTaskStatus(CheckTaskType.CTT_LARGE_TEST);
        var nativeBuildsStatus = iteration.getCheckTaskStatus(CheckTaskType.CTT_NATIVE_BUILD);

        if (largeTestStatus == CheckTaskStatus.NOT_REQUIRED &&
                nativeBuildsStatus == CheckTaskStatus.NOT_REQUIRED) {
            log.info(
                    "Iteration {}, no registered Large tests/Native builds for autostart",
                    iterationId
            );
            markDiscoveredCommit(check, chunkType);
            return; // --- No updates required
        }

        if (!largeTestStatus.isDiscovering() &&
                !nativeBuildsStatus.isDiscovering()) {
            log.info(
                    "Iteration {}, not expecting Large tests/Native builds for autostart, " +
                            "large tests = {}, native builds = {}",
                    iterationId, largeTestStatus, nativeBuildsStatus
            );
            markDiscoveredCommit(check, chunkType);
            return; // --- No updates required
        }

        var largeTestsStatistics = iteration.getTestTypeStatistics().get(Common.ChunkType.CT_LARGE_TEST);
        var nativeBuildsStatistics = iteration.getTestTypeStatistics().get(Common.ChunkType.CT_BUILD);

        if (largeTestStatus.isDiscovering() && !largeTestsStatistics.isCompleted()) {
            log.info("Iteration {}, Large tests, {} is not complete, pending...",
                    iterationId, Common.ChunkType.CT_LARGE_TEST);
            return; // --- Not ready
        }
        if (nativeBuildsStatus.isDiscovering() && !nativeBuildsStatistics.isCompleted()) {
            log.info("Iteration {}, Native Builds, {} is not complete, pending...",
                    iterationId, Common.ChunkType.CT_BUILD);
            return; // --- Not ready
        }

        log.info("Iteration {}, all required chunks are ready", iterationId);

        // Optimization - do not start launch tests discovery if runLargeTestsAfterDiscovery is not set
        if (largeTestStatus == CheckTaskStatus.MAYBE_DISCOVERING) {
            if (check.getAutostartLargeTests().isEmpty() && !check.getRunLargeTestsAfterDiscovery()) {
                largeTestStatus = CheckTaskStatus.NOT_REQUIRED;
                var targetState = Map.of(CheckTaskType.CTT_LARGE_TEST, largeTestStatus);
                log.info("Iteration {}, reset target state {}", iterationId, targetState);
                cache.iterations().writeThrough(
                        iteration.updateCheckTaskStatuses(targetState)
                );
                if (nativeBuildsStatus == CheckTaskStatus.NOT_REQUIRED) {
                    log.info(
                            "Iteration {}, no registered Large tests/Native builds for autostart",
                            iterationId
                    );
                    markDiscoveredCommit(check, chunkType);
                    return; // --- No updates required
                }
            }
        }

        log.info("Scheduling LargeAutostartBootstrapTask: {}", iterationId);
        bazingaTaskManager.schedule(new LargeAutostartBootstrapTask(iterationId));

        var targetState = Map.of(
                CheckTaskType.CTT_LARGE_TEST, scheduleIfPossible(largeTestStatus),
                CheckTaskType.CTT_NATIVE_BUILD, scheduleIfPossible(nativeBuildsStatus)
        );
        log.info("Iteration {}, target state is {}", iterationId, targetState);
        cache.iterations().writeThrough(
                iteration.updateCheckTaskStatuses(targetState)
        );
    }

    private void markDiscoveredCommit(CheckEntity check, Common.ChunkType chunkType) {
        if (check.getType() == CheckOuterClass.CheckType.TRUNK_POST_COMMIT &&
                chunkType == Common.ChunkType.CT_LARGE_TEST) {
            log.info("Schedule marking commit as discovered: {}", check.getRight().getRevision());
            bazingaTaskManager.schedule(new MarkDiscoveredCommitTask(check.getId()));
        }
    }

    private CheckTaskStatus scheduleIfPossible(CheckTaskStatus status) {
        return status.isDiscovering() ? CheckTaskStatus.SCHEDULED : status;
    }

}
