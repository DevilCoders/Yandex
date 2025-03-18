package ru.yandex.ci.engine.launch.auto;

import java.nio.file.Path;
import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.logging.LoggingMeterRegistry;
import one.util.streamex.EntryStream;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;
import org.mockito.junit.jupiter.MockitoSettings;
import org.mockito.quality.Strictness;

import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcCommitTable;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.ConfigCreationInfo;
import ru.yandex.ci.core.config.ConfigEntity;
import ru.yandex.ci.core.config.ConfigSecurityState;
import ru.yandex.ci.core.config.ConfigStatus;
import ru.yandex.ci.core.config.a.model.AYamlConfig;
import ru.yandex.ci.core.config.a.model.CiConfig;
import ru.yandex.ci.core.config.a.model.FilterConfig;
import ru.yandex.ci.core.config.a.model.ReleaseConfig;
import ru.yandex.ci.core.config.a.model.StageConfig;
import ru.yandex.ci.core.config.a.model.auto.AutoReleaseConfig;
import ru.yandex.ci.core.config.a.model.auto.Conditions;
import ru.yandex.ci.core.db.TestCiDbUtils;
import ru.yandex.ci.core.db.model.AutoReleaseQueueItem;
import ru.yandex.ci.core.db.model.AutoReleaseSettingsHistory;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.core.db.table.AutoReleaseQueueTable;
import ru.yandex.ci.core.db.table.ConfigStateTable;
import ru.yandex.ci.core.discovery.CommitDiscoveryProgress;
import ru.yandex.ci.core.discovery.CommitDiscoveryProgressTable;
import ru.yandex.ci.core.discovery.DiscoveredCommitTable;
import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.project.AutoReleaseConfigState;
import ru.yandex.ci.core.project.ReleaseConfigState;
import ru.yandex.ci.core.security.YavToken;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.launch.LaunchService;
import ru.yandex.ci.engine.launch.NotEligibleForAutoReleaseException;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.StageGroupState;
import ru.yandex.ci.flow.engine.runtime.state.StageGroupTable;
import ru.yandex.ci.util.UserUtils;
import ru.yandex.lang.NonNullApi;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyList;
import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.isNull;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;
import static org.mockito.Mockito.when;
import static ru.yandex.ci.core.db.model.AutoReleaseQueueItem.State.WAITING_PREVIOUS_COMMITS;
import static ru.yandex.ci.core.db.model.AutoReleaseQueueItem.State.WAITING_SCHEDULE;

@NonNullApi
@ExtendWith(MockitoExtension.class)
@MockitoSettings(strictness = Strictness.LENIENT)
class AutoReleaseServiceTest extends CommonTestBase {
    private static final AutoReleaseQueueItem.State WAITING_CONDITIONS =
            AutoReleaseQueueItem.State.WAITING_CONDITIONS;
    private static final AutoReleaseQueueItem.State WAITING_FREE_STAGE =
            AutoReleaseQueueItem.State.WAITING_FREE_STAGE;
    private static final AutoReleaseQueueItem.State CHECKING_FREE_STAGE =
            AutoReleaseQueueItem.State.CHECKING_FREE_STAGE;
    private AutoReleaseService autoReleaseService;
    private AutoReleaseQueue autoReleaseQueue;
    @Mock
    private CiDb db;
    @Mock
    private ConfigurationService configurationService;
    @Mock
    private LaunchService launchService;
    @Mock
    private ArcCommitTable arcCommitTable;
    @Mock
    private AutoReleaseQueueTable autoReleaseQueueTable;
    @Mock
    private CommitDiscoveryProgressTable commitDiscoveryProgressTable;
    @Mock
    private ConfigStateTable configStateTable;
    @Mock
    private AutoReleaseSettingsService autoReleaseSettingsService;
    @Mock
    private DiscoveredCommitTable discoveredCommitTable;
    @Mock
    private StageGroupTable stageGroupTable;
    @Mock
    private RuleEngine ruleEngine;

    @Mock
    private ReleaseScheduler releaseScheduler;

    private final MeterRegistry meterRegistry = new LoggingMeterRegistry();

    @BeforeEach
    public void resetMocks() {
        mockDatabase();

        when(ruleEngine.test(any(), any(), anyList())).thenReturn(Result.launchRelease());

        autoReleaseQueue = spy(
                new AutoReleaseQueue(db, autoReleaseSettingsService, configurationService, Set.of())
        );

        autoReleaseService = spy(
                new AutoReleaseService(
                        db,
                        configurationService,
                        launchService,
                        autoReleaseSettingsService,
                        ruleEngine,
                        releaseScheduler,
                        autoReleaseQueue,
                        meterRegistry,
                        true
                )
        );
    }

    @Test
    void shouldNot_addToAutoReleaseQueue_whenAutoReleaseIsDisabled() {
        var processId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release");
        var rev0 = TestData.TRUNK_R1;

        var discoveredCommit = TestData.createDiscoveredCommit(processId, rev0);
        var releaseConfig = createReleaseConfig();
        doReturn(new AutoReleaseState(false, false)).when(autoReleaseQueue).computeAutoReleaseState(any());
        mockConfig(processId.getPath(), ReleaseConfig.builder()
                .id(processId.getSubId())
                .build());

        autoReleaseService.addToAutoReleaseQueue(
                discoveredCommit, releaseConfig, rev0, DiscoveryType.DIR,
                Set.of(DiscoveryType.DIR)
        );
        verifyNoInteractions(db);
    }

