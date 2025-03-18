package ru.yandex.ci.storage.reader.check;

import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.check.CreateIterationParams;
import ru.yandex.ci.storage.core.check.tasks.RestartTestsTask;
import ru.yandex.ci.storage.core.db.constant.CheckTypeUtils;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.reader.cache.ReaderCache;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Value
@Slf4j
public class TestRestartPlanner implements CheckEventsListener {
    BazingaTaskManager bazingaTaskManager;

    int minRecheckableTargets;
    int maxRecheckableTargets;
    int recheckablePercentLimit;

    String environment;

    @Override
    public void onBeforeIterationCompleted(
            ReaderCheckService checkService, ReaderCache.Modifiable cache, CheckIterationEntity.Id iterationId
    ) {
        if (iterationId.isMetaIteration() ||
                iterationId.getIterationType().equals(CheckIteration.IterationType.HEAVY) ||
                iterationId.getNumber() > 1) {
            return;
        }

        var check = cache.checks().getOrThrow(iterationId.getCheckId());
        if (check.getTestRestartsAllowed()) {
            if (!check.isFromEnvironment(environment)) {
                log.info(
                        "Not restarting tests for {}, reason: check created in different environment: {}",
                        iterationId, check.getEnvironment()
                );
            } else if (CheckTypeUtils.isPrecommitCheck(check.getType())) {
                createRestartIfNeeded(checkService, cache, cache.iterations().getFreshOrThrow(iterationId));
            } else {
                createRestartIfNeeded(checkService, cache, cache.iterations().getFreshOrThrow(iterationId));
            }
        } else {
            log.info("Not restarting tests for {}, reason: restarts not allowed", iterationId);
        }
    }

    private void createRestartIfNeeded(
            ReaderCheckService checkService, ReaderCache.Modifiable cache, CheckIterationEntity completedIteration
    ) {
        var statistics = completedIteration.getStatistics().getAllToolchain();

        var noRecheckReason = "";
        if (statistics.getMain().getConfigureOrEmpty().getFailedAdded() > 0) {
            noRecheckReason = "Has new failed configure";
        } else if (statistics.getMain().getBuildOrEmpty().getFailedAdded() > 0) {
            noRecheckReason = "Has new failed build";
        } else if (statistics.getMain().getStyleOrEmpty().getFailedAdded() > 0) {
            noRecheckReason = "Has new failed style";
        }

        var small = statistics.getMain().getSmallTestsOrEmpty();
        var medium = statistics.getMain().getMediumTestsOrEmpty();

        var smallFailedAdded = small.getFailedAdded();
        var mediumFailedAdded = medium.getFailedAdded();

        var total = small.getTotal() + medium.getTotal();
        var failedTotal = smallFailedAdded + mediumFailedAdded;

        if (failedTotal > 0 && failedTotal > minRecheckableTargets) {
            var failedPercent = failedTotal * 100.0 / total;
            if (failedTotal > maxRecheckableTargets) {
                noRecheckReason = "More then " + maxRecheckableTargets + " failed tests";
            } else if (failedPercent > recheckablePercentLimit) {
                noRecheckReason = "Failed %d%% of tests, limit %d%%".formatted(
                        Math.round(failedPercent), recheckablePercentLimit
                );
            }
        }

        if (!noRecheckReason.isEmpty()) {
            cache.iterations().writeThrough(
                    completedIteration.setAttribute(Common.StorageAttribute.SA_NO_RECHECK_REASON, noRecheckReason)
            );

            log.info("Not restarting tests for {}, reason: {}", completedIteration.getId(), noRecheckReason);
            return;
        }

        if (failedTotal > 0) {
            log.info("Planing test restart for {}", completedIteration.getId());

            var createdIteration = createIteration(checkService, cache, completedIteration);

            var bazingaJobId = bazingaTaskManager.schedule(new RestartTestsTask(createdIteration.getId()));
            log.info("Restart job scheduled: iteration id: {}, task id {}", completedIteration.getId(), bazingaJobId);
        } else {
            log.info("Not restarting tests for {}, reason: not failed tests", completedIteration.getId());
        }
    }

    private CheckIterationEntity createIteration(
            ReaderCheckService checkService, ReaderCache.Modifiable cache, CheckIterationEntity completedIteration
    ) {
        var completedIterationId = completedIteration.getId();

        var createdIteration = checkService.registerIterationInTx(
                cache,
                CheckIterationEntity.Id.of(
                        completedIterationId.getCheckId(),
                        completedIterationId.getIterationType(),
                        0
                ),
                CreateIterationParams.builder()
                        .info(completedIteration.getInfo())
                        .build()
        );

        log.info("Recheck iteration created: {}", createdIteration.getId());

        cache.iterations().writeThrough(
                createdIteration.setAttribute(
                        Common.StorageAttribute.SA_RECHECK_FOR, Integer.toString(completedIterationId.getNumber())
                )
        );

        return createdIteration;
    }
}
