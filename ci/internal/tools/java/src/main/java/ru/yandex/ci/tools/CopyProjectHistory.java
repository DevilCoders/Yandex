package ru.yandex.ci.tools;

import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Objects;
import java.util.Optional;
import java.util.Set;
import java.util.function.Predicate;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.lang3.tuple.Pair;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;

import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.yql.YqlPredicate;
import yandex.cloud.repository.kikimr.yql.YqlView;
import ru.yandex.ci.common.ydb.KikimrTableCi;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.db.model.AutoReleaseSettingsHistory;
import ru.yandex.ci.core.db.model.ConfigDiscoveryDir;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.core.discovery.DiscoveredCommit;
import ru.yandex.ci.core.discovery.DiscoveredCommitState;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.project.ActionConfigState;
import ru.yandex.ci.core.project.ReleaseConfigState;
import ru.yandex.ci.engine.launch.LaunchService;
import ru.yandex.ci.engine.launch.version.VersionUtils;
import ru.yandex.ci.engine.timeline.TimelineService;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.definition.stage.StageGroupHelper;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.di.ResourceEntity;
import ru.yandex.ci.flow.engine.runtime.state.StageGroupState;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.spring.UrlServiceConfig;
import ru.yandex.ci.flow.spring.ydb.YdbCiConfig;
import ru.yandex.ci.flow.utils.UrlService;
import ru.yandex.ci.ydb.service.CounterEntity;

/**
 * DRY_RUN=true by default
 */
@Slf4j
@Configuration
@Import({YdbCiConfig.class, UrlServiceConfig.class})
public class CopyProjectHistory extends AbstractSpringBasedApp {

    private static final boolean DRY_RUN = false;

    @Autowired
    CiDb db;

    @Autowired
    UrlService urlService;

    @Override
    protected void run() throws Exception {
        migrate_DEVTOOLSSUPPORT_18759();
    }

    void migrate_DEVTOOLSSUPPORT_18759() {
        var from = "kinopoisk/frontend/ott/a.yaml";
        var to = "kinopoisk/frontend/kp/a.yaml";

        var acceptProcessIds = Set.of(
                CiProcessId.ofString(from + ":r:hd-release")
//                CiProcessId.ofString(from + ":r:testing-hd")
//                CiProcessId.ofString(from + ":f:hd-pr-action")
        );
//        var mapProcessIds = Map.of(
//                CiProcessId.ofString(to + ":r:hd-release"),
//                CiProcessId.ofString(to + ":r:release-hd-www"),
//                CiProcessId.ofString(to + ":r:testing-hd"),
//                CiProcessId.ofString(to + ":r:testing-hd-www"),
//                CiProcessId.ofString(to + ":f:hd-pr-action"),
//                CiProcessId.ofString(to + ":f:pullrequest-hd-www")
//        );

        var movedFrom = List.of(Path.of(from));
        var movedTo = Path.of(to);
        Predicate<CiProcessId> filter = acceptProcessIds::contains;

        migrateImpl(movedFrom, movedTo, filter);
    }

    void migrateTest() {
        var from = "junk/pochemuto/ci-processes/sawmill-release-action/a.yaml";
        var to = "junk/miroslav2/ci/run-cli/a.yaml";

        var acceptProcessIds = Set.of(
                CiProcessId.ofString(from + ":r:sawmill-release-with-hf-and-rb")
        );
//        var mapProcessIds = Map.of(
//                CiProcessId.ofString(to + ":r:sawmill-release-with-hf-and-rb"),
//                CiProcessId.ofString(to + ":r:sawmill-release-with-hf-and-rb-and-copy")
//        );

        var movedFrom = List.of(Path.of(from));
        var movedTo = Path.of(to);
        Predicate<CiProcessId> filter = acceptProcessIds::contains;

        migrateImpl(movedFrom, movedTo, filter);
    }

