package ru.yandex.ci.engine.discovery;

import java.util.Collection;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nonnull;

import com.google.common.base.Preconditions;
import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.ConfigStatus;
import ru.yandex.ci.core.config.a.model.FilterConfig;
import ru.yandex.ci.core.config.a.model.ReleaseConfig;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.discovery.DiscoveredCommit;
import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.ci.engine.branch.BranchService;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.engine.launch.LaunchService.LaunchMode;
import ru.yandex.ci.engine.launch.OnCommitLaunchService;
import ru.yandex.ci.engine.launch.auto.AutoReleaseService;

@Slf4j
@AllArgsConstructor
public class DiscoveryServiceProcessor {

    @Nonnull
    private final BranchService branchService;

    @Nonnull
    private final CiMainDb db;

    @Nonnull
    private final AutoReleaseService autoReleaseService;

    @Nonnull
    private final OnCommitLaunchService onCommitLaunchService;

    void processTriggeredProcesses(DiscoveryContext context, TriggeredProcesses processes) {
        new ProcessTriggers(context, processes).process();
    }

    @AllArgsConstructor
    class ProcessTriggers {
        @Nonnull
        private final DiscoveryContext context;

        @Nonnull
        private final TriggeredProcesses processes;

        void process() {
            processReleasesOnCommitTrigger();
            processFlowsWithOnCommitTrigger();
        }

        private void processReleasesOnCommitTrigger() {
            var aYamlConfig = context.getConfigBundle().getValidAYamlConfig();

            var releases = processes.getReleases();
            if (releases.isEmpty()) {
                log.info("No releases to discover");
                return;
            }

            releases.keySet().forEach(processId -> {
                var releaseConfig = aYamlConfig.getCi().getRelease(processId.getSubId());

                log.info(
                        "Processing release process id {} for config {}. CiProcessId: {}",
                        releaseConfig.getId(), context.getConfigPath(), processId
                );

                var discoveredCommit = db.currentOrTx(() -> db.discoveredCommit().updateOrCreate(
                        processId, context.getRevision(), context.getConfigChange(), context.getDiscoveryType()
                ));

                if (!context.isTrunk()) {
                    db.currentOrTx(() -> branchService.addCommit(processId, context.getRevision()));
                }

                tryStartAutoRelease(processId, releaseConfig, discoveredCommit);
            });
        }

        private void tryStartAutoRelease(
                CiProcessId processId,
                ReleaseConfig releaseConfig,
                DiscoveredCommit discoveredCommit
        ) {
            var filters = releaseConfig.getFilters();
            if (releaseConfig.getFilters().isEmpty()) {
                filters = List.of(FilterConfig.defaultFilter());
            }

            var requiredDiscovery = findRequiredDiscoveryForAutostart(getDiscoveryFields(filters));
            if (discoveryModeSuitesForAutoStart(requiredDiscovery)) {
                autoReleaseService.addToAutoReleaseQueue(
                        discoveredCommit, releaseConfig, context.getConfigBundle().getRevision(),
                        context.getDiscoveryType(), requiredDiscovery
                );
            } else {
                log.info("auto start release {} is skipped, cause discovery mode {} doesn't suite, required {}",
                        processId, context.getDiscoveryType(), requiredDiscovery);
            }
        }

        private boolean discoveryModeSuitesForAutoStart(Set<DiscoveryType> requiredDiscovery) {
            return requiredDiscovery.contains(context.getDiscoveryType());
        }

        private Set<DiscoveryType> findRequiredDiscoveryForAutostart(
                Set<FilterConfig.Discovery> discoveryFromFilters
        ) {
            Preconditions.checkArgument(!discoveryFromFilters.isEmpty());

            return discoveryFromFilters.stream()
                    .flatMap(it -> switch (it) {
                        case DIR, DEFAULT -> Stream.of(DiscoveryType.DIR);
                        case GRAPH -> Stream.of(DiscoveryType.GRAPH);
                        case ANY -> Stream.of(DiscoveryType.DIR, DiscoveryType.GRAPH);
                        case PCI_DSS -> Stream.of(DiscoveryType.PCI_DSS);
                    })
                    .collect(Collectors.toSet());
        }

        private void processFlowsWithOnCommitTrigger() {
            var triggeredActions = processes.getActions();
            if (triggeredActions.isEmpty()) {
                log.info("No actions to discover");
                return;
            }

            var configBundle = context.getConfigBundle();
            if (configBundle.getStatus() == ConfigStatus.SECURITY_PROBLEM) {
                log.info(
                        "Config security state is not valid ({}), launches will be delayed for {}",
                        configBundle.getConfigEntity().getSecurityState(), configBundle.getConfigPath()
                );
            }

            triggeredActions.forEach((processId, triggeredFilters) -> {
                var launchMode = LaunchMode.ofAction(configBundle, processId, false);

                db.currentOrTx(() -> db.discoveredCommit().updateOrCreate(
                        processId, context.getRevision(), context.getConfigChange(), context.getDiscoveryType()
                ));

                tryStartOnCommitAction(configBundle, processId, triggeredFilters, launchMode);
            });
        }

        private void tryStartOnCommitAction(
                ConfigBundle configBundle,
                CiProcessId processId,
                Collection<FilterConfig> triggeredTriggers,
                LaunchMode launchMode
        ) {
            var requiredDiscovery = findRequiredDiscoveryForAutostart(getDiscoveryFields(triggeredTriggers));
            if (discoveryModeSuitesForAutoStart(requiredDiscovery)) {
                onCommitLaunchService.autoStartFlow(processId, context.getRevision(), configBundle, launchMode);
            } else {
                log.info("auto start flow {} is skipped, cause discovery mode {} doesn't suite, required {}",
                        processId, context.getDiscoveryType(), requiredDiscovery);
            }
        }
    }

    private static Set<FilterConfig.Discovery> getDiscoveryFields(Collection<FilterConfig> filters) {
        return filters.stream()
                .map(FilterConfig::getDiscovery)
                .collect(Collectors.toUnmodifiableSet());
    }

}
