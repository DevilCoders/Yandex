package ru.yandex.ci.engine.launch.auto;

import java.util.List;
import java.util.Set;

import javax.annotation.Nullable;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.engine.launch.LaunchService;
import ru.yandex.ci.flow.db.CiDb;

@Slf4j
@RequiredArgsConstructor
public class PostponeActionService {

    private final CiDb db;
    private final LaunchService launchService;

    public void executePostponeActions(int taskDelaySeconds) {
        log.info("Lookup and execute postpone actions");

        var active = db.scan().run(() -> db.configStates().findAllVisible(true));

        int processed = 0;
        int checked = 0;
        for (var state : active) {
            for (var action : state.getActions()) {
                checked++;
                if (action.getMaxActiveCount() != null && action.getBinarySearchConfig() == null) {
                    processed++;
                    var ciProcessId = CiProcessId.ofFlow(state.getConfigPath(), action.getFlowId());

                    processPostponeActions(
                            ciProcessId,
                            action.getMaxActiveCount(),
                            getMaxStart(action.getMaxStartPerMinute(), taskDelaySeconds)
                    );
                }
            }
        }

        log.info("Processed {} postponed actions out of {}, total {} states", processed, checked, active.size());
    }

    private int getMaxStart(@Nullable Integer maxStartPerMinute, int taskDelaySeconds) {
        if (maxStartPerMinute == null) {
            return 0;
        }
        if (maxStartPerMinute <= 0) {
            return 0;
        }
        return (int) (maxStartPerMinute * (taskDelaySeconds / 60.0));
    }

    private void processPostponeActions(CiProcessId ciProcessId, int maxActiveCount, int maxStartPerMinute) {
        log.info("Processing postpone launches for {}", ciProcessId);
        log.info("Max active count is {}, max start per minute is {}", maxActiveCount, maxStartPerMinute);

        var launches = collectLaunchesToSchedule(ciProcessId, maxActiveCount, maxStartPerMinute);
        log.info("Total launches to schedule: {}", launches.size());

        if (!launches.isEmpty()) {
            scheduleLaunches(launches);
        }
    }

    private List<Launch.Id> collectLaunchesToSchedule(
            CiProcessId ciProcessId,
            int maxActiveCount,
            int maxStartPerMinute
    ) {
        return db.currentOrReadOnly(() -> {

            // Don't expect any launches except for trunk, but check this anyway
            var trunk = ArcBranch.trunk();

            if (maxActiveCount == 0) {
                log.info("Max active launches is {}, check all postpone actions", maxActiveCount);
                return db.launches().getLaunchIds(ciProcessId, trunk, LaunchState.Status.POSTPONE, 0);
            }

            // Treat failure as terminal status
            var terminalStates = Set.of(
                    LaunchState.Status.CANCELED,
                    LaunchState.Status.SUCCESS,
                    LaunchState.Status.FAILURE,
                    LaunchState.Status.POSTPONE
            );

            var currentActiveCount = db.launches().getLaunchesCountExcept(ciProcessId, trunk, terminalStates);
            if (currentActiveCount >= maxActiveCount) {
                log.info("Current active launches is {}, max allowed is {}", currentActiveCount, maxActiveCount);
                return List.of();
            }
            long maxCount = maxActiveCount - currentActiveCount;
            log.info("Current active launches is {}, max allowed is {}, max to schedule {}",
                    currentActiveCount, maxActiveCount, maxCount);

            if (maxStartPerMinute > 0) {
                maxCount = Math.min(maxCount, maxStartPerMinute);
                log.info("Limit maximum number of launches to schedule to {}", maxCount);
            }

            return db.launches().getLaunchIds(ciProcessId, ArcBranch.trunk(), LaunchState.Status.POSTPONE, maxCount);
        });
    }

    private void scheduleLaunches(List<Launch.Id> launchIds) {
        log.info("Total tasks to launch: {}", launchIds.size());
        for (var launchId : launchIds) {
            log.info("Scheduling postponed launch: {}", launchId);
            launchService.startDelayedOrPostponedLaunch(launchId);
        }
        log.info("Launch complete");
    }

}