    private void migrateImpl(
            List<Path> sourceAYamls,
            Path targetAYaml,
            Predicate<CiProcessId> acceptProcessIds
    ) {
        var statistics = new Statistics();
        var warnings = new ArrayList<String>();
        try {
            db.currentOrTx(() -> new HistoryCopier(sourceAYamls, targetAYaml, acceptProcessIds, statistics, warnings)
                    .copyInTx());
        } finally {
            log.info("Statistics: {}", statistics);
            warnings.forEach(log::warn);
        }
    }

    @RequiredArgsConstructor
    private class HistoryCopier {
        private final List<Path> sourceAYamls;
        private final Path targetAYaml;
        private final Predicate<CiProcessId> acceptProcessIds;

        private final Statistics statistics;
        private final List<String> warnings;

        void copyInTx() {
            statistics.clear();
            warnings.clear();

            var configStateAndCiProcesses = sourceAYamls.stream()
                    .map(sourceAYaml -> {
                        var configState = db.configStates().get(sourceAYaml);

                        List<CiProcessId> processIds = new ArrayList<>();
                        configState.getReleases().forEach(release -> processIds.add(
                                CiProcessId.ofRelease(configState.getConfigPath(), release.getReleaseId())
                        ));
                        configState.getActions().forEach(flow -> processIds.add(
                                CiProcessId.ofFlow(configState.getConfigPath(), flow.getFlowId())
                        ));
                        log.info("copyFromAYaml: {}, releases {}, flows {}", sourceAYaml,
                                configState.getReleases().size(), configState.getActions().size());

                        return Pair.of(configState, processIds);
                    })
                    .toList();

            log.info("sourceProcessIds: {}", configStateAndCiProcesses.stream()
                    .flatMap(it -> it.getRight().stream())
                    .collect(Collectors.toList()));

            if (checkThatSourceAYamlsNotContainEqualProcessIds(configStateAndCiProcesses, acceptProcessIds, warnings)) {
                for (var configStateAndCiProcess : configStateAndCiProcesses) {
                    var configState = configStateAndCiProcess.getLeft();
                    var processIds = configStateAndCiProcess.getRight();
                    copyInTx(configState, processIds);
                }
            }

            if (!warnings.isEmpty()) {
                throw new RuntimeException("Warning list is not empty. See warnings below");
            }
        }