    @Test
    void addToAutoReleaseQueue_whenAutoReleaseIsEnabled() {
        var processId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release");
        var rev0 = TestData.TRUNK_R1;

        var discoveredCommit = TestData.createDiscoveredCommit(processId, rev0);
        var releaseConfig = createReleaseConfig();
        doReturn(new AutoReleaseState(true, false)).when(autoReleaseQueue).computeAutoReleaseState(any());
        mockConfig(processId.getPath(), ReleaseConfig.builder()
                .id(processId.getSubId())
                .build());

        autoReleaseService.addToAutoReleaseQueue(
                discoveredCommit, releaseConfig, rev0, DiscoveryType.DIR,
                Set.of(DiscoveryType.DIR)
        );
        verify(autoReleaseQueueTable).save(eq(AutoReleaseQueueItem.of(
                rev0,
                processId,
                WAITING_PREVIOUS_COMMITS
        )));
    }

    @Test
    void processReleasesWaitingPreviousCommits_whenNoCommitFoundInArcCommitTable() {
        var cProcessId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release");
        doReturn(List.of(
                AutoReleaseQueueItem.of(TestData.TRUNK_R2, cProcessId, CHECKING_FREE_STAGE)
        )).when(autoReleaseQueueTable).findByState(eq(WAITING_PREVIOUS_COMMITS));
        doReturn(Optional.empty()).when(arcCommitTable).findOptional(eq(TestData.TRUNK_R2.getCommitId()));

        autoReleaseService.processReleasesWaitingPreviousCommits();
        verify(autoReleaseQueueTable, times(0)).save(any(AutoReleaseQueueItem.class));
    }

    @Test
    void processReleasesWaitingPreviousCommits_whenCommitParentsNotYetFetched() {
        var cProcessId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release");
        doReturn(List.of(
                AutoReleaseQueueItem.of(TestData.TRUNK_R2, cProcessId, CHECKING_FREE_STAGE)
        )).when(autoReleaseQueueTable).findByState(eq(WAITING_PREVIOUS_COMMITS));

        doReturn(Optional.of(
                ArcCommit.builder()
                        .id(ArcCommit.Id.of(TestData.TRUNK_R2.getCommitId()))
                        .author("author")
                        .message("Message")
                        .createTime(Instant.parse("2019-01-02T10:00:00.000Z"))
                        .parents(List.of("parent-commit-hash", "merge-commit"))
                        .svnRevision(10)
                        .pullRequestId(123)
                        .build()
        )).when(arcCommitTable).findOptional(eq(TestData.TRUNK_R2.getCommitId()));

        autoReleaseService.processReleasesWaitingPreviousCommits();
        verify(autoReleaseQueueTable, times(0)).save(any(AutoReleaseQueueItem.class));
    }

    @Test
    void processReleasesWaitingPreviousCommits() {
        var cProcessId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release");
        var commitsInQueue = List.of(TestData.TRUNK_COMMIT_2, TestData.TRUNK_COMMIT_3, TestData.TRUNK_COMMIT_4);
        var itemsInQueue = commitsInQueue.stream()
                .map(it -> AutoReleaseQueueItem.of(
                        it.toOrderedTrunkArcRevision(), cProcessId, WAITING_PREVIOUS_COMMITS
                ))
                .toList();

        doReturn(itemsInQueue).when(autoReleaseQueueTable).findByState(eq(WAITING_PREVIOUS_COMMITS));

        commitsInQueue.forEach(
                commit -> doReturn(Optional.of(commit)).when(arcCommitTable).findOptional(eq(commit.getCommitId()))
        );

        var commitsWithDiscoveredParents = List.of(TestData.TRUNK_COMMIT_2, TestData.TRUNK_COMMIT_3);
        commitsWithDiscoveredParents.forEach(
                commit -> doReturn(Optional.of(
                        CommitDiscoveryProgress.builder()
                                .arcRevision(commit.toOrderedTrunkArcRevision())
                                .dirDiscoveryFinished(true)
                                .dirDiscoveryFinishedForParents(true)
                                .build()
                )).when(commitDiscoveryProgressTable).find(eq(commit.getCommitId()))
        );

        autoReleaseService.processReleasesWaitingPreviousCommits();
        verify(autoReleaseQueueTable).delete(eq(Set.of(itemsInQueue.get(0).getId())));
        verify(autoReleaseQueueTable).save(eq(itemsInQueue.get(1).withState(WAITING_CONDITIONS)));

        verify(autoReleaseQueueTable, never()).delete(eq(Set.of(itemsInQueue.get(2).getId())));
        verify(autoReleaseQueueTable, never()).save(eq(itemsInQueue.get(2).withState(WAITING_CONDITIONS)));
    }

    @Test
    void processReleasesWaitingConditions_whenQueueIsEmpty() {
        doReturn(List.of()).when(autoReleaseQueueTable)
                .findByState(eq(AutoReleaseQueueItem.State.WAITING_CONDITIONS));
        autoReleaseService.processReleasesWaitingConditions(new AutoReleaseService.ProcessingQueueErrors());

        verify(autoReleaseQueueTable, never()).delete(any(AutoReleaseQueueItem.Id.class));
        verify(autoReleaseQueueTable, never()).save(any(AutoReleaseQueueItem.class));
    }

