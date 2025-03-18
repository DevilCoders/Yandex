package ru.yandex.ci.engine.discovery;

import java.util.Collection;
import java.util.List;
import java.util.function.BiFunction;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nonnull;

import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import one.util.streamex.EntryStream;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.model.ActionConfig;
import ru.yandex.ci.core.config.a.model.CiConfig;
import ru.yandex.ci.core.config.a.model.TriggerConfig;

@Slf4j
@AllArgsConstructor
public class DiscoveryServicePostCommitTriggers {

    @Nonnull
    private final DiscoveryServiceFilters discoveryServiceFilters;

    public TriggeredProcesses findTriggeredProcesses(DiscoveryContext context, CiConfig ciConfig) {
        return new ContextCollector(context, ciConfig, discoveryServiceFilters.getFilter(context)).find();
    }

    @AllArgsConstructor
    static class ContextCollector {

        @Nonnull
        private final DiscoveryContext context;

        @Nonnull
        private final CiConfig ciConfig;

        @Nonnull
        private final DiscoveryServiceFilters.DiscoveryModesFilter filter;

        TriggeredProcesses find() {
            var releaseIds = findTriggeredReleaseIds();
            var flowIds = findActionsTriggeredOnCommit();

            return TriggeredProcesses.of(releaseIds)
                    .merge(TriggeredProcesses.of(flowIds));
        }

        private List<TriggeredProcesses.Triggered> findActionsTriggeredOnCommit() {
            return findImpl(
                    "on-commit triggered flow(s)",
                    ciConfig.getMergedActions().values(),
                    (actions, isReleaseBranch) -> actions
                            .stream()
                            .flatMap(action -> EntryStream.of(action.getTriggers())
                                    .flatMapKeyValue((i, trigger) -> streamActions(
                                            isReleaseBranch, action, i, trigger
                                    ))
                            )
                            .collect(Collectors.toList())
            );
        }

        @Nonnull
        private Stream<? extends TriggeredProcesses.Triggered> streamActions(
                boolean isReleaseBranch,
                ActionConfig action,
                int triggerIndex,
                TriggerConfig trigger
        ) {
            var processId = CiProcessId.ofFlow(context.getConfigPath(), action.getId());
            if (trigger.getOn() != TriggerConfig.On.COMMIT) {
                log.info("Action [{}], trigger {} (flow {} {}) skipped",
                        processId, triggerIndex, action.getFlow(), trigger.getOn());
                return Stream.empty();
            }

            var branch = context.getRevision().getBranch();
            var branchType = branch.getType();
            var acceptedBranchTypes = trigger.getTargetBranchesOrDefault();
            if (!acceptedBranchTypes.contains(branchType)) {
                log.info("Action [{}], trigger {} (flow {}) skipped: commit to branch '{}' which type is {}," +
                                " but trigger accepts only {} ",
                        action.getId(), triggerIndex, action.getFlow(), branch, branchType,
                        acceptedBranchTypes
                );
                return Stream.empty();
            }

            log.info("Checking action: {}", processId);
            var suitableFilters = filter.findSuitableFilters(
                    isReleaseBranch ? List.of() : trigger.getFilters()
            );
            if (suitableFilters.isEmpty()) {
                log.info("Action [{}], trigger {} (flow {}) skipped: no filters matched",
                        processId, triggerIndex, action.getFlow());
                return Stream.empty();
            }
            return suitableFilters.stream()
                    .map(f -> new TriggeredProcesses.Triggered(processId, f));
        }


        private List<TriggeredProcesses.Triggered> findTriggeredReleaseIds() {
            return findImpl(
                    "release(s)",
                    ciConfig.getReleases().values(),
                    (releases, isReleaseBranch) -> releases.stream()
                            .flatMap(releaseConfig -> {
                                var processId = CiProcessId.ofRelease(context.getConfigPath(), releaseConfig.getId());
                                log.info("Checking release process: {}", processId);

                                var suitableFilters = filter.findSuitableFilters(
                                        isReleaseBranch ? List.of() : releaseConfig.getFilters()
                                );
                                if (suitableFilters.isEmpty()) {
                                    log.info("Release [{}] skipped: no filters matched", processId);
                                    return Stream.empty();
                                }
                                return suitableFilters.stream().map(
                                        filterConfig -> new TriggeredProcesses.Triggered(processId, filterConfig)
                                );
                            })
                            .collect(Collectors.toList())
            );
        }

        private <T> List<TriggeredProcesses.Triggered> findImpl(
                String type, Collection<T> values,
                BiFunction<Collection<T>, Boolean, List<TriggeredProcesses.Triggered>> action
        ) {
            // No filters must be used when collecting triggers for release branches
            boolean isReleaseBranch = context.isReleaseBranch();
            if (isReleaseBranch) {
                log.info("Accept all {} for branch commits", type);
            }
            var triggered = action.apply(values, isReleaseBranch);
            logTriggeredProcessesInfo(type, values, triggered);
            return triggered;
        }

        private void logTriggeredProcessesInfo(
                String type,
                Collection<?> values,
                List<TriggeredProcesses.Triggered> triggered
        ) {
            var triggeredProcessIds = triggered.stream()
                    .map(TriggeredProcesses.Triggered::getProcessId)
                    .collect(Collectors.toSet());

            log.info("Got {} out of {} {} in config {} for {}: {}",
                    triggeredProcessIds.size(), values.size(), type, context.getConfigPath(), context.getRevision(),
                    triggeredProcessIds
            );
        }

    }
}