        private void copyInTx(@Nullable ConfigState configState, List<CiProcessId> processIds) {
            /* ConfigState with source a.yaml will be deleted after moving a.yaml (via pull request).
            You can copy processIds from logs if there is a need and run this method with configState=null */
            for (var sourceProcessId : processIds) {
                if (!acceptProcessIds.test(sourceProcessId)) {
                    log.info("Process {} ignored", sourceProcessId);
                    continue;
                }

                var copiedProcessId = sourceProcessId.getType().isRelease()
                        ? CiProcessId.ofRelease(targetAYaml, sourceProcessId.getSubId())
                        : CiProcessId.ofFlow(targetAYaml, sourceProcessId.getSubId());
                log.info("sourceProcessId: {}, copiedProcessId {}", sourceProcessId, copiedProcessId);

                var launches = db.launches().getLaunches(sourceProcessId, -1, -1, false).stream()
                        .sorted(Comparator.comparing(it -> it.getId().getLaunchNumber()))
                        .toList();

                log.info("launches: {}", launches.size());
                statistics.increment("launches", launches.size());

                for (var sourceLaunch : launches) {
                    if (sourceLaunch.getFlowLaunchId() == null) {
                        warnings.add("sourceLaunch %s is not started".formatted(sourceLaunch.getId()));
                        continue;
                    }
                    var sourceFlowLaunch = db.flowLaunch().get(FlowLaunchId.of(sourceLaunch.getFlowLaunchId()));

                    if (!sourceLaunch.getStatus().isTerminal()) {
                        if (!sourceFlowLaunch.isDisabled()) {
                            var title = getTitle(configState, sourceProcessId);
                            warnings.add(("sourceLaunch %s has not terminal status %s; title: %s, launched at: %s, " +
                                    "triggered by: %s, url: %s").formatted(
                                    sourceLaunch.getId(), sourceLaunch.getStatus(), title, sourceLaunch.getCreated(),
                                    sourceLaunch.getTriggeredBy(), urlService.toLaunch(sourceLaunch)
                            ));
                        }
                    }

                    var copiedLaunchId = LaunchId.of(copiedProcessId, sourceLaunch.getId().getLaunchNumber());
                    var copiedFlowLaunch = sourceFlowLaunch.toBuilder()
                            .id(FlowLaunchId.of(copiedLaunchId))
                            .build();
                    save(db.flowLaunch(), sourceFlowLaunch, copiedFlowLaunch);

                    var copiedLaunch = sourceLaunch.toBuilder()
                            .id(copiedLaunchId.toKey())
                            .flowLaunchId(copiedFlowLaunch.getIdString())
                            .build();
                    save(db.launches(), sourceLaunch, copiedLaunch);

                    copyJobInstance(sourceFlowLaunch, copiedFlowLaunch);
                    copyStageGroup(sourceProcessId, copiedProcessId, sourceFlowLaunch.getTargetRevision().getBranch());

                    // we don't update resources and producedResources/consumedResources inside flowLaunch
                    var sourceResources = db.resources().find(List.of(
                            YqlPredicate.where("flowLaunchId").eq(sourceFlowLaunch.getIdString()),
                            YqlView.index(ResourceEntity.IDX_FLOW_LAUNCH_ID)
                    ));
                    statistics.increment("resources", sourceResources.size());
                }

                copyLastAutoReleaseSettingsHistory(sourceProcessId, copiedProcessId);
                copyConfigDiscoveryDirs(sourceProcessId, copiedProcessId);
                copyLaunchCounter(sourceProcessId, copiedProcessId);
                copyDiscoveredCommits(sourceProcessId, copiedProcessId);
                copyTimelineItems(sourceProcessId, copiedProcessId);
                copyTimelineBranchItems(sourceProcessId, copiedProcessId);
                copyVersions(sourceProcessId, copiedProcessId);
            }

        }

        private void copyVersions(CiProcessId sourceProcessId, CiProcessId copiedProcessId) {
            var sourceVersions = db.versions().find(List.of(
                    YqlPredicate.where("id.processId").eq(sourceProcessId.asString())
            ));
            statistics.increment("versions", sourceVersions.size());
            for (var sourceVersion : sourceVersions) {
                var copiedVersion = sourceVersion.withId(
                        sourceVersion.getId().withProcessId(copiedProcessId.asString())
                );
                save(db.versions(), sourceVersion, copiedVersion);
            }
        }

        private void copyTimelineBranchItems(CiProcessId sourceProcessId, CiProcessId copiedProcessId) {
            var sourceTimelineBranchItems = db.timelineBranchItems().find(List.of(
                    YqlPredicate.where("id.processId").eq(sourceProcessId.asString())
            ));
            statistics.increment("timelineBranchItems", sourceTimelineBranchItems.size());
            for (var sourceTimelineBranchItem : sourceTimelineBranchItems) {
                var copiedTimelineBranchItem = sourceTimelineBranchItem.withId(
                        sourceTimelineBranchItem.getId().withProcessId(copiedProcessId.asString())
                );
                save(db.timelineBranchItems(), sourceTimelineBranchItem, copiedTimelineBranchItem);

                var sourceBranchAndCounterKeyPairs = List.of(
                        Pair.of(
                                sourceTimelineBranchItem.getArcBranch(),
                                VersionUtils.counterKey(sourceProcessId, sourceTimelineBranchItem.getArcBranch())
                        ),
                        Pair.of(ArcBranch.trunk(), VersionUtils.counterKey(sourceProcessId, ArcBranch.trunk()))
                );
                for (var sourceBranchAndCounterKey : sourceBranchAndCounterKeyPairs) {
                    var sourceCounter = db.counter().find(CounterEntity.Id.of(
                            VersionUtils.VERSION_NAMESPACE,
                            sourceBranchAndCounterKey.getValue()
                    ));
                    if (sourceCounter.isPresent()) {
                        var copiedCounter = CounterEntity.of(
                                CounterEntity.Id.of(
                                        VersionUtils.VERSION_NAMESPACE,
                                        VersionUtils.counterKey(copiedProcessId, sourceBranchAndCounterKey.getKey())
                                ),
                                sourceCounter.get().getValue()
                        );
                        save(db.counter(), sourceCounter.get(), copiedCounter);
                    }
                    statistics.increment("timelineBranchItemsCounter", 1);
                }
            }
        }

