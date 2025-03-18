package ru.yandex.ci.engine.launch;

import java.time.Instant;
import java.time.LocalDateTime;
import java.time.Month;
import java.time.ZoneOffset;
import java.util.List;
import java.util.Set;

import javax.annotation.Nullable;

import com.google.gson.JsonObject;
import org.assertj.core.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementId;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.Branch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.config.a.ConfigChangeType;
import ru.yandex.ci.core.config.a.model.RuntimeConfig;
import ru.yandex.ci.core.config.a.model.RuntimeSandboxConfig;
import ru.yandex.ci.core.discovery.ConfigChange;
import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.ci.core.launch.Activity;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchPullRequestInfo;
import ru.yandex.ci.core.launch.LaunchRuntimeInfo;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.launch.LaunchVcsInfo;
import ru.yandex.ci.core.launch.ReleaseVcsInfo;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.pr.PullRequestVcsInfo;
import ru.yandex.ci.core.tasklet.SchemaService;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.core.timeline.Offset;
import ru.yandex.ci.core.timeline.TimelineItem;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.engine.branch.BranchService;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.engine.event.LaunchEventTask;
import ru.yandex.ci.engine.launch.LaunchService.LaunchMode;
import ru.yandex.ci.engine.timeline.TimelineService;
import ru.yandex.lang.NonNullApi;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;
import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.Mockito.clearInvocations;
import static org.mockito.Mockito.verify;

@NonNullApi
class LaunchServiceTest extends EngineTestBase {

    @Autowired
    private TimelineService timelineService;

    @Autowired
    private BranchService branchService;

    @Autowired
    private SchemaService schemaService;

    @BeforeEach
    void setUp() {
        mockValidationSuccessful();
        clearInvocations(bazingaTaskManagerStub);
    }

    @Test
    void mergeFlowVars_simple() {
        Assertions.assertThat(merge(null, null))
                .isEqualTo(null);

        JsonObject object1 = new JsonObject();
        object1.addProperty("key1", 42);

        Assertions.assertThat(merge(object1, null))
                .isEqualTo(object1);

        Assertions.assertThat(merge(null, object1))
                .isEqualTo(object1);

        JsonObject object2 = new JsonObject();
        object1.addProperty("key2", 21);

        JsonObject expectedObject1 = new JsonObject();
        expectedObject1.addProperty("key1", 42);
        expectedObject1.addProperty("key2", 21);

        Assertions.assertThat(merge(object2, object1))
                .isEqualTo(expectedObject1);

        Assertions.assertThat(merge(object1, object2))
                .isEqualTo(expectedObject1);
    }

    @Test
    void mergeFlowVars_override() {

        JsonObject object1 = new JsonObject();
        object1.addProperty("key1", 21);
        object1.addProperty("key2", 42);

        JsonObject object2 = new JsonObject();
        object2.addProperty("key2", 721);
        object2.addProperty("key3", 742);


        JsonObject expectedObject1 = new JsonObject();
        expectedObject1.addProperty("key1", 21);
        expectedObject1.addProperty("key2", 721);
        expectedObject1.addProperty("key3", 742);

        Assertions.assertThat(merge(object1, object2))
                .isEqualTo(expectedObject1);


        JsonObject expectedObject2 = new JsonObject();
        expectedObject2.addProperty("key2", 42);
        expectedObject2.addProperty("key3", 742);
        expectedObject2.addProperty("key1", 21);

        Assertions.assertThat(merge(object2, object1))
                .isEqualTo(expectedObject2);

    }

    @Nullable
    private JsonObject merge(@Nullable JsonObject o, @Nullable JsonObject o2) {
        return schemaService.override(o, o2);
    }

    @Test
    void titleSourceFromRelease() {
        var processId = TestData.SIMPLE_RELEASE_PROCESS_ID;

        discoveryToR2();
        delegateToken(TestData.CONFIG_PATH_SIMPLE_RELEASE);

        assertThat(db.currentOrReadOnly(() -> db.launches().getLaunches(processId, -1, -1)))
                .isEmpty();

        var launch = startRelease(processId, TestData.TRUNK_R2, "Ninja");
        assertThat(launch.getTitle()).isEqualTo("Woodcutter release #1");
    }

