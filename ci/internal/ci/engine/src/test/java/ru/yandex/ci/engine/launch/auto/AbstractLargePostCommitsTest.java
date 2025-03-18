package ru.yandex.ci.engine.launch.auto;

import java.time.Instant;
import java.util.Map;
import java.util.Optional;

import javax.annotation.Nullable;

import com.google.gson.JsonObject;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.VirtualCiProcessId;
import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.launch.LaunchState.Status;
import ru.yandex.ci.core.launch.PostponeLaunch;
import ru.yandex.ci.core.launch.PostponeLaunch.PostponeStatus;
import ru.yandex.ci.core.proto.ProtobufSerialization;
import ru.yandex.ci.core.security.YavToken;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.engine.launch.LaunchService;
import ru.yandex.ci.engine.launch.OnCommitLaunchService;
import ru.yandex.ci.storage.api.StorageApi;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.Common;

import static org.assertj.core.api.Assertions.assertThat;

public abstract class AbstractLargePostCommitsTest extends EngineTestBase {


    /* normal (1h):
     * COMMIT1 +
     * COMMIT2 -
     * COMMIT3 -
     * COMMIT4 +
     * COMMIT5 -
     * COMMIT6 +
     * COMMIT7 +
     * COMMIT8 -
     * COMMIT9 -
     * COMMIT10 +
     * COMMIT11 +
     */

    /* slow (2h):
     * COMMIT1 +
     * COMMIT2 -
     * COMMIT3 -
     * COMMIT4 -
     * COMMIT5 +
     * COMMIT6 +
     * COMMIT7 +
     */

    public static final ArcCommit COMMIT1 = commit("r1", TestData.TRUNK_COMMIT_1, "04:05");
    public static final ArcCommit COMMIT2 = commit("r2", COMMIT1, "04:15");
    public static final ArcCommit COMMIT3 = commit("r3", COMMIT2, "05:04");
    public static final ArcCommit COMMIT4 = commit("r4", COMMIT3, "05:59");
    public static final ArcCommit COMMIT5 = commit("r5", COMMIT4, "06:33");
    public static final ArcCommit COMMIT6 = commit("r6", COMMIT5, "09:55");
    public static final ArcCommit COMMIT7 = commit("r7", COMMIT6, "11:04");
    public static final ArcCommit COMMIT8 = commit("r8", COMMIT7, "11:45");
    public static final ArcCommit COMMIT9 = commit("r9", COMMIT8, "11:55");
    public static final ArcCommit COMMIT10 = commit("r10", COMMIT9, "15:15");
    public static final ArcCommit COMMIT11 = commit("r11", COMMIT10, "23:45");
    public static final ArcCommit COMMIT12 = commit("r12", COMMIT11, "02", "11:38"); // 24+ from COMMIT7

    private static final String DELEGATED_OWNER = "DELEGATED-CI-OWNER";
    private static final YavToken.Id DELEGATED_TOKEN = YavToken.Id.of("delegated-token");

    @Autowired
    protected BinarySearchExecutor binarySearchExecutor;

    @Autowired
    protected DiscoveryProgressChecker discoveryProgressChecker;

    protected CiProcessId ciProcessId;

    @BeforeEach
    void beforeEach() {
        ciProcessId = getCiProcessId();

        mockValidationSuccessful();
        mockYavAny();

        arcServiceStub.reset();
        arcServiceStub.addFirstCommit();
        arcServiceStub.addCommitAuto(COMMIT1);
    }

    @AfterEach
    void afterEach() {
        arcServiceStub.resetAndInitTestData();
    }

    //

    protected abstract CiProcessId getCiProcessId();

    protected abstract DiscoveryType getDiscoveryType();

    protected abstract void checkPostponeStatus(PostponeLaunch postponeLaunch);

    protected void configure(ArcCommit discoveryCommit) {
        discoveryConfig();

        db.currentOrTx(() -> {
            addEmptyCommit(COMMIT2);
            addEmptyCommit(COMMIT3);
            addEmptyCommit(COMMIT4);
            addEmptyCommit(COMMIT5);
            addEmptyCommit(COMMIT6);
            addEmptyCommit(COMMIT7);
            addEmptyCommit(COMMIT8);
            addEmptyCommit(COMMIT9);
            addEmptyCommit(COMMIT10);
            addEmptyCommit(COMMIT11);
            addEmptyCommit(COMMIT12);
        });
        progressDiscovered(discoveryCommit);

        assertThat(findPostpone(COMMIT1)).isEmpty();
    }

    //


    protected void discoveryConfig() {
        discovery(COMMIT1);
        delegateToken(VirtualCiProcessId.of(ciProcessId).getResolvedPath());
    }

    protected Launch createLaunch(ArcCommit commit) {
        return createLaunch(commit, buildFlowVars(commit));
    }

    protected Launch createLaunch(ArcCommit commit, @Nullable JsonObject flowVars) {
        var delegatedSecurity = new LaunchService.DelegatedSecurity("ci-service", DELEGATED_TOKEN, DELEGATED_OWNER);
        var launch = onCommitLaunchService.startFlow(
                OnCommitLaunchService.StartFlowParameters.builder()
                        .processId(ciProcessId)
                        .branch(ArcBranch.trunk())
                        .revision(ArcRevision.of(commit.getCommitId()))
                        .triggeredBy(TestData.CI_USER)
                        .launchMode(LaunchService.LaunchMode.POSTPONE)
                        .delegatedSecurity(delegatedSecurity)
                        .flowVars(flowVars)
                        .build()
        );
        check(commit, getPostpone(commit), launch, Status.POSTPONE, PostponeStatus.NEW, null, null);
        return launch;
    }