        private void copyTimelineItems(CiProcessId sourceProcessId, CiProcessId copiedProcessId) {
            var sourceTimelineItems = db.timeline().find(List.of(
                    YqlPredicate.where("id.processId").eq(sourceProcessId.asString())
            ));
            statistics.increment("timelineItems", sourceTimelineItems.size());

            for (var sourceTimelineItem : sourceTimelineItems) {
                var copiedTimelineItemBuilder = sourceTimelineItem.toBuilder()
                        .id(sourceTimelineItem.getId().withProcessId(copiedProcessId.asString()));
                if (sourceTimelineItem.getBranch() != null) {
                    copiedTimelineItemBuilder.branchId(
                            copiedProcessId, sourceTimelineItem.getBranch().getBranch()
                    );
                } else {
                    Objects.requireNonNull(sourceTimelineItem.getLaunch(),
                            "%s has null branch and launch fields".formatted(sourceTimelineItem.getId()));
                    copiedTimelineItemBuilder.launch(
                            sourceTimelineItem.getLaunch().withProcessId(copiedProcessId.asString())
                    );
                }
                var copiedTimelineItem = copiedTimelineItemBuilder.build();
                save(db.timeline(), sourceTimelineItem, copiedTimelineItem);

                var sourceCounter = db.counter().find(CounterEntity.Id.of(
                        TimelineService.TIMELINE_VERSION_NAMESPACE,
                        sourceProcessId.asString()
                ));
                if (sourceCounter.isPresent()) {
                    var copiedCounter = CounterEntity.of(
                            CounterEntity.Id.of(
                                    TimelineService.TIMELINE_VERSION_NAMESPACE,
                                    copiedProcessId.asString()
                            ),
                            sourceCounter.get().getValue()
                    );
                    save(db.counter(), sourceCounter.get(), copiedCounter);
                    statistics.increment("timelineItemsCounter", 1);
                }
            }
        }

        private void copyDiscoveredCommits(CiProcessId sourceProcessId, CiProcessId copiedProcessId) {
            var sourceDiscoveredCommits = db.discoveredCommit().find(List.of(
                    YqlPredicate.where("id.processId").eq(sourceProcessId.asString())
            ));
            statistics.increment("discoveredCommits", sourceDiscoveredCommits.size());
            for (var sourceDiscoveredCommit : sourceDiscoveredCommits) {
                var copiedDiscoveredCommit = DiscoveredCommit.of(
                        copiedProcessId,
                        sourceDiscoveredCommit.getArcRevision(),
                        sourceDiscoveredCommit.getStateVersion(),
                        DiscoveredCommitState.builder()
                                .clearLaunchIds()
                                .launchIds(
                                        sourceDiscoveredCommit.getState().getLaunchIds().stream()
                                                .map(it -> LaunchId.of(copiedProcessId, it.getNumber()))
                                                .toList()
                                )
                                .clearCancelledLaunchIds()
                                .cancelledLaunchIds(
                                        sourceDiscoveredCommit.getState().getCancelledLaunchIds().stream()
                                                .map(it -> LaunchId.of(copiedProcessId, it.getNumber()))
                                                .toList()
                                )
                                .clearDisplacedLaunchIds()
                                .displacedLaunchIds(
                                        sourceDiscoveredCommit.getState().getDisplacedLaunchIds().stream()
                                                .map(it -> LaunchId.of(copiedProcessId, it.getNumber()))
                                                .toList()
                                )
                                .build()
                );
                save(db.discoveredCommit(), sourceDiscoveredCommit, copiedDiscoveredCommit);
            }
        }

