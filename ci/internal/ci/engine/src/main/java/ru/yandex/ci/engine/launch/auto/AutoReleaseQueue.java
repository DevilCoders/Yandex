package ru.yandex.ci.engine.launch.auto;

import java.util.Comparator;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.base.Strings;
import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.model.ReleaseConfig;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.db.model.AutoReleaseQueueItem;
import ru.yandex.ci.core.db.model.AutoReleaseSettingsHistory;
import ru.yandex.ci.core.discovery.DiscoveredCommit;
import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.ci.core.project.AutoReleaseConfigState;
import ru.yandex.ci.core.project.ReleaseConfigState;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.launch.NotEligibleForAutoReleaseException;

import static ru.yandex.ci.core.db.model.AutoReleaseQueueItem.State.WAITING_CONDITIONS;
import static ru.yandex.ci.core.db.model.AutoReleaseQueueItem.State.WAITING_PREVIOUS_COMMITS;

@Slf4j
@AllArgsConstructor
public class AutoReleaseQueue {
    private final CiMainDb db;
    private final AutoReleaseSettingsService autoReleaseSettingsService;
    private final ConfigurationService configurationService;
    private final Set<String> autoReleaseWhiteList;

    public void addToAutoReleaseQueue(
            DiscoveredCommit discoveredCommit,
            ReleaseConfig releaseConfig,
            OrderedArcRevision releaseConfigRevision,
            DiscoveryType commitDiscoveredWith,
            Set<DiscoveryType> requiredDiscovery
    ) {
        var branch = discoveredCommit.getArcRevision().getBranch();
        if (!branch.isTrunk() && !branch.isRelease()) {
            log.info(
                    "Process {} wasn't scheduled to auto release, change {} is not in trunk or release branch",
                    discoveredCommit.getProcessId(), discoveredCommit.getArcRevision()
            );
            return;
        }

        var trunkReleaseConfig = configurationService.getLastValidConfig(
                discoveredCommit.getProcessId().getPath(), ArcBranch.trunk()
        ).getRelease(discoveredCommit.getProcessId());

        if (discoveredCommit.getArcRevision().getBranch().isRelease() && trunkReleaseConfig.isPresent()) {
            if (trunkReleaseConfig.get().getBranches().getDefaultConfigSource().isTrunk()) {
                releaseConfig = trunkReleaseConfig.get();
            }
        }

        var context = createContext(discoveredCommit, releaseConfig, releaseConfigRevision);
        if (context == null) {
            return;
        }

        var state = computeAutoReleaseState(context);
        if (!state.isEnabled(branch.getType())) {
            log.info("Auto release is disabled: {}, state {}", context, state);
            return;
        }

        var existingLaunchId = discoveredCommit.getState().getLaunchIds()
                .stream()
                .filter(it -> it.getProcessId().equals(context.getProcessId()))
                .findFirst()
                .orElse(null);

        if (existingLaunchId != null) {
            log.info(
                    "Auto release is skipped: {}, cause commit {} discovered with {} already has launch of flow {}",
                    context, discoveredCommit, commitDiscoveredWith, context.getProcessId()
            );
            return;
        }

        var autoReleaseQueueItemState = branch.isTrunk() ? WAITING_PREVIOUS_COMMITS : WAITING_CONDITIONS;
        db.currentOrTx(() -> db.autoReleaseQueue().save(
                AutoReleaseQueueItem.of(
                        context.getRevision(), context.getProcessId(), autoReleaseQueueItemState,
                        requiredDiscovery
                )
        ));

        log.info(
                "Auto release queue item added with state {}: {} discovered with {}",
                autoReleaseQueueItemState, context, commitDiscoveredWith
        );
    }

    @Nullable
    private AutoReleaseContext createContext(
            DiscoveredCommit discoveredCommit,
            ReleaseConfig releaseConfig,
            OrderedArcRevision releaseConfigRevision
    ) {
        try {
            return AutoReleaseContext.create(discoveredCommit, releaseConfig, releaseConfigRevision);
        } catch (NotEligibleForAutoReleaseException e) {
            log.info(
                    "Not eligible for auto release: {}, processId {}",
                    discoveredCommit.getArcRevision(), discoveredCommit.getProcessId()
            );
            return null;
        }
    }