    @Test
    void checkThatAllParentCommitsWereDiscovered_whenRequiredDirDiscovery() {
        var zeroProgress = CommitDiscoveryProgress.builder()
                .arcRevision(TestData.TRUNK_R2)
                .build();
        var testData = new Object[][]{
                // expected, progress
                {false, zeroProgress},
                {false, zeroProgress.withDirDiscoveryFinished(true)},
                {false, zeroProgress.withDirDiscoveryFinishedForParents(true)},
                {true, zeroProgress.withDirDiscoveryFinished(true).withDirDiscoveryFinishedForParents(true)},

                {false, zeroProgress.withGraphDiscoveryFinished(true)},
                {false, zeroProgress.withGraphDiscoveryFinishedForParents(true)},
                {false, zeroProgress.withGraphDiscoveryFinished(true).withGraphDiscoveryFinishedForParents(true)},
        };

        EntryStream.of(List.of(testData)).forKeyValue((i, inputAndExpected) -> {
            var expected = (boolean) inputAndExpected[0];
            var progress = (CommitDiscoveryProgress) inputAndExpected[1];
            var actual = AutoReleaseService.checkThatAllParentCommitsWereDiscovered(progress,
                    Set.of(DiscoveryType.DIR));

            assertThat(actual)
                    .withFailMessage("%s: input %s, got: %s, expected: %s", i, progress, actual, expected)
                    .isEqualTo(expected);
        });
    }

    @Test
    void checkThatAllParentCommitsWereDiscovered_whenRequiredGraphDiscovery() {
        var zeroProgress = CommitDiscoveryProgress.builder()
                .arcRevision(TestData.TRUNK_R2)
                .build();
        var testData = new Object[][]{
                // expected, progress
                {false, zeroProgress},
                {false, zeroProgress.withDirDiscoveryFinished(true)},
                {false, zeroProgress.withDirDiscoveryFinishedForParents(true)},
                {false, zeroProgress.withDirDiscoveryFinished(true).withDirDiscoveryFinishedForParents(true)},

                {false, zeroProgress.withGraphDiscoveryFinished(true)},
                {false, zeroProgress.withGraphDiscoveryFinishedForParents(true)},
                {true, zeroProgress.withGraphDiscoveryFinished(true).withGraphDiscoveryFinishedForParents(true)},
        };

        EntryStream.of(List.of(testData)).forKeyValue((i, inputAndExpected) -> {
            var expected = (boolean) inputAndExpected[0];
            var progress = (CommitDiscoveryProgress) inputAndExpected[1];
            var actual = AutoReleaseService.checkThatAllParentCommitsWereDiscovered(progress,
                    Set.of(DiscoveryType.GRAPH));

            assertThat(actual)
                    .withFailMessage("%s: input %s, got: %s, expected: %s", i, progress, actual, expected)
                    .isEqualTo(expected);
        });
    }

    @Test
    void checkThatAllParentCommitsWereDiscovered_whenRequiredAnyDiscovery() {
        var zeroProgress = CommitDiscoveryProgress.builder()
                .arcRevision(TestData.TRUNK_R2)
                .build();
        var testData = new Object[][]{
                // expected, progress
                {false, zeroProgress},
                {false, zeroProgress.withDirDiscoveryFinished(true)},
                {false, zeroProgress.withDirDiscoveryFinishedForParents(true)},
                {false, zeroProgress.withDirDiscoveryFinished(true).withDirDiscoveryFinishedForParents(true)},

                {false, zeroProgress.withGraphDiscoveryFinished(true)},
                {false, zeroProgress.withGraphDiscoveryFinishedForParents(true)},
                {false, zeroProgress.withGraphDiscoveryFinished(true).withGraphDiscoveryFinishedForParents(true)},

                {true, zeroProgress.withDirDiscoveryFinished(true).withDirDiscoveryFinishedForParents(true)
                        .withGraphDiscoveryFinished(true).withGraphDiscoveryFinishedForParents(true)
                        .withPciDssState(CommitDiscoveryProgress.PciDssState.PROCESSED)
                        .withDirDiscoveryFinishedForParents(true)
                },
                {false, zeroProgress.withDirDiscoveryFinished(false).withDirDiscoveryFinishedForParents(true)
                        .withGraphDiscoveryFinished(true).withGraphDiscoveryFinishedForParents(true)
                        .withPciDssState(CommitDiscoveryProgress.PciDssState.PROCESSED)
                        .withDirDiscoveryFinishedForParents(true)
                },
                {false, zeroProgress.withDirDiscoveryFinished(true).withDirDiscoveryFinishedForParents(true)
                        .withGraphDiscoveryFinished(false).withGraphDiscoveryFinishedForParents(true)
                        .withPciDssState(CommitDiscoveryProgress.PciDssState.PROCESSED)
                        .withDirDiscoveryFinishedForParents(true)
                },
        };

        EntryStream.of(List.of(testData)).forKeyValue((i, inputAndExpected) -> {
            var expected = (boolean) inputAndExpected[0];
            var progress = (CommitDiscoveryProgress) inputAndExpected[1];
            var actual = AutoReleaseService.checkThatAllParentCommitsWereDiscovered(progress,
                    Set.of(DiscoveryType.DIR, DiscoveryType.GRAPH));

            assertThat(actual)
                    .withFailMessage("%s: input %s, got: %s, expected: %s", i, progress, actual, expected)
                    .isEqualTo(expected);
        });
    }