        private void copyLaunchCounter(CiProcessId sourceProcessId, CiProcessId copiedProcessId) {
            var sourceLaunchCounter = db.counter().find(CounterEntity.Id.of(
                    LaunchService.LAUNCH_NUMBER_NAMESPACE, sourceProcessId.asString()
            ));
            statistics.increment("launchCounter", sourceLaunchCounter.map(it -> 1).orElse(0));
            if (sourceLaunchCounter.isPresent()) {
                var copiedLaunchCounter = CounterEntity.of(
                        CounterEntity.Id.of(LaunchService.LAUNCH_NUMBER_NAMESPACE, copiedProcessId.asString()),
                        sourceLaunchCounter.get().getValue()
                );
                save(db.counter(), sourceLaunchCounter.get(), copiedLaunchCounter);
            }
        }

        private void copyConfigDiscoveryDirs(CiProcessId sourceProcessId, CiProcessId copiedProcessId) {
            var sourceConfigDiscoveryDirs = db.configDiscoveryDirs().find(List.of(
                    YqlPredicate.where("id.configPath").eq(sourceProcessId.getPath().toString())
            ));
            statistics.increment("configDiscoveryDirs", sourceConfigDiscoveryDirs.size());

            for (var sourceConfigDiscoveryDir : sourceConfigDiscoveryDirs) {
                var copiedConfigDiscoveryDir = ConfigDiscoveryDir.of(
                        ConfigDiscoveryDir.Id.of(
                                copiedProcessId.getPath().toString(),
                                sourceConfigDiscoveryDir.getId().getPathPrefix()
                        ),
                        sourceConfigDiscoveryDir.getCommitId(),
                        sourceConfigDiscoveryDir.getDeleted()
                );
                save(db.configDiscoveryDirs(), sourceConfigDiscoveryDir, copiedConfigDiscoveryDir);
            }
        }

        private void copyLastAutoReleaseSettingsHistory(CiProcessId sourceProcessId, CiProcessId copiedProcessId) {
            var sourceAutoReleaseSettingsHistory = db.autoReleaseSettingsHistory().findLatest(sourceProcessId);
            statistics.increment("autoReleaseSettingsHistory", sourceAutoReleaseSettingsHistory != null ? 1 : 0);
            if (sourceAutoReleaseSettingsHistory != null) {
                var copiedAutoReleaseSettingsHistory = AutoReleaseSettingsHistory.of(
                        copiedProcessId,
                        sourceAutoReleaseSettingsHistory.isEnabled(),
                        sourceAutoReleaseSettingsHistory.getLogin(),
                        sourceAutoReleaseSettingsHistory.getMessage(),
                        sourceAutoReleaseSettingsHistory.getId().getTimestamp()
                );
                save(
                        db.autoReleaseSettingsHistory(),
                        sourceAutoReleaseSettingsHistory,
                        copiedAutoReleaseSettingsHistory
                );
            }
        }

        private void copyStageGroup(CiProcessId sourceProcessId, CiProcessId copiedProcessId, ArcBranch branch) {
            var sourceStageGroup = findStageGroup(StageGroupHelper.createStageGroupId(sourceProcessId, branch));
            statistics.increment("stageGroup", sourceStageGroup.map(it -> 1).orElse(0));

            if (sourceStageGroup.isPresent()) {
                var copiedStageGroup = new StageGroupState(
                        StageGroupState.Id.of(
                                StageGroupHelper.createStageGroupId(copiedProcessId)
                        ),
                        sourceStageGroup.get().getVersion(),
                        sourceStageGroup.get().getQueue()
                );
                Objects.requireNonNull(copiedStageGroup);
                save(db.stageGroup(), sourceStageGroup.get(), copiedStageGroup);
            }
        }