    @Test
    void launchVcsInfo() {
        CiProcessId processId = TestData.SIMPLE_RELEASE_PROCESS_ID;

        discoveryToR2();
        delegateToken(TestData.CONFIG_PATH_SIMPLE_RELEASE);

        assertThat(db.currentOrReadOnly(() -> db.launches().getLaunches(processId, -1, -1)))
                .isEmpty();

        Launch launch = startRelease(processId, TestData.TRUNK_R2, "Ninja");
        LaunchVcsInfo vcsInfo = launch.getVcsInfo();
        assertThat(launch.getTriggeredBy()).contains("Ninja");
        assertThat(vcsInfo).isEqualTo(LaunchVcsInfo.builder()
                .revision(TestData.TRUNK_R2)
                .commit(TestData.TRUNK_COMMIT_2)
                .releaseVcsInfo(ReleaseVcsInfo.builder().build())
                .commitCount(1)
                .build()
        );

        discoveryToR7();
        Launch launch2 = startRelease(processId, TestData.TRUNK_R7, "Cowboy");
        LaunchVcsInfo vcsInfo2 = launch2.getVcsInfo();
        assertThat(launch2.getTriggeredBy()).contains("Cowboy");
        assertThat(vcsInfo2).isEqualTo(LaunchVcsInfo.builder()
                .revision(TestData.TRUNK_R7)
                .commit(TestData.TRUNK_COMMIT_7)
                .previousRevision(TestData.TRUNK_R2)
                .releaseVcsInfo(ReleaseVcsInfo.builder()
                        .previousRevision(TestData.TRUNK_R2)
                        .build())
                .commitCount(5)
                .build()
        );
    }

    @Test
    void launchVcsInfoWithoutCommitInDB() {
        CiProcessId processId = TestData.SIMPLE_RELEASE_PROCESS_ID;

        discoveryToR2();
        delegateToken(TestData.CONFIG_PATH_SIMPLE_RELEASE);

        assertThat(db.currentOrReadOnly(() -> db.launches().getLaunches(processId, -1, -1)))
                .isEmpty();

        db.currentOrTx(() -> db.arcCommit().delete(TestData.TRUNK_R2.toCommitId()));
        db.currentOrTx(() -> db.arcCommit().delete(TestData.TRUNK_COMMIT_2.getId()));

        Launch launch = startRelease(processId, TestData.TRUNK_R2, "Ninja");
        LaunchVcsInfo vcsInfo = launch.getVcsInfo();
        assertThat(launch.getTriggeredBy()).contains("Ninja");
        assertThat(vcsInfo).isEqualTo(LaunchVcsInfo.builder()
                .revision(TestData.TRUNK_R2)
                .commit(TestData.TRUNK_COMMIT_2)
                .releaseVcsInfo(ReleaseVcsInfo.builder().build())
                .commitCount(1)
                .build()
        );

        discoveryToR7();
        Launch launch2 = startRelease(processId, TestData.TRUNK_R7, "Cowboy");
        LaunchVcsInfo vcsInfo2 = launch2.getVcsInfo();
        assertThat(launch2.getTriggeredBy()).contains("Cowboy");
        assertThat(vcsInfo2).isEqualTo(LaunchVcsInfo.builder()
                .revision(TestData.TRUNK_R7)
                .commit(TestData.TRUNK_COMMIT_7)
                .previousRevision(TestData.TRUNK_R2)
                .releaseVcsInfo(ReleaseVcsInfo.builder()
                        .previousRevision(TestData.TRUNK_R2)
                        .build())
                .commitCount(5)
                .build()
        );
    }

    @Test
    void forbidStartingReleaseInBranchOnTrunkCommit() {
        CiProcessId processId = TestData.WITH_BRANCHES_RELEASE_PROCESS_ID;

        discoveryToR6();
        delegateToken(processId.getPath());

        var createdBranch = createBranch(processId, TestData.TRUNK_R6);

        assertThatThrownBy(() -> startRelease(processId, TestData.TRUNK_R6, "Cowboy"))
                .hasMessage("Cannot start launch at revision r6 in branch trunk: " +
                        "forbidden at branch trunk, cause commit belongs to branch " +
                        "releases/ci-test/test-sawmill-release-1");

        assertThatThrownBy(() -> launchService.startRelease(
                processId,
                TestData.TRUNK_R5.toRevision(),
                createdBranch.getArcBranch(),
                TestData.CI_USER,
                null,
                false,
                false,
                null,
                true,
                null,
                null, null))
                .hasMessage("Cannot start launch at revision r5 in branch releases/ci-test/test-sawmill-release-1: " +
                        "forbidden at earlier revision then base branch revision r6");
    }

