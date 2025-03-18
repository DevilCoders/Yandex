package ru.yandex.ci.engine.timeline;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcServiceStub;
import ru.yandex.ci.core.arc.CommitId;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.Branch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.engine.branch.BranchService;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.doReturn;

public class CommitRangeServiceCanLaunchAtTest extends EngineTestBase {

    private static final CiProcessId PROCESS_ID = TestData.WITH_BRANCHES_RELEASE_PROCESS_ID;

    @Autowired
    private BranchService branchService;

    @Autowired
    private CommitRangeService commitRangeService;

    @BeforeEach
    void setUp() {
        mockValidationSuccessful();
        doReturn(TestData.RELEASE_BRANCH_2.asString())
                .when(branchNameGenerator).generateName(any(), any(), anyInt());
        discoveryToR3();
        delegateToken(PROCESS_ID.getPath());
    }

    @AfterEach
    void tearDown() {
        ((ArcServiceStub) arcService).resetAndInitTestData();
    }

    @Test
    void atTrunk() {
        assertThat(checkCanLaunchAt(TestData.TRUNK_R2, ArcBranch.trunk())).isTrue();
        startRelease(TestData.TRUNK_R2, ArcBranch.trunk());
        assertThat(checkCanLaunchAt(TestData.TRUNK_R3, ArcBranch.trunk())).isTrue();
    }

    @Test
    void atBranch() {
        ArcBranch branch = createBranchAt(TestData.TRUNK_R2).getArcBranch();
        assertThat(checkCanLaunchAt(TestData.TRUNK_R2, branch)).isTrue();
        startRelease(TestData.TRUNK_R2, branch);
        assertThat(checkCanLaunchAt(TestData.TRUNK_R3, branch)).isTrue();
    }

    @Test
    void atTheSameRevisionInTrunk() {
        assertThat(checkCanLaunchAt(TestData.TRUNK_R2, ArcBranch.trunk())).isTrue();
        Launch launch = startRelease(TestData.TRUNK_R2, ArcBranch.trunk());

        // forbidden when launch is not in terminal status
        assertThat(checkCanLaunchAt(TestData.TRUNK_R2, ArcBranch.trunk())).isFalse();

        db.currentOrTx(() ->
                db.launches().save(
                        db.launches().get(launch.getLaunchId())
                                .toBuilder()
                                .status(LaunchState.Status.SUCCESS)
                                .build()
                )
        );
        assertThat(checkCanLaunchAt(TestData.TRUNK_R2, ArcBranch.trunk())).isTrue();
    }

    @Test
    void atTheSameRevisionInTrunk_whenParameter_allowLaunchWithoutCommits_IsFalse() {
        assertThat(checkCanLaunchAt(TestData.TRUNK_R2, ArcBranch.trunk())).isTrue();
        Launch launch = startRelease(TestData.TRUNK_R2, ArcBranch.trunk());

        // forbidden when launch is not in terminal status
        assertThat(checkCanLaunchAt(TestData.TRUNK_R2, ArcBranch.trunk(), false, false)).isFalse();

        db.currentOrTx(() ->
                db.launches().save(
                        db.launches().get(launch.getLaunchId())
                                .toBuilder()
                                .status(LaunchState.Status.SUCCESS)
                                .build()
                )
        );
        // forbidden cause parameter allowLaunchWithoutCommits is false
        assertThat(checkCanLaunchAt(TestData.TRUNK_R2, ArcBranch.trunk(), false, false)).isFalse();
    }

    @Test
    void atTheSameRevisionInBranch() {
        ArcBranch branch = createBranchAt(TestData.TRUNK_R2).getArcBranch();
        assertThat(checkCanLaunchAt(TestData.TRUNK_R2, branch)).isTrue();
        Launch launch = startRelease(TestData.TRUNK_R2, branch);

        // forbidden when launch is not in terminal status
        assertThat(checkCanLaunchAt(TestData.TRUNK_R2, branch)).isFalse();

        db.currentOrTx(() ->
                db.launches().save(
                        db.launches().get(launch.getLaunchId())
                                .toBuilder()
                                .status(LaunchState.Status.SUCCESS)
                                .build()
                )
        );
        assertThat(checkCanLaunchAt(TestData.TRUNK_R2, branch)).isTrue();
    }