        private Optional<StageGroupState> findStageGroup(@Nullable String stageGroupId) {
            return Optional.ofNullable(stageGroupId)
                    .flatMap(db.stageGroup()::findOptional);
        }

        private void copyJobInstance(FlowLaunchEntity sourceFlowLaunch, FlowLaunchEntity copiedFlowLaunch) {
            var sourceJobInstances = db.jobInstance().find(List.of(
                    YqlPredicate.where("id.flowLaunchId").eq(sourceFlowLaunch.getIdString())
            ));
            statistics.increment("jobInstances", sourceJobInstances.size());

            for (var sourceJobInstance : sourceJobInstances) {
                var copiedJobInstance = sourceJobInstance.withId(
                        sourceJobInstance.getId().withFlowLaunchId(copiedFlowLaunch.getIdString())
                );
                save(db.jobInstance(), sourceJobInstance, copiedJobInstance);
            }
        }

        @Nullable
        private String getTitle(@Nullable ConfigState configState, CiProcessId processId) {
            if (configState == null) {
                return null;
            }
            if (processId.getType().isRelease()) {
                return configState.findRelease(processId.getSubId())
                        .map(ReleaseConfigState::getTitle)
                        .orElse(null);
            }
            return configState.getActions().stream()
                    .filter(it -> it.getFlowId().equals(processId.getSubId()))
                    .findFirst()
                    .map(ActionConfigState::getTitle)
                    .orElse(null);
        }

        private <T extends Entity<T>> T save(KikimrTableCi<T> table, T source, T copy) {
            if (DRY_RUN) {
                log.info("Dry run: {}, copy: {}, source: {}", copy.getClass(), copy.getId(), source.getId());
                return copy;
            }
            log.info("Save: {}, copy: {}, source: {}", copy.getClass(), copy.getId(), source.getId());
            return table.save(copy);
        }

        private static boolean checkThatSourceAYamlsNotContainEqualProcessIds(
                List<Pair<ConfigState, List<CiProcessId>>> configStateAndCiProcesses,
                Predicate<CiProcessId> acceptProcessIds,
                List<String> allWarnings
        ) {
            var warnings = new ArrayList<String>();
            var knownReleaseIdsAndAYaml = new HashMap<String, Path>();
            var knownFlowIdsAndAYaml = new HashMap<String, Path>();
            for (var configStateAndCiProcess : configStateAndCiProcesses) {
                var configState = configStateAndCiProcess.getLeft();
                var processIds = configStateAndCiProcess.getRight();

                for (var processId : processIds) {
                    if (!acceptProcessIds.test(processId)) {
                        log.info("Process {} ignored", processId);
                        continue;
                    }

                    if (processId.getType().isRelease()) {
                        var oldYamlPath = knownReleaseIdsAndAYaml
                                .putIfAbsent(processId.getSubId(), configState.getConfigPath());
                        if (oldYamlPath != null) {
                            warnings.add("%s and %s has releases with equal ids: %s".formatted(
                                    configState.getConfigPath(), oldYamlPath, processId.getSubId()
                            ));
                        }
                    } else {
                        var oldYamlPath = knownFlowIdsAndAYaml
                                .putIfAbsent(processId.getSubId(), configState.getConfigPath());
                        if (oldYamlPath != null) {
                            warnings.add("%s and %s has flows with equal ids: %s".formatted(
                                    configState.getConfigPath(), oldYamlPath, processId.getSubId()
                            ));
                        }
                    }
                }
            }

            if (!warnings.isEmpty()) {
                allWarnings.addAll(warnings);
                return false;
            } else {
                return true;
            }
        }
    }

    public static void main(String[] args) {
        startAndStopThisClass(args, Environment.STABLE);
    }

    private static class Statistics extends LinkedHashMap<String, Integer> {
        void increment(String key, int value) {
            put(key, getOrDefault(key, 0) + value);
        }
    }
}