    @Test
    void forbidTrunkReleases() {
        CiProcessId processId = TestData.WITH_BRANCHES_FORBID_TRUNK_RELEASE_PROCESS_ID;

        discoveryToR6();
        delegateToken(processId.getPath());

        assertThatThrownBy(() -> startRelease(processId, TestData.TRUNK_R6, "Cowboy"))
                .hasMessage("Cannot start launch in branch trunk," +
                        " releases in trunk is forbidden by configuration in a.yaml");

        var createdBranch = createBranch(processId, TestData.TRUNK_R6);

        assertThat(startReleaseCustomBranch(processId, TestData.TRUNK_R6, createdBranch.getArcBranch()))
                .isNotNull();
    }

    @Test
    void overrideRuntime() {
        CiProcessId processId = CiProcessId.ofRelease(
                TestData.CONFIG_PATH_ACTION_CUSTOM_RUNTIME, "my-release"
        );

        discoveryToR2();
        delegateToken(processId.getPath());

        var launch = startRelease(processId, TestData.TRUNK_R2, "Cowboy");
        var runtimeInfo = launch.getFlowInfo().getRuntimeInfo();
        assertThat(runtimeInfo.getSandboxOwner()).isNull();
        assertThat(runtimeInfo.getRuntimeConfig()).isEqualTo(RuntimeConfig.builder()
                .sandbox(RuntimeSandboxConfig.builder()
                        .owner("CI")
                        .tags(List.of("ADDED-TAG", "ADDED-SECOND-TAG"))
                        .build())
                .build());
    }

    private Branch createBranch(CiProcessId processId, OrderedArcRevision revision) {
        return db.currentOrTx(() -> branchService.createBranch(processId, revision, TestData.CI_USER));
    }

    @Test
    void startReleaseShouldCreateAutoBranch() {
        CiProcessId processId = TestData.WITH_AUTO_BRANCHES_RELEASE_PROCESS_ID;

        discoveryToR2();
        delegateToken(processId.getPath());

        assertThat(db.currentOrReadOnly(() -> db.launches().getLaunches(processId, -1, -1)))
                .isEmpty();

        Launch launch = startRelease(processId, TestData.TRUNK_R2, "Ninja");
        LaunchVcsInfo vcsInfo = launch.getVcsInfo();

        var autoCreatedBranch = ArcBranch.ofBranchName("releases/ci-test/test-sawmill-release-1");
        assertThat(launch.getTriggeredBy()).contains("Ninja");
        assertThat(vcsInfo).isEqualTo(LaunchVcsInfo.builder()
                .revision(TestData.TRUNK_R2)
                .commit(TestData.TRUNK_COMMIT_2)
                .selectedBranch(autoCreatedBranch)
                .releaseVcsInfo(ReleaseVcsInfo.builder().build())
                .commitCount(1)
                .build()
        );

        var trunkTimeline = db.currentOrReadOnly(() ->
                timelineService.getTimeline(processId, ArcBranch.trunk(), Offset.EMPTY, -1));

        // в транковом таймлайне только ветка
        assertThat(trunkTimeline)
                .hasSize(1)
                .extracting(TimelineItem::getBranch)
                .element(0)
                .isNotNull();

        var branch = trunkTimeline.get(0).getBranch();
        assertThat(branch.getArcBranch()).isEqualTo(autoCreatedBranch);
        assertThat(branch.getVersion()).isNotNull().isEqualTo(launch.getVersion());
        assertThat(branch.getState().getActiveLaunches()).containsExactly(launch.getLaunchId().getNumber());


        var branchTimeline = db.currentOrReadOnly(() ->
                timelineService.getTimeline(processId, branch.getArcBranch(), Offset.EMPTY, -1));

        // релиз переехал в ветку
        assertThat(branchTimeline)
                .extracting(TimelineItem::getLaunch)
                .containsExactly(launch);
    }