    @Test
    void atTheSameRevisionInBranch_whenParameter_allowLaunchWithoutCommits_IsFalse() {
        ArcBranch branch = createBranchAt(TestData.TRUNK_R2).getArcBranch();
        assertThat(checkCanLaunchAt(TestData.TRUNK_R2, branch)).isTrue();
        Launch launch = startRelease(TestData.TRUNK_R2, branch);

        // forbidden when launch is not in terminal status
        assertThat(checkCanLaunchAt(TestData.TRUNK_R2, branch, false, false)).isFalse();

        db.currentOrTx(() ->
                db.launches().save(
                        db.launches().get(launch.getLaunchId())
                                .toBuilder()
                                .status(LaunchState.Status.SUCCESS)
                                .build()
                )
        );
        // forbidden cause parameter allowLaunchWithoutCommits is false
        assertThat(checkCanLaunchAt(TestData.TRUNK_R2, branch, false, false)).isFalse();
    }

    @Test
    void startReleaseInTrunkAndThenCreateBranch() {
        Launch launch = startRelease(TestData.TRUNK_R2, ArcBranch.trunk());
        ArcBranch branch = createBranchAt(TestData.TRUNK_R2).getArcBranch();

        assertThat(checkCanLaunchAt(TestData.TRUNK_R3, ArcBranch.trunk())).isTrue();
        assertThat(checkCanLaunchAt(TestData.TRUNK_R3, branch)).isTrue();

        assertThat(checkCanLaunchAt(TestData.TRUNK_R2, ArcBranch.trunk())).isFalse();
        assertThat(checkCanLaunchAt(TestData.TRUNK_R2, branch)).isFalse();

        db.currentOrTx(() ->
                db.launches().save(
                        db.launches().get(launch.getLaunchId())
                                .toBuilder()
                                .status(LaunchState.Status.SUCCESS)
                                .build()
                )
        );
        // starting release at revision R1 in trunk is forbidden cause the branch was created at this revision
        assertThat(checkCanLaunchAt(TestData.TRUNK_R2, ArcBranch.trunk())).isFalse();
        assertThat(checkCanLaunchAt(TestData.TRUNK_R2, branch)).isTrue();
    }

    @Test
    void startFlow_shouldReturnTrue() {
        CiProcessId processId = CiProcessId.ofFlow(TestData.CONFIG_PATH_ABC, "flow-id");
        assertThat(
                commitRangeService.canStartLaunchAt(
                        processId, OrderedArcRevision.fromHash("some-commit", ArcBranch.trunk(), 1, 0),
                        ArcBranch.trunk()
                ).isAllowed()
        ).isTrue();
    }

    @Test
    void startReleaseEarlierThanLatest() {
        discoveryToR5();
        delegateToken(PROCESS_ID.getPath());

        assertThat(checkCanLaunchAt(TestData.TRUNK_R5, ArcBranch.trunk())).isTrue();

        startRelease(TestData.TRUNK_R5, ArcBranch.trunk());

        assertThat(checkCanLaunchAt(TestData.TRUNK_R2, ArcBranch.trunk())).isFalse();
        assertThat(checkCanLaunchAt(TestData.TRUNK_R2, ArcBranch.trunk(), true, true)).isTrue();
    }

    private boolean checkCanLaunchAt(OrderedArcRevision revision, ArcBranch branch) {
        return db.currentOrReadOnly(() -> commitRangeService.canStartLaunchAt(PROCESS_ID, revision, branch)
                .isAllowed());
    }

    private boolean checkCanLaunchAt(OrderedArcRevision revision, ArcBranch branch,
                                     boolean allowLaunchWithoutCommits, boolean allowLaunchEarlier) {
        return db.currentOrReadOnly(() ->
                commitRangeService.canStartLaunchAt(PROCESS_ID, revision, branch,
                        allowLaunchWithoutCommits, allowLaunchEarlier)
                        .isAllowed()
        );
    }

    private Launch startRelease(CommitId revision, ArcBranch branch) {
        return launchService.startRelease(PROCESS_ID, revision, branch, TestData.CI_USER, null, false,
                false, null, true, null, null, null);
    }

    private Branch createBranchAt(OrderedArcRevision revision) {
        return db.currentOrTx(() -> branchService.createBranch(PROCESS_ID, revision, TestData.CI_USER));
    }
}