    public void deleteObsoleteAutoReleases(Set<AutoReleaseQueueItem.Id> obsoleteReleases) {
        if (obsoleteReleases.isEmpty()) {
            return;
        }
        db.currentOrTx(() -> db.autoReleaseQueue().delete(obsoleteReleases));
        log.info("Deleted obsolete auto releases: {}", obsoleteReleases);
    }

    public AutoReleaseState computeAutoReleaseState(AutoReleaseContext context) {
        var settings = autoReleaseSettingsService.findLastForProcessId(context.getProcessId());
        var releaseState = Optional.of(context.getReleaseConfig())
                .map(ReleaseConfigState::of)
                .orElse(null);

        return computeAutoReleaseState(releaseState, settings, context.getProcessId());
    }

    public List<AutoReleaseQueueItem> findByState(AutoReleaseQueueItem.State state) {
        return db.currentOrReadOnly(() -> db.autoReleaseQueue().findByState(state));
    }

    public AutoReleaseState computeAutoReleaseState(
            @Nullable ReleaseConfigState releaseConfigState,
            @Nullable AutoReleaseSettingsHistory lastSettings,
            CiProcessId processId
    ) {
        var enabledInConfig = isAutoReleaseEnabledInConfig(releaseConfigState, ArcBranch.Type.TRUNK);
        var enabledInForBranchesConfig = isAutoReleaseEnabledInConfig(
                releaseConfigState, ArcBranch.Type.RELEASE_BRANCH
        );
        if (!autoReleaseWhiteList.isEmpty() && !autoReleaseWhiteList.contains(processId.asString())) {
            log.info("Auto release is disabled, cause processId {} is not in whitelist", processId);
            enabledInConfig = false;
        }
        return new AutoReleaseState(enabledInConfig, enabledInForBranchesConfig, lastSettings);
    }

    public void dropItem(AutoReleaseQueueItem item) {
        db.currentOrTx(() -> db.autoReleaseQueue().delete(item.getId()));
        log.info("item {} deleted", item.getId());
    }

    public void changeState(AutoReleaseQueueItem item, AutoReleaseQueueItem.State newState) {
        db.currentOrTx(() -> {
            db.autoReleaseQueue().save(item.withState(newState));
            log.info("{} changed state from {} to {}", item.getId(), item.getState(), newState);
        });
    }

    private static boolean isAutoReleaseEnabledInConfig(
            @Nullable ReleaseConfigState releaseState, ArcBranch.Type branchType
    ) {
        return Optional.ofNullable(releaseState)
                .map(branchType == ArcBranch.Type.TRUNK
                        ? ReleaseConfigState::getAuto
                        : ReleaseConfigState::getBranchesAuto
                )
                .map(AutoReleaseConfigState::isEnabled)
                .orElse(Boolean.FALSE);
    }

    public String getQueueSnapshotDebugString() {
        var queue = db.scan().run(() -> db.autoReleaseQueue().findAll());
        var byProcessId = queue.stream()
                .collect(Collectors.groupingBy(item -> item.getId().getProcessId(), Collectors.collectingAndThen(
                                Collectors.toList(),
                                list -> list.stream().sorted(
                                                Comparator.comparing(item -> item.getOrderedArcRevision().getNumber()))
                                        .collect(Collectors.toList())
                        ))
                );

        var processIds = byProcessId.keySet().stream().sorted().toList();

        var sb = new StringBuilder();
        for (var processId : processIds) {
            sb.append("\n--- ").append(processId);
            for (var item : byProcessId.get(processId)) {
                sb.append("\n    ").append(item.getOrderedArcRevision().getNumber())
                        .append(" :: ")
                        .append(Strings.padEnd(item.getState().toString(), 25, ' '))
                        .append(" :: ")
                        .append(item);
            }
        }
        return sb.toString();
    }
}