    @Test
    void startReleaseAndCancelInStages() {
        CiProcessId processId = TestData.WITH_AUTO_BRANCHES_RELEASE_PROCESS_ID;

        discoveryToR7();
        delegateToken(processId.getPath());

        Launch launchR1 = startRelease(processId, TestData.TRUNK_R2);
        Launch launchR5 = startRelease(processId, TestData.TRUNK_R6);

        assertThat(launchR1.getFlowInfo().getStageGroupId())
                .isNotEqualTo(launchR5.getFlowInfo().getStageGroupId());

        var createdBranch = ArcBranch.ofBranchName(launchR5.getSelectedBranch());
        assertThat(createdBranch.isRelease()).isTrue();

        discoveryServicePostCommits.processPostCommit(createdBranch, TestData.RELEASE_R6_1.toRevision(), false);
        var revision = TestData.projectRevToBranch(TestData.RELEASE_R6_1, createdBranch);

        Launch launchR5n1 = startRelease(processId, revision, TestData.CI_USER, true);

        assertThat(launchR5.getFlowInfo().getStageGroupId())
                .isEqualTo(launchR5n1.getFlowInfo().getStageGroupId());

        launchR1 = getLaunch(launchR1.getLaunchId());
        assertThat(launchR1.getStatus()).isEqualTo(LaunchState.Status.STARTING);

        launchR5 = getLaunch(launchR5.getLaunchId());
        assertThat(launchR5.getStatus()).isEqualTo(LaunchState.Status.CANCELLING);
    }

    @Test
    void startReleaseAndCancelOthers() {
        CiProcessId processId = TestData.SIMPLE_RELEASE_PROCESS_ID;

        discoveryToR7();
        delegateToken(TestData.CONFIG_PATH_SIMPLE_RELEASE);

        db.currentOrReadOnly(() ->
                assertThat(db.launches().getLaunches(processId, -1, -1, false))
                        .isEmpty()
        );

        Launch launch1 = startRelease(processId, TestData.TRUNK_R5, "andreevdm", true);
        db.currentOrReadOnly(() ->
                assertThat(db.launches().getActiveLaunches(processId)).hasSize(1)
        );
        // assume that flowLaunch has been launched
        db.currentOrTx(() -> {
            db.launches().save(
                    launch1.toBuilder()
                            .flowLaunchId(FlowFullId.of(processId.getPath(), "flow-id").asString())
                            .build()
            );
        });

        Launch launch2 = startRelease(processId, TestData.TRUNK_R6, "andreevdm", true);

        verify(bazingaTaskManagerStub).schedule(argThat(task ->
                (task instanceof LaunchStartTask) &&
                        launch1.getLaunchId().equals(
                                ((LaunchStartTask.Params) task.getParameters()).getLaunchId()
                        )
        ));
        verify(bazingaTaskManagerStub).schedule(argThat(task ->
                (task instanceof LaunchEventTask) &&
                        launch1.getLaunchId().equals(
                                ((LaunchEventTask.Params) task.getParameters()).getLaunchId()
                        ) &&
                        LaunchState.Status.STARTING.equals(
                                ((LaunchEventTask.Params) task.getParameters()).getLaunchStatus()
                        )
        ));
        verify(bazingaTaskManagerStub).schedule(argThat(task ->
                (task instanceof LaunchCancelTask) &&
                        launch1.getLaunchId().equals(
                                ((LaunchCancelTask.Params) task.getParameters()).getLaunchId()
                        )
        ));
        verify(bazingaTaskManagerStub).schedule(argThat(task ->
                (task instanceof LaunchStartTask) &&
                        launch2.getLaunchId().equals(
                                ((LaunchStartTask.Params) task.getParameters()).getLaunchId()
                        )
        ));
        verify(bazingaTaskManagerStub).schedule(argThat(task ->
                (task instanceof LaunchEventTask) &&
                        launch2.getLaunchId().equals(
                                ((LaunchEventTask.Params) task.getParameters()).getLaunchId()
                        ) &&
                        LaunchState.Status.STARTING.equals(
                                ((LaunchEventTask.Params) task.getParameters()).getLaunchStatus()
                        )
        ));

        db.currentOrReadOnly(() ->
                assertThat(db.launches().getLaunches(processId, -1, -1, false))
                        .containsExactly(
                                launch2,
                                launch1.toBuilder()
                                        .flowLaunchId(FlowFullId.of(processId.getPath(), "flow-id").asString())
                                        .status(LaunchState.Status.CANCELLING)
                                        .statusText("")
                                        .cancelledBy("andreevdm")
                                        .cancelledReason("Cancelled by another release")
                                        .build()
                        )
        );
    }