    @Test
    void processReleasesWaitingConditions() {
        var cProcessId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release");
        var dProcessId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABD, "release");
        var eProcessId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABE, "release");

        // should be deleted, cause there is new cReleaseRev2 for cProcessId
        var cReleaseRev0 = AutoReleaseQueueItem.of(TestData.TRUNK_R1, cProcessId, WAITING_CONDITIONS);
        doReturn(new AutoReleaseState(true, false))
                .when(autoReleaseQueue).computeAutoReleaseState(argMatches(cReleaseRev0));
        doReturn(
                Optional.of(TestData.createDiscoveredCommit(cProcessId, TestData.TRUNK_R1))
        ).when(discoveredCommitTable).findCommit(eq(cProcessId), eq(TestData.TRUNK_R1));

        var cReleaseRev2 = AutoReleaseQueueItem.of(TestData.TRUNK_R2, cProcessId, WAITING_CONDITIONS);
        doReturn(new AutoReleaseState(true, false))
                .when(autoReleaseQueue).computeAutoReleaseState(argMatches(cReleaseRev2));
        doReturn(
                Optional.of(TestData.createDiscoveredCommit(cProcessId, TestData.TRUNK_R2))
        ).when(discoveredCommitTable).findCommit(eq(cProcessId), eq(TestData.TRUNK_R2));

        // should stay in the same state, cause auto release conditions are not satisfied
        var dReleaseRev3 = AutoReleaseQueueItem.of(TestData.TRUNK_R3, dProcessId, WAITING_CONDITIONS);
        doReturn(new AutoReleaseState(true, false))
                .when(autoReleaseQueue).computeAutoReleaseState(argMatches(dReleaseRev3));
        doReturn(Action.WAIT_COMMITS)
                .when(autoReleaseService).checkAutoReleaseConditions(any(), argMatches(dReleaseRev3));
        doReturn(
                Optional.of(TestData.createDiscoveredCommit(dProcessId, TestData.TRUNK_R3))
        ).when(discoveredCommitTable).findCommit(eq(dProcessId), eq(TestData.TRUNK_R3));

        // should be deleted, cause auto release is disabled for eProcessId at TRUNK_R3
        var eReleaseRev4 = AutoReleaseQueueItem.of(TestData.TRUNK_R4, eProcessId, WAITING_CONDITIONS);
        doReturn(new AutoReleaseState(false, false))
                .when(autoReleaseQueue).computeAutoReleaseState(argMatches(eReleaseRev4));
        doReturn(
                Optional.of(TestData.createDiscoveredCommit(eProcessId, TestData.TRUNK_R4))
        ).when(discoveredCommitTable).findCommit(eq(eProcessId), eq(TestData.TRUNK_R4));

        doReturn(List.of(cReleaseRev0, cReleaseRev2, dReleaseRev3, eReleaseRev4))
                .when(autoReleaseQueueTable).findByState(eq(WAITING_CONDITIONS));

        mockConfigs(cProcessId, dProcessId, eProcessId);

        autoReleaseService.processReleasesWaitingConditions(new AutoReleaseService.ProcessingQueueErrors());

        verify(autoReleaseQueueTable).delete(eq(Set.of(cReleaseRev0.getId())));
        verify(autoReleaseQueueTable).save(eq(cReleaseRev2.withState(CHECKING_FREE_STAGE)));
        verify(autoReleaseQueueTable).delete(eq(eReleaseRev4.getId()));
    }

    private void mockConfigs(CiProcessId... releaseProcesses) {
        var sameConfig = Stream.of(releaseProcesses)
                .collect(Collectors.groupingBy(CiProcessId::getPath));

        for (var entry : sameConfig.entrySet()) {
            mockConfig(entry.getKey(), entry.getValue());
        }
    }

    private ConfigBundle mockConfig(Path path, Collection<CiProcessId> processes) {
        var config = createConfig(path, processes);
        doReturn(Optional.of(config)).when(configurationService)
                .getOrCreateConfig(eq(path), any(OrderedArcRevision.class));
        return config;
    }

    private ConfigBundle mockConfig(Path path, ReleaseConfig releaseConfig) {
        var config = createConfig(path, releaseConfig);
        doReturn(Optional.of(config)).when(configurationService)
                .getOrCreateConfig(eq(path), any(OrderedArcRevision.class));

        doReturn(config).when(configurationService)
                .getLastValidConfig(eq(path), any(ArcBranch.class));
        return config;
    }

    @Test
    void scheduleLaunchAfterFlowUnlockedStage_whenThereAreNoPendingAutoReleases() {
        var processId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release");
        var launchId = LaunchId.of(processId, 1);
        var flowLaunchId = FlowLaunchId.of(launchId);

        doReturn(List.of()).when(autoReleaseQueueTable)
                .findByProcessIdAndState(eq(processId), eq(WAITING_FREE_STAGE));
        autoReleaseService.scheduleLaunchAfterFlowUnlockedStage(flowLaunchId, launchId);

        verify(autoReleaseQueueTable, never()).save(any(AutoReleaseQueueItem.class));
    }

    @Test
    void scheduleLaunchAfterFlowUnlockedStage() {
        var processId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release");
        var launchId = LaunchId.of(processId, 1);
        var flowLaunchId = FlowLaunchId.of(launchId);

        var releaseRev0 = AutoReleaseQueueItem.of(TestData.TRUNK_R1, processId, WAITING_CONDITIONS);
        var releaseRev2 = AutoReleaseQueueItem.of(TestData.TRUNK_R2, processId, WAITING_CONDITIONS);

        doReturn(List.of(releaseRev0, releaseRev2))
                .when(autoReleaseQueueTable).findByProcessIdAndState(eq(processId), eq(WAITING_FREE_STAGE));
        autoReleaseService.scheduleLaunchAfterFlowUnlockedStage(flowLaunchId, launchId);

        verify(autoReleaseQueueTable).save(eq(releaseRev0.withState(CHECKING_FREE_STAGE)));
        verify(autoReleaseQueueTable).save(eq(releaseRev2.withState(CHECKING_FREE_STAGE)));
    }

    @Test
    void processReleasesCheckingFreeStage_shouldDeleteObsoleteQueuedReleases() {
        var cProcessId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release");

        var cReleaseRev2 = AutoReleaseQueueItem.of(TestData.TRUNK_R2, cProcessId, CHECKING_FREE_STAGE);
        doReturn(new AutoReleaseState(true, false))
                .when(autoReleaseQueue).computeAutoReleaseState(argMatches(cReleaseRev2));
        doReturn(Action.LAUNCH_RELEASE)
                .when(autoReleaseService).checkAutoReleaseConditions(any(), argMatches(cReleaseRev2));
        // leads to transferring to state WAITING_FREE_STAGE
        doReturn(false).when(autoReleaseService).firstStageInReleaseFlowIsFree(argMatches(cReleaseRev2));
        doReturn(
                Optional.of(TestData.createDiscoveredCommit(cProcessId, TestData.TRUNK_R2))
        ).when(discoveredCommitTable).findCommit(eq(cProcessId), eq(TestData.TRUNK_R2));

        // should be deleted, cause there is newer revision cReleaseRev2 for cProcessId
        var cReleaseRev0 = AutoReleaseQueueItem.of(TestData.TRUNK_R1, cProcessId, CHECKING_FREE_STAGE);
        doReturn(List.of(cReleaseRev0, cReleaseRev2))
                .when(autoReleaseQueueTable).findByState(eq(CHECKING_FREE_STAGE));

        mockConfigs(cProcessId);

        autoReleaseService.processReleasesCheckingFreeStage(new AutoReleaseService.ProcessingQueueErrors());

        verify(autoReleaseQueueTable).delete(eq(Set.of(cReleaseRev0.getId())));
        verify(autoReleaseQueueTable).save(eq(cReleaseRev2.withState(WAITING_FREE_STAGE)));
        verifyNoInteractions(launchService);
    }

    @Test
    void processReleasesCheckingFreeStage_shouldDeleteQueuedRelease_whenAutoReleaseDisabled() {
        var dProcessId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABD, "release");

        var dReleaseRev3 = AutoReleaseQueueItem.of(TestData.TRUNK_R3, dProcessId, CHECKING_FREE_STAGE);
        doReturn(new AutoReleaseState(false, false))
                .when(autoReleaseQueue).computeAutoReleaseState(argMatches(dReleaseRev3));
        doReturn(true).when(autoReleaseService).firstStageInReleaseFlowIsFree(argMatches(dReleaseRev3));
        doReturn(
                Optional.of(TestData.createDiscoveredCommit(dProcessId, TestData.TRUNK_R3))
        ).when(discoveredCommitTable).findCommit(eq(dProcessId), eq(TestData.TRUNK_R3));

        doReturn(List.of(dReleaseRev3))
                .when(autoReleaseQueueTable).findByState(eq(CHECKING_FREE_STAGE));

        mockConfigs(dProcessId);

        autoReleaseService.processReleasesCheckingFreeStage(new AutoReleaseService.ProcessingQueueErrors());

        verify(autoReleaseQueueTable).delete(eq(dReleaseRev3.getId()));
        verifyNoInteractions(launchService);
    }

    @Test
    void processReleasesCheckingFreeStage_shouldChangeStatusToWaitingSchedule() {
        var eProcessId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABE, "release");

        var eReleaseRev4 = AutoReleaseQueueItem.of(TestData.TRUNK_R4, eProcessId, CHECKING_FREE_STAGE);
        // should be deleted, cause auto release is disabled for eProcessId at TRUNK_R3
        doReturn(new AutoReleaseState(true, false))
                .when(autoReleaseQueue).computeAutoReleaseState(argMatches(eReleaseRev4));
        doReturn(Result.scheduleAt(Instant.now().plus(2, ChronoUnit.HOURS)))
                .when(ruleEngine).test(eProcessId, TestData.TRUNK_R4, List.of());
        doReturn(true).when(autoReleaseService).firstStageInReleaseFlowIsFree(argMatches(eReleaseRev4));
        doReturn(
                Optional.of(TestData.createDiscoveredCommit(eProcessId, TestData.TRUNK_R4))
        ).when(discoveredCommitTable).findCommit(eq(eProcessId), eq(TestData.TRUNK_R4));

        doReturn(List.of(eReleaseRev4))
                .when(autoReleaseQueueTable).findByState(eq(CHECKING_FREE_STAGE));

        mockConfigs(eProcessId);

        autoReleaseService.processReleasesCheckingFreeStage(new AutoReleaseService.ProcessingQueueErrors());

        verify(autoReleaseQueueTable).save(eq(eReleaseRev4.withState(WAITING_SCHEDULE)));
        verifyNoInteractions(launchService);
    }

    @Test
    void processReleasesCheckingFreeStage_shouldLaunchRelease() {
        var eProcessId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABE, "release");

        var eReleaseRev4 = AutoReleaseQueueItem.of(TestData.TRUNK_R4, eProcessId, CHECKING_FREE_STAGE);
        // should be deleted, cause auto release is disabled for eProcessId at TRUNK_R3
        doReturn(new AutoReleaseState(true, false))
                .when(autoReleaseQueue).computeAutoReleaseState(argMatches(eReleaseRev4));
        doReturn(Action.LAUNCH_RELEASE)
                .when(autoReleaseService).checkAutoReleaseConditions(any(), argMatches(eReleaseRev4));
        doReturn(true).when(autoReleaseService).firstStageInReleaseFlowIsFree(argMatches(eReleaseRev4));
        doReturn(
                Optional.of(TestData.createDiscoveredCommit(eProcessId, TestData.TRUNK_R4))
        ).when(discoveredCommitTable).findCommit(eq(eProcessId), eq(TestData.TRUNK_R4));

        doReturn(List.of(eReleaseRev4))
                .when(autoReleaseQueueTable).findByState(eq(CHECKING_FREE_STAGE));

        var configBundle = mockConfig(eProcessId.getPath(), List.of(eProcessId));

        autoReleaseService.processReleasesCheckingFreeStage(new AutoReleaseService.ProcessingQueueErrors());

        verify(autoReleaseQueueTable).delete(eq(eReleaseRev4.getId()));
        verify(launchService).startRelease(
                eq(eProcessId),
                eq(eReleaseRev4.getOrderedArcRevision().toRevision()),
                eq(ArcBranch.trunk()),
                eq(UserUtils.loginForInternalCiProcesses()),
                eq(configBundle.getRevision()),
                eq(false),
                eq(false),
                isNull(),
                eq(false),
                isNull(),
                isNull(),
                isNull()
        );
    }

    @Test
    void waitMinimumCommits() {
        var eProcessId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABE, "release");

        var eReleaseRev4 = AutoReleaseQueueItem.of(TestData.TRUNK_R4, eProcessId, WAITING_CONDITIONS);

        doReturn(
                Optional.of(TestData.createDiscoveredCommit(eProcessId, TestData.TRUNK_R4))
        ).when(discoveredCommitTable).findCommit(eq(eProcessId), eq(TestData.TRUNK_R4));

        doReturn(List.of(eReleaseRev4))
                .when(autoReleaseQueueTable).findByState(eq(WAITING_CONDITIONS));

        var conditions = List.of(new Conditions(7, null, null));
        mockConfig(eProcessId.getPath(), ReleaseConfig.builder()
                .id(eProcessId.getSubId())
                .auto(new AutoReleaseConfig(true, conditions))
                .build());

        when(ruleEngine.test(eProcessId, TestData.TRUNK_R4, conditions)).thenReturn(Result.waitCommits());

        autoReleaseService.processAutoReleaseQueue();
        verify(autoReleaseQueueTable).delete(eReleaseRev4.getId());

        when(ruleEngine.test(eProcessId, TestData.TRUNK_R4, conditions)).thenReturn(Result.launchRelease());

        autoReleaseService.processAutoReleaseQueue();

        verify(autoReleaseQueueTable).save(eReleaseRev4.withState(CHECKING_FREE_STAGE));
    }

    @Test
    void firstStageInReleaseFlowIsFree_returnsTrue_whenStageGroupStateNotFound()
            throws NotEligibleForAutoReleaseException {
        var eProcessId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABE, "release");
        var discoveredCommit = TestData.createDiscoveredCommit(eProcessId, TestData.TRUNK_R4);
        var releaseConfig = createReleaseConfig();

        var context = AutoReleaseContext.create(
                discoveredCommit, releaseConfig, TestData.TRUNK_R4
        );
        assertThat(autoReleaseService.firstStageInReleaseFlowIsFree(context))
                .isTrue();
    }

    @Test
    void firstStageInReleaseFlowIsFree_usesImplicitStage_whenReleaseStagesAreEmpty()
            throws NotEligibleForAutoReleaseException {
        var eProcessId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABE, "release");
        var discoveredCommit = TestData.createDiscoveredCommit(eProcessId, TestData.TRUNK_R4);
        var releaseConfig = createReleaseConfig(List.of(), false);

        var context = AutoReleaseContext.create(
                discoveredCommit, releaseConfig, TestData.TRUNK_R4
        );

        doReturn(Optional.of(StageGroupState.of(context.getStageGroupId(), 1L)))
                .when(stageGroupTable).findOptional(eq(context.getStageGroupId()));
        assertThat(autoReleaseService.firstStageInReleaseFlowIsFree(context)).isTrue();
    }

    @Test
    void firstStageInReleaseFlowIsFree() throws NotEligibleForAutoReleaseException {
        var eProcessId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABE, "release");
        var discoveredCommit = TestData.createDiscoveredCommit(eProcessId, TestData.TRUNK_R4);
        var releaseConfig = createReleaseConfig();

        var context = AutoReleaseContext.create(
                discoveredCommit, releaseConfig, TestData.TRUNK_R4
        );

        doReturn(Optional.of(StageGroupState.of(context.getStageGroupId(), 1L)))
                .when(stageGroupTable).findOptional(eq(context.getStageGroupId()));

        assertThat(autoReleaseService.firstStageInReleaseFlowIsFree(context))
                .isTrue();
    }

    @Test
    void processReleasesWaitingFreeStage() {
        var cProcessId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release");

        var cReleaseRev2 = AutoReleaseQueueItem.of(TestData.TRUNK_R2, cProcessId, WAITING_FREE_STAGE);
        doReturn(
                Optional.of(TestData.createDiscoveredCommit(cProcessId, TestData.TRUNK_R2))
        ).when(discoveredCommitTable).findCommit(eq(cProcessId), eq(TestData.TRUNK_R2));

        // should be deleted, cause there is newer revision cReleaseRev2 for cProcessId
        var cReleaseRev0 = AutoReleaseQueueItem.of(TestData.TRUNK_R1, cProcessId, WAITING_FREE_STAGE);
        doReturn(List.of(cReleaseRev0, cReleaseRev2))
                .when(autoReleaseQueueTable).findByState(eq(WAITING_FREE_STAGE));

        mockConfigs(cProcessId);

        autoReleaseService.processReleasesWaitingFreeStage();

        verify(autoReleaseQueueTable).delete(eq(Set.of(cReleaseRev0.getId())));
        verifyNoInteractions(launchService);
    }

    @Test
    void findAutoReleaseStateOrDefault_forCiProcessId() {
        var processId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release-1");
        var spyAutoReleaseService = spy(autoReleaseService);
        doReturn(Map.of()).when(spyAutoReleaseService).findAutoReleaseStateOrDefault(anyList());

        spyAutoReleaseService.findAutoReleaseStateOrDefault(processId);
        verify(spyAutoReleaseService).findAutoReleaseStateOrDefault(eq(List.of(processId)));
    }

    @Test
    void findAutoReleaseStateOrDefault_forListOfCiProcessIds_whenListIsEmpty() {
        doReturn(Map.of()).when(autoReleaseSettingsService)
                .findLastForProcessIds(eq(List.of()));
        assertThat(
                autoReleaseService.findAutoReleaseStateOrDefault(List.of())
        ).isEmpty();
    }

    @Test
    void findAutoReleaseStateOrDefault_forListOfCiProcessIds_whenConfigBundleNotFound() {
        var eRelease = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABE, "release");
        doThrow(NoSuchElementException.class).when(configStateTable).get(eq(TestData.CONFIG_PATH_ABE));

        assertThat(
                autoReleaseService.findAutoReleaseStateOrDefault(List.of(eRelease))
        ).contains(Map.entry(eRelease, AutoReleaseState.DEFAULT));
    }

    @Test
    void findAutoReleaseStateOrDefault_forListOfCiProcessIds() {
        // has no AutoReleaseSettingsHistory
        var cRelease1 = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release-1");
        // has AutoReleaseSettingsHistory
        var cRelease2 = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release-2");
        doReturn(Map.of(
                cRelease2, AutoReleaseSettingsHistory.of(
                        cRelease2, false, "login", "message", Instant.parse("2020-01-02T10:00:00.000Z")
                )
        )).when(autoReleaseSettingsService)
                .findLastForProcessIds(eq(List.of(cRelease1, cRelease2)));

        doReturn(Map.of(
                TestData.CONFIG_PATH_ABC,
                ConfigState.builder()
                        .configPath(TestData.CONFIG_PATH_ABC)
                        .created(Instant.parse("2020-01-02T10:00:00.000Z"))
                        .updated(Instant.parse("2020-01-02T10:00:00.000Z"))
                        .lastRevision(TestData.TRUNK_R4)
                        .status(ConfigState.Status.OK)
                        .releases(List.of(
                                        ReleaseConfigState.builder()
                                                .releaseId("release-2")
                                                .title("Title")
                                                .auto(new AutoReleaseConfigState(true))
                                                .build()
                                )
                        )
                        .build()
        )).when(configStateTable).findByIds(argThat(a -> a.contains(TestData.CONFIG_PATH_ABC)));

        assertThat(
                autoReleaseService.findAutoReleaseStateOrDefault(List.of(cRelease1, cRelease2))
        ).contains(
                Map.entry(cRelease1, new AutoReleaseState(false, false, null)),
                Map.entry(cRelease2, new AutoReleaseState(
                        true,
                        false,
                        new AutoReleaseSettingsHistory(
                                AutoReleaseSettingsHistory.Id.of(
                                        "a/b/c/a.yaml:r:release-2", Instant.parse("2020-01-02T10:00:00.000Z")
                                ),
                                false, "login", "message"
                        )
                ))
        );
    }

    @Test
    void computeAutoReleaseState() {
        final var processId = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "release");
        final var timestamp = Instant.parse("2020-01-02T10:00:00.000Z");

        assertThat(autoReleaseQueue.computeAutoReleaseState(null, null, processId))
                .isEqualTo(new AutoReleaseState(false, false, null));

        assertThat(
                autoReleaseQueue.computeAutoReleaseState(
                        createReleaseState(false), null, processId
                )
        ).isEqualTo(new AutoReleaseState(false, false, null));

        assertThat(
                autoReleaseQueue.computeAutoReleaseState(
                        createReleaseState(true), null, processId
                )
        ).isEqualTo(new AutoReleaseState(true, false, null));

        assertThat(
                autoReleaseQueue.computeAutoReleaseState(
                        createReleaseState(true),
                        AutoReleaseSettingsHistory.of(
                                processId, true, "login", "message", timestamp
                        ),
                        processId
                )
        ).isEqualTo(new AutoReleaseState(
                true,
                false,
                new AutoReleaseSettingsHistory(
                        AutoReleaseSettingsHistory.Id.of("a/b/c/a.yaml:r:release", timestamp),
                        true, "login", "message"
                )
        ));

        assertThat(
                autoReleaseQueue.computeAutoReleaseState(
                        createReleaseState(true),
                        AutoReleaseSettingsHistory.of(
                                processId, false, "login", "message", timestamp
                        ),
                        processId
                )
        ).isEqualTo(new AutoReleaseState(
                true,
                false,
                new AutoReleaseSettingsHistory(
                        AutoReleaseSettingsHistory.Id.of("a/b/c/a.yaml:r:release", timestamp),
                        false, "login", "message"
                )
        ));
    }

    private void mockDatabase() {
        TestCiDbUtils.mockToCallRealTxMethods(db);
        doReturn(autoReleaseQueueTable).when(db).autoReleaseQueue();
        doReturn(commitDiscoveryProgressTable).when(db).commitDiscoveryProgress();
        doReturn(configStateTable).when(db).configStates();
        doReturn(discoveredCommitTable).when(db).discoveredCommit();
        doReturn(stageGroupTable).when(db).stageGroup();
        doReturn(arcCommitTable).when(db).arcCommit();
    }

    private static ReleaseConfig createReleaseConfig() {
        return createReleaseConfig(List.of(
                StageConfig.builder().id("testing").title("Тестинг").build(),
                StageConfig.builder().id("stable").title("Продакшейн").build()
        ), false);
    }

    private static ReleaseConfig createReleaseConfig(List<StageConfig> stages, boolean autoRelease) {
        return ReleaseConfig.builder()
                .id("my-app").title("My Application").flow("my-app-release")
                .stages(stages)
                .filters(List.of(
                        FilterConfig.builder()
                                .discovery(FilterConfig.Discovery.DIR)
                                .stQueues(List.of("TESTENV"))
                                .build(),
                        FilterConfig.builder()
                                .discovery(FilterConfig.Discovery.GRAPH)
                                .authorServices(List.of("ci", "testenv"))
                                .build(),
                        FilterConfig.builder()
                                .discovery(FilterConfig.Discovery.GRAPH)
                                .stQueues(List.of("CI"))
                                .build()
                ))
                .auto(new AutoReleaseConfig(autoRelease))
                .build();
    }

    private static ConfigBundle createConfig(Path configPath, Collection<CiProcessId> releaseProcesses) {
        var releaseConfigs = releaseProcesses.stream()
                .map(p -> ReleaseConfig.builder()
                        .id(p.getSubId())
                        .build()
                )
                .toArray(ReleaseConfig[]::new);
        return createConfig(configPath, releaseConfigs);
    }

    private static ConfigBundle createConfig(Path configPath, ReleaseConfig... configs) {
        var entity = new ConfigEntity(
                configPath,
                TestData.TRUNK_R4,
                ConfigStatus.READY,
                List.of(),
                Collections.emptyNavigableMap(),
                new ConfigSecurityState(
                        YavToken.Id.of("yavTokenUuid"),
                        ConfigSecurityState.ValidationStatus.CONFIG_NOT_CHANGED
                ),
                new ConfigCreationInfo(Instant.now(), null, null, null),
                null,
                null
        );
        var ciConfig = CiConfig.builder();
        ciConfig.release(configs);
        return new ConfigBundle(
                entity,
                new AYamlConfig("ci", "Woodcutter", ciConfig.build(), null),
                Collections.emptyNavigableMap()
        );
    }

    private static ReleaseConfigState createReleaseState(boolean autoRelease) {
        return ReleaseConfigState.builder()
                .releaseId("release")
                .title("My Application")
                .auto(new AutoReleaseConfigState(autoRelease))
                .build();
    }

    private static AutoReleaseContext argMatches(AutoReleaseQueueItem item) {
        return argThat(
                it -> it.getProcessId().asString().equals(item.getId().getProcessId())
                        && it.getRevision().equals(item.getOrderedArcRevision())
        );
    }

}
