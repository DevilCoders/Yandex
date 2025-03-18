package ru.yandex.ci.engine.launch.auto;

import java.util.List;

import lombok.Value;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.model.ReleaseConfig;
import ru.yandex.ci.core.config.a.model.auto.AutoReleaseConfig;
import ru.yandex.ci.core.config.a.model.auto.Conditions;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.db.model.AutoReleaseQueueItem;
import ru.yandex.ci.core.discovery.DiscoveredCommit;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.launch.NotEligibleForAutoReleaseException;
import ru.yandex.ci.flow.engine.definition.stage.StageGroupHelper;
import ru.yandex.lang.NonNullApi;

@Value
@NonNullApi
class AutoReleaseContext {

    CiProcessId processId;
    OrderedArcRevision revision;
    String stageGroupId;
    DiscoveredCommit discoveredCommit;

    ReleaseConfig releaseConfig;
    OrderedArcRevision releaseConfigRevision;

    private AutoReleaseContext(
            DiscoveredCommit discoveredCommit,
            ReleaseConfig releaseConfig,
            OrderedArcRevision releaseConfigRevision
    ) throws NotEligibleForAutoReleaseException {
        this.processId = discoveredCommit.getProcessId();
        this.stageGroupId = createStageGroupId(discoveredCommit, releaseConfig);
        this.revision = discoveredCommit.getArcRevision();
        this.discoveredCommit = discoveredCommit;
        this.releaseConfig = releaseConfig;
        this.releaseConfigRevision = releaseConfigRevision;
    }


    static AutoReleaseContext create(
            CiMainDb db,
            ConfigurationService configurationService,
            AutoReleaseQueueItem item
    ) throws NotEligibleForAutoReleaseException {
        var processId = CiProcessId.ofString(item.getId().getProcessId());

        var foundDiscoveredCommit = db.currentOrReadOnly(() ->
                db.discoveredCommit().findCommit(processId, item.getOrderedArcRevision())
        ).orElseThrow(() ->
                new NotEligibleForAutoReleaseException(
                        "No discovered commit found: %s, processId %s".formatted(
                                item.getOrderedArcRevision(), processId
                        )
                )
        );

        var configBundle = getConfigOrThrow(configurationService, foundDiscoveredCommit);
        return new AutoReleaseContext(
                foundDiscoveredCommit,
                configBundle.getValidReleaseConfigOrThrow(processId),
                configBundle.getRevision()
        );
    }

    /**
     * @param releaseConfigRevision can be <= then <b>discoveredCommit.getArcRevision()</b>, cause config
     *                              at <b>discoveredCommit.getArcRevision()</b>
     *                              may not exist or may be not valid
     */
    static AutoReleaseContext create(
            DiscoveredCommit discoveredCommit,
            ReleaseConfig releaseConfig,
            OrderedArcRevision releaseConfigRevision
    ) throws NotEligibleForAutoReleaseException {
        return new AutoReleaseContext(discoveredCommit, releaseConfig, releaseConfigRevision);
    }


    private static ConfigBundle getConfigOrThrow(
            ConfigurationService configurationService, DiscoveredCommit discoveredCommit
    ) throws NotEligibleForAutoReleaseException {
        CiProcessId processId = discoveredCommit.getProcessId();
        OrderedArcRevision revision = discoveredCommit.getArcRevision();

        var configOpt = configurationService.getOrCreateConfig(processId.getPath(), revision);
        if (configOpt.isEmpty()) {
            throw new NotEligibleForAutoReleaseException(
                    String.format("no configuration found in discovered commit: %s, processId %s",
                            revision, processId)
            );
        }
        ConfigBundle config = configOpt.get();
        if (!config.getStatus().isValidCiConfig()) {
            throw new NotEligibleForAutoReleaseException(String.format(
                    "no valid config found for discovered commit: %s, processId %s, config state %s",
                    revision, processId, config.getStatus()
            ));
        }
        return config;
    }

    private static String createStageGroupId(
            DiscoveredCommit discoveredCommit,
            ReleaseConfig releaseConfig
    ) throws NotEligibleForAutoReleaseException {
        var processId = discoveredCommit.getProcessId();
        String stageGroupId;
        if (releaseConfig.isIndependentStages()) {
            var branch = getBranch(discoveredCommit);
            stageGroupId = StageGroupHelper.createStageGroupId(processId, branch);
        } else {
            stageGroupId = StageGroupHelper.createStageGroupId(processId);
        }

        if (stageGroupId == null) {
            throw new NotEligibleForAutoReleaseException(String.format(
                    "stageGroupId is null: %s, processId %s",
                    discoveredCommit.getArcRevision(), discoveredCommit.getProcessId()
            ));
        }
        return stageGroupId;
    }

    private static ArcBranch getBranch(DiscoveredCommit discoveredCommit) {
        return discoveredCommit.getArcRevision().getBranch();
    }

    private ArcBranch getBranch() {
        return getBranch(discoveredCommit);
    }

    public List<Conditions> getConditions() {
        return getAutoReleaseConfig().getConditions();
    }

    private AutoReleaseConfig getAutoReleaseConfig() {
        var branchType = getBranch().getType();
        return switch (branchType) {
            case TRUNK -> releaseConfig.getAuto();
            case RELEASE_BRANCH -> releaseConfig.getBranches().getAuto();
            case PR, GROUP_BRANCH, USER_BRANCH, UNKNOWN -> throw new RuntimeException("Unsupported branch type "
                    + branchType);
        };
    }

    String asString() {
        return String.format("{%s, processId %s, stageGroupId %s, releaseConfigRevision %s}",
                revision, processId, stageGroupId, releaseConfigRevision);
    }

    @Override
    public String toString() {
        return asString();
    }

}