    @Test
    void startVersionOfRelease() {
        CiProcessId processId = CiProcessId.ofRelease(
                TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-release-with-start-version"
        );

        discoveryToR6();
        delegateToken(processId.getPath());

        Launch firstLaunch = startRelease(processId, TestData.TRUNK_R2, TestData.CI_USER);
        assertThat(firstLaunch.getVersion()).isEqualTo(Version.major("42"));

        Launch nextLaunch = startRelease(processId, TestData.TRUNK_R6, TestData.CI_USER);
        assertThat(nextLaunch.getVersion()).isEqualTo(Version.major("43"));
    }

    @Test
    void getTokenFromTrunk() {
        CiProcessId processId = TestData.WITH_BRANCHES_RELEASE_PROCESS_ID;

        discoveryToR6();
        delegateToken(processId.getPath());

        var branch = createBranch(processId, TestData.TRUNK_R6);
        discovery(branch, TestData.RELEASE_R6_1, TestData.RELEASE_R6_2);

        var revision = TestData.projectRevToBranch(TestData.RELEASE_R6_2, branch.getArcBranch());
        var launch = startRelease(processId,
                revision,
                TestData.CI_USER
        );
        assertThat(launch).isNotNull();
        assertThat(launch.getVcsInfo().getRevision()).isEqualTo(revision);
    }

    private void discovery(Branch branch, OrderedArcRevision... revisions) {
        for (var rev : revisions) {
            discoveryServicePostCommits.processPostCommit(branch.getArcBranch(), rev.toRevision(), false);
        }
    }

    @Test
    void cancelRelease() {
        CiProcessId processId = TestData.SIMPLE_RELEASE_PROCESS_ID;

        discoveryToR7();
        delegateToken(TestData.CONFIG_PATH_SIMPLE_RELEASE);

        assertThat(db.currentOrReadOnly(() -> db.launches().getLaunches(processId, -1, -1, false)))
                .isEmpty();

        Launch r3Launch = startRelease(processId, TestData.TRUNK_R4);
        db.currentOrTx(() -> db.launches().save(
                r3Launch.toBuilder()
                        .status(LaunchState.Status.SUCCESS)
                        .build()
        ));
        Launch r4Launch = startRelease(processId, TestData.TRUNK_R5);
        Launch r5Launch = startRelease(processId, TestData.TRUNK_R6);

        launchService.cancel(r4Launch.getLaunchId(), "andreevdm", "some reason");

        verify(bazingaTaskManagerStub).schedule(argThat(task ->
                (task instanceof LaunchCancelTask) &&
                        r4Launch.getLaunchId().equals(
                                ((LaunchCancelTask.Params) task.getParameters()).getLaunchId()
                        )
        ));

        assertThat(db.currentOrReadOnly(() -> db.launches().getLaunches(processId, -1, -1, false)))
                .contains(
                        r3Launch.toBuilder().status(LaunchState.Status.SUCCESS).build(),
                        r5Launch,
                        r4Launch.toBuilder()
                                .status(LaunchState.Status.CANCELLING)
                                .statusText("")
                                .cancelledBy("andreevdm")
                                .cancelledReason("some reason")
                                .build()
                );


        Instant time = LocalDateTime.of(2015, Month.APRIL, 1, 14, 42).toInstant(ZoneOffset.UTC);
        clock.setTime(time);

        executeBazingaTasks(LaunchCancelTask.class);

        assertThat(db.currentOrReadOnly(() -> db.launches().getLaunches(processId, -1, -1, false)))
                .contains(
                        r5Launch.toBuilder()
                                .vcsInfo(
                                        r5Launch.getVcsInfo().toBuilder()
                                                .previousRevision(TestData.TRUNK_R4)
                                                .commitCount(2)
                                                .releaseVcsInfo(ReleaseVcsInfo.builder()
                                                        .stableRevision(TestData.TRUNK_R4)
                                                        .previousRevision(TestData.TRUNK_R4)
                                                        .build()
                                                )
                                                .build()
                                )
                                .activity(Activity.ACTIVE)
                                .cancelledReleases(Set.of(2))
                                .build(),
                        r4Launch.toBuilder()
                                .status(LaunchState.Status.CANCELED)
                                .activity(Activity.FINISHED)
                                .activityChanged(Instant.now(clock))
                                .finished(Instant.now(clock))
                                .statusText("")
                                .cancelledBy("andreevdm")
                                .cancelledReason("some reason")
                                .build()
                );
    }

