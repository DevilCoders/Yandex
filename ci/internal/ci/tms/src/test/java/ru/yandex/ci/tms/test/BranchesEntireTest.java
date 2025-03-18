package ru.yandex.ci.tms.test;

import java.util.List;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.Branch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.flow.YavDelegationException;
import ru.yandex.ci.engine.timeline.CommitFetchService;
import ru.yandex.ci.engine.timeline.TimelineCommit;

import static org.assertj.core.api.Assertions.assertThat;

public class BranchesEntireTest extends AbstractEntireTest {

    private static final CiProcessId PROCESS_ID = TestData.EMPTY_MANUAL_RELEASE_PROCESS_ID;

    @Autowired
    private CommitFetchService commitFetchService;

    /**
     * <pre>
     *     ▷ - активный релиз
     *     ▶ - успешно завершенный релиз
     *     o - просто коммит
     *     ║ - дерево коммитов
     *
     *                                 ▷ r6_3   ───┐
     *                                 ║           │
     *                                 ║           │
     *                                 ║           │
     *      ┌──── ▷ r8                 ▶ r6_2   ◄──┘  ──┐
     *      │     ║                    ║                │
     *      │     ║                    ║                │
     *      │     ║                    ║                │
     *      │     o r7                 o r6_1           │
     *      │     ║                    ║                │
     *      │     ║                    ║                │
     *      │     ║                    ║                │
     *      └───► o r6 ════════════════╝                │
     *            ║                                     │
     *            ║                                     │
     *            ║          ┌──────────────────────────┘
     *            o r5       │
     *            ║          │
     *            ║          │
     *            ║          │
     *      ┌───  ▶ r4  ◄────┘         ▷ r2_2    ───┐
     *      │     ║                    ║            │
     *      │     ║                    ║            │
     *      │     ║                    ║            │
     *      │     o r3                 ▷ r2_1       │
     *      │     ║                    ║            │
     *      │     ║                    ║            │
     *      │     ║                    ║            │
     *      ▼     o r2 ════════════════╝            ▼
     * </pre>
     */
    @Test
    void complexTest() throws YavDelegationException, InterruptedException {
        processCommits(
                TestData.TRUNK_COMMIT_2,
                TestData.TRUNK_COMMIT_3,
                TestData.TRUNK_COMMIT_4,
                TestData.TRUNK_COMMIT_5,
                TestData.TRUNK_COMMIT_6,
                TestData.TRUNK_COMMIT_7,
                TestData.TRUNK_COMMIT_8
        );

        engineTester.delegateToken(PROCESS_ID.getPath());
        Branch branchR2 = createBranchAt(TestData.TRUNK_R2);
        processCommits(branchR2.getArcBranch(), TestData.RELEASE_BRANCH_COMMIT_2_1,
        TestData.RELEASE_BRANCH_COMMIT_2_2);
        engineTester.delegateToken(PROCESS_ID.getPath(), branchR2.getArcBranch());

        Launch launchR4 = launch(PROCESS_ID, TestData.TRUNK_R4);
        disableManualSwitch(launchR4, "end");
        engineTester.waitLaunch(launchR4.getLaunchId(), WAIT, LaunchState.Status.SUCCESS);

        Branch branchR6 = createBranchAt(TestData.TRUNK_R6);
        processCommits(
                branchR6.getArcBranch(),
                TestData.RELEASE_BRANCH_COMMIT_6_1,
                TestData.RELEASE_BRANCH_COMMIT_6_2,
                TestData.RELEASE_BRANCH_COMMIT_6_3
        );
        engineTester.delegateToken(PROCESS_ID.getPath(), branchR6.getArcBranch());

        Launch launchR6n2 = launchRelease(PROCESS_ID, TestData.RELEASE_R6_2, branchR6.getArcBranch());
        disableManualSwitch(launchR6n2, "end");
        engineTester.waitLaunch(launchR6n2.getLaunchId(), WAIT, LaunchState.Status.SUCCESS);

        Launch launchR8 = launch(PROCESS_ID, TestData.TRUNK_R8);
        Launch launchR6n3 = launchRelease(PROCESS_ID, TestData.RELEASE_R6_3, branchR6.getArcBranch());
        Launch launchR2n1 = launchRelease(PROCESS_ID, TestData.RELEASE_R2_1, branchR2.getArcBranch());
        Launch launchR2n2 = launchRelease(PROCESS_ID, TestData.RELEASE_R2_2, branchR2.getArcBranch());

        assertThat(getCommitsToStable(launchR4))
                .extracting(this::revision)
                // R2 включено, потому что ветка не в стейбле
                .containsExactly(
                        TestData.TRUNK_R4,
                        TestData.TRUNK_R3,
                        TestData.TRUNK_R2
                );

        assertThat(getCommitsToStable(launchR2n1))
                .extracting(this::revision)
                .containsExactly(
                        TestData.projectRevToBranch(TestData.RELEASE_R2_1, branchR2.getArcBranch()),
                        TestData.TRUNK_R2
                );

        assertThat(getCommitsToStable(launchR2n2))
                .extracting(this::revision)
                .containsExactly(
                        TestData.projectRevToBranch(TestData.RELEASE_R2_2, branchR2.getArcBranch()),
                        TestData.projectRevToBranch(TestData.RELEASE_R2_1, branchR2.getArcBranch()),
                        TestData.TRUNK_R2
                );

        assertThat(getCommitsToStable(launchR6n2))
                .extracting(this::revision)
                .containsExactly(
                        TestData.projectRevToBranch(TestData.RELEASE_R6_2, branchR6.getArcBranch()),
                        TestData.projectRevToBranch(TestData.RELEASE_R6_1, branchR6.getArcBranch()),
                        TestData.TRUNK_R6,
                        TestData.TRUNK_R5
                );

        assertThat(getCommitsToStable(launchR6n3))
                .extracting(this::revision)
                .containsExactly(
                        TestData.projectRevToBranch(TestData.RELEASE_R6_3, branchR6.getArcBranch())
                );

        assertThat(getCommitsToStable(launchR8))
                .extracting(this::revision)
                .containsExactly(
                        TestData.TRUNK_R8,
                        TestData.TRUNK_R7
                        // R6 уехало вместе с релизом в ветке
                );
    }

