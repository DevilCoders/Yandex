package ru.yandex.ci.engine.launch.auto;

import java.time.Duration;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.VirtualCiProcessId;
import ru.yandex.ci.core.config.a.model.TriggerConfig;
import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.launch.PostponeLaunch;
import ru.yandex.ci.core.project.ActionConfigState;
import ru.yandex.ci.flow.db.CiDb;

@Slf4j
@RequiredArgsConstructor
public class ActionPostCommitHandler implements BinarySearchHandler {

    private final CiDb db;

    @Override
    public Execution beginExecution() {
        var active = db.scan().run(() -> db.configStates().findAllVisible(false));
        var actions = new HashMap<CiProcessId, ActionConfigState>();
        for (var config : active) {
            for (var action : config.getActions()) {
                if (hasValidTrigger(action) &&
                        action.getMaxActiveCount() != null && action.getBinarySearchConfig() != null) {
                    var processId = CiProcessId.ofFlow(config.getConfigPath(), action.getFlowId());
                    actions.put(processId, action);
                }
            }
        }

        log.info("Configured {} actions with binary search", actions.size());
        return new SingleExecution(actions); // Some caching is required
    }

    private boolean hasValidTrigger(ActionConfigState actionConfigState) {
        for (var trigger : actionConfigState.getTriggers()) {
            if (trigger.getOn() == TriggerConfig.On.COMMIT) {
                return true;
            }
        }
        return false;
    }

    @RequiredArgsConstructor
    private static class SingleExecution implements Execution {

        private final Map<CiProcessId, ActionConfigState> actions;

        @Nullable
        @Override
        public BinarySearchSettings getBinarySearchSettings(CiProcessId ciProcessId) {
            var action = actions.get(ciProcessId);
            if (action == null) {
                log.info("Unable to find action config with binary search: {}", ciProcessId);
                return null;
            }

            var discoveryTypes = calculateDiscoveryTypes(action);
            if (discoveryTypes.isEmpty()) {
                log.info("Unable to calculate discovery types for action {}", action);
                return null;
            }

            var maxActiveCount = action.getMaxActiveCount();
            Preconditions.checkState(maxActiveCount != null);

            var binarySearchConfig = action.getBinarySearchConfig();
            Preconditions.checkState(binarySearchConfig != null);

            return BinarySearchSettings.builder()
                    .minIntervalDuration(
                            Objects.requireNonNullElse(binarySearchConfig.getMinIntervalDuration(), Duration.ZERO))
                    .minIntervalSize(Objects.requireNonNullElse(binarySearchConfig.getMinIntervalSize(), 0))
                    .closeLaunchesOlderThan(binarySearchConfig.getCloseIntervalsOlderThan())
                    .maxActiveLaunches(maxActiveCount)
                    .discoveryTypes(discoveryTypes)
                    .treatFailedLaunchAsComplete(true) // Accept failed launches as part of binary search
                    .build();
        }

        private Set<DiscoveryType> calculateDiscoveryTypes(ActionConfigState action) {
            var result = new HashSet<DiscoveryType>();
            for (var trigger : action.getTriggers()) {
                if (trigger.getOn() != TriggerConfig.On.COMMIT) {
                    continue;
                }
                if (trigger.getFilters().isEmpty()) {
                    result.add(DiscoveryType.DIR);
                    continue;
                }
                for (var filter : trigger.getFilters()) {
                    switch (filter.getDiscovery()) {
                        case DEFAULT, DIR -> result.add(DiscoveryType.DIR);
                        case GRAPH -> result.add(DiscoveryType.GRAPH);
                        case ANY -> result.addAll(List.of(DiscoveryType.DIR, DiscoveryType.GRAPH));
                        default -> {
                            // unsupported
                        }
                    }
                }
            }
            return result;
        }


        @Override
        public List<VirtualCiProcessId.VirtualType> getVirtualTypes() {
            return List.of();
        }

        @Override
        public ComparisonResult hasNewFailedTests(Launch launchFrom, Launch launchTo) {
            var leftFailure = launchFrom.getStatus() == LaunchState.Status.FAILURE;
            var rightFailure = launchTo.getStatus() == LaunchState.Status.FAILURE;
            if (!leftFailure && rightFailure) {
                return ComparisonResult.HAS_NEW_FAILED_TESTS;
            } else {
                return ComparisonResult.NO_NEW_FAILED_TESTS;
            }
        }

        @Override
        public void onLaunchStatusChange(PostponeLaunch postponeLaunch) {
            // do nothing
        }
    }
}