    protected StorageApi.LargeTaskId largeTaskId(ArcCommit commit) {
        return StorageApi.LargeTaskId.newBuilder()
                .setIterationId(CheckIteration.IterationId.newBuilder()
                        .setCheckId(String.valueOf(10000000001000L + commit.getSvnRevision()))
                        .setCheckType(CheckIteration.IterationType.HEAVY)
                        .setNumber(1)
                        .build())
                .setCheckTaskType(Common.CheckTaskType.CTT_LARGE_TEST)
                .setIndex(1)
                .build();
    }

    protected JsonObject buildFlowVars(ArcCommit commit) {
        var result = new JsonObject();
        result.add("request", ProtobufSerialization.serializeToGson(largeTaskId(commit)));
        return result;
    }

    protected void completeLaunch(LaunchId launchId) {
        db.currentOrTx(() -> {
            var launch = db.launches().get(launchId);
            db.launches().save(launch.withStatus(Status.SUCCESS));
        });
    }

    protected void failureLaunch(LaunchId launchId) {
        db.currentOrTx(() -> {
            var launch = db.launches().get(launchId);
            db.launches().save(launch.withStatus(Status.FAILURE));
        });
    }

    protected void cancelLaunch(LaunchId launchId) {
        db.currentOrTx(() -> {
            var launch = db.launches().get(launchId);
            db.launches().save(launch.withStatus(Status.CANCELED));
        });
    }

    protected void addEmptyCommit(ArcCommit commit) {
        arcServiceStub.addCommit(commit, Map.of());
        saveCommit(commit);
    }


    protected Optional<PostponeLaunch> findPostpone(ArcCommit commit) {
        return db.currentOrReadOnly(() -> db.postponeLaunches().find(postponeId(commit)));
    }

    protected PostponeLaunch getPostpone(ArcCommit commit) {
        return db.currentOrReadOnly(() -> db.postponeLaunches().get(postponeId(commit)));
    }

    protected void check(
            ArcCommit commit,
            LaunchId launchId,
            LaunchState.Status launchStatus,
            PostponeStatus postponeStatus,
            @Nullable PostponeLaunch.StartReason startReason
    ) {
        check(commit, launchId, launchStatus, postponeStatus, startReason, null);
    }

    protected void check(
            ArcCommit commit,
            LaunchId launchId,
            LaunchState.Status launchStatus,
            PostponeStatus postponeStatus,
            @Nullable PostponeLaunch.StartReason startReason,
            @Nullable PostponeLaunch.StopReason stopReason
    ) {
        var postpone = getPostpone(commit);
        var launch = db.currentOrReadOnly(() -> db.launches().get(launchId));

        check(commit, postpone, launch, launchStatus, postponeStatus, startReason, stopReason);
    }

    protected void check(
            ArcCommit commit,
            PostponeLaunch postponeLaunch,
            Launch launch,
            LaunchState.Status launchStatus,
            PostponeStatus postponeStatus,
            @Nullable PostponeLaunch.StartReason startReason,
            @Nullable PostponeLaunch.StopReason stopReason
    ) {

        var launchCommit = launch.getVcsInfo().getCommit();
        assertThat(launchCommit)
                .isNotNull()
                .isEqualTo(commit);

        var id = postponeId(commit);
        assertThat(id.getSvnRevision())
                .isGreaterThan(0)
                .isEqualTo(launchCommit.getSvnRevision());

        assertThat(postponeLaunch)
                .isEqualTo(PostponeLaunch.builder()
                        .id(postponeId(commit))
                        .commitTime(launchCommit.getCreateTime())
                        .launchNumber(launch.getId().getLaunchNumber())
                        .virtualType(VirtualCiProcessId.VirtualType.of(getCiProcessId()))
                        .status(postponeStatus)
                        .startReason(startReason)
                        .stopReason(stopReason)
                        .build()
                );

        assertThat(launch.getStatus())
                .isEqualTo(launchStatus);

        var runtimeInfo = launch.getFlowInfo().getRuntimeInfo();
        assertThat(runtimeInfo.getSandboxOwner())
                .isEqualTo(DELEGATED_OWNER);
        assertThat(runtimeInfo.getYavTokenUid())
                .isEqualTo(DELEGATED_TOKEN);

        checkPostponeStatus(postponeLaunch);
    }

    protected void progressDiscovered(ArcCommit commit) {
        var revision = commit.toOrderedTrunkArcRevision();
        db.currentOrTx(() -> discoveryProgressChecker.updateLastProcessedCommitInTx(revision, getDiscoveryType()));
    }

    protected PostponeLaunch.Id postponeId(ArcCommit arcCommit) {
        return PostponeLaunch.Id.of(ciProcessId, arcCommit.getSvnRevision());
    }

    protected static ArcCommit commit(String path, ArcCommit parent, String time) {
        return commit(path, parent, "01", time);
    }

    protected static ArcCommit commit(String path, ArcCommit parent, String date, String time) {
        var revision = OrderedArcRevision.fromHash(
                "LargePostCommitServiceTest/" + path,
                ArcBranch.trunk(),
                parent.getSvnRevision() + 1,
                parent.getPullRequestId() + 1
        );
        var commitTime = Instant.parse("2022-01-%sT%s:00.000Z".formatted(date, time));
        return TestData.toCommit(revision, TestData.CI_USER)
                .withParent(parent)
                .toBuilder()
                .createTime(commitTime)
                .build();
    }
}