    /**
     * <pre>
     *
     *    o r8                 ▶ r6_2     ──┐
     *    ║                    ║            │
     *    ║                    ║            │
     *    ║                    ║            │
     *    o r7                 o r6_1       │
     *    ║                    ║            │
     *    ║                    ║            │
     *    ║                    ║            │
     *    ▶ r6 ════════════════╝  ◄─────────┘
     *    ║
     *    o r5
     *    ║
     *    o r4
     *    ║
     *    o r3
     *    ║
     *    o r2
     * </pre>
     */
    @Test
    void branchFromStableRelease() throws InterruptedException, YavDelegationException {
        processCommits(
                TestData.TRUNK_COMMIT_2,
                TestData.TRUNK_COMMIT_3,
                TestData.TRUNK_COMMIT_4,
                TestData.TRUNK_COMMIT_5,
                TestData.TRUNK_COMMIT_6,
                TestData.TRUNK_COMMIT_7,
                TestData.TRUNK_COMMIT_8
        );

        engineTester.delegateToken(PROCESS_ID.getPath());

        Launch launchR6 = launch(PROCESS_ID, TestData.TRUNK_R6);
        disableManualSwitch(launchR6, "end");
        engineTester.waitLaunch(launchR6.getLaunchId(), WAIT, LaunchState.Status.SUCCESS);

        Branch branchR6 = createBranchAt(TestData.TRUNK_R6);
        processCommits(
                branchR6.getArcBranch(),
                TestData.RELEASE_BRANCH_COMMIT_6_1,
                TestData.RELEASE_BRANCH_COMMIT_6_2
        );
        engineTester.delegateToken(PROCESS_ID.getPath(), branchR6.getArcBranch());

        Launch launchR6n2 = launch(PROCESS_ID, TestData.RELEASE_R6_2);
        disableManualSwitch(launchR6n2, "end");
        engineTester.waitLaunch(launchR6n2.getLaunchId(), WAIT, LaunchState.Status.SUCCESS);


        assertThat(getCommitsToStable(launchR6n2))
                .extracting(this::revision)
                .containsExactly(
                        TestData.RELEASE_R6_2,
                        TestData.RELEASE_R6_1
                );
    }

    private Branch createBranchAt(OrderedArcRevision trunkR6) {
        return db.currentOrTx(() ->
                branchService.createBranch(PROCESS_ID, trunkR6, TestData.CI_USER));
    }

    private OrderedArcRevision revision(TimelineCommit tc) {
        return tc.getCommit().getRevision();
    }

    protected Launch launchRelease(CiProcessId processId, OrderedArcRevision revision, ArcBranch branch) {
        return super.launch(processId, TestData.projectRevToBranch(revision, branch));
    }

    private List<TimelineCommit> getCommitsToStable(Launch launch) {
        return db.tx(() ->
                commitFetchService.fetchTimelineCommits(launch.getLaunchId(),
                        CommitFetchService.CommitOffset.empty(), -1, true
                )).items();
    }
}