    @Test
    void startDelayedLaunches() {
        CiProcessId processId = TestData.SIMPLE_FLOW_PROCESS_ID;

        discoveryToR7();
        delegateToken(TestData.CONFIG_PATH_SIMPLE_RELEASE);

        OrderedArcRevision prRevision = OrderedArcRevision.fromRevision(ArcRevision.of(TestData.TRUNK_R2.getCommitId()),
                ArcBranch.ofPullRequest(1L), 100L, 0);
        db.currentOrTx(() -> db.discoveredCommit().updateOrCreate(
                processId, prRevision,
                new ConfigChange(ConfigChangeType.NONE),
                DiscoveryType.DIR
        ));

        ConfigBundle configBundle = configurationService.getLastConfig(processId.getPath(), ArcBranch.trunk());
        Launch prLaunch = launchService.startPrFlow(
                processId,
                prRevision,
                "andreevdm",
                configBundle,
                createLaunchPullRequestInfo(),
                LaunchMode.DELAY,
                true,
                null
        );

        launchService.startDelayedLaunches(processId.getPath(), configBundle.getRevision().toRevision());

        verify(bazingaTaskManagerStub).schedule(argThat(task ->
                (task instanceof LaunchStartTask) &&
                        prLaunch.getLaunchId().equals(
                                ((LaunchStartTask.Params) task.getParameters()).getLaunchId()
                        )
        ));

        db.currentOrReadOnly(() ->
                assertThat(db.launches().getLaunches(processId, -1, -1, false))
                        .hasSize(1)
                        .first()
                        .isEqualTo(
                                prLaunch.toBuilder()
                                        .flowInfo(prLaunch.getFlowInfo().toBuilder()
                                                .runtimeInfo(LaunchRuntimeInfo.ofRuntimeSandboxOwner(
                                                        TestData.YAV_TOKEN_UUID, "CI"))
                                                .build()
                                        )
                                        .status(LaunchState.Status.STARTING)
                                        .build()
                        )
        );
    }

    private static LaunchPullRequestInfo createLaunchPullRequestInfo() {
        return new LaunchPullRequestInfo(
                1L,
                100L,
                TestData.CI_USER,
                null,
                null,
                ArcanumMergeRequirementId.of("CI", "pr/new: Woodcutter"),
                createPullRequestVcsInfo(),
                List.of(),
                List.of(),
                null
        );
    }

    private static PullRequestVcsInfo createPullRequestVcsInfo() {
        return new PullRequestVcsInfo(
                ArcRevision.of("merge_rev"),
                TestData.TRUNK_COMMIT_2.getRevision(),
                ArcBranch.ofBranchName("trunk"),
                ArcRevision.of("feature_rev"),
                ArcBranch.ofBranchName("feature_branch")
        );
    }

    private Launch startRelease(CiProcessId processId, OrderedArcRevision revision, String triggeredBy) {
        return startRelease(processId, revision, triggeredBy, false);
    }

    private Launch startRelease(CiProcessId processId, OrderedArcRevision revision) {
        return startRelease(processId, revision, TestData.CI_USER, false);
    }

    private Launch startRelease(CiProcessId processId,
                                OrderedArcRevision revision,
                                String triggeredBy,
                                boolean cancelOthers) {
        return launchService.startRelease(
                processId,
                revision.toRevision(),
                revision.getBranch(),
                triggeredBy,
                null,
                cancelOthers,
                false,
                null,
                true,
                null,
                null,
                null);
    }

    /**
     * Метод актуален только если нужен запуск на транковой ревизии но в ветке. В остальных случаях ветка однозначно
     * определяется по ревизии и можно использовать {@link #startRelease(CiProcessId, OrderedArcRevision)}
     */
    private Launch startReleaseCustomBranch(CiProcessId processId, OrderedArcRevision revision, ArcBranch branch) {
        return launchService.startRelease(
                processId, revision.toRevision(), branch, TestData.CI_USER,
                null, false, false, null, true, null, null, null
        );
    }

    private Launch getLaunch(LaunchId id) {
        return db.currentOrReadOnly(() -> db.launches().get(id));
    }
}
