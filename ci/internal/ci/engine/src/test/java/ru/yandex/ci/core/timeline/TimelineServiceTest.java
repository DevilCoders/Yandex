package ru.yandex.ci.core.timeline;

import java.util.List;
import java.util.Set;
import java.util.function.Supplier;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

import org.assertj.core.data.Index;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Nested;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.Branch;
import ru.yandex.ci.core.arc.branch.BranchInfo;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchRuntimeInfo;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.launch.LaunchVcsInfo;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.engine.timeline.TimelineService;

import static org.assertj.core.api.Assertions.assertThat;
import static ru.yandex.ci.core.timeline.TimelineItemAssert.assertItem;

class TimelineServiceTest extends EngineTestBase {
    private static final CiProcessId PROCESS_ID = TestData.SIMPLE_RELEASE_PROCESS_ID;

    @Autowired
    private TimelineService timelineService;

    private int launchNumber;

    @BeforeEach
    void setUp() {
        launchNumber = 1;
    }

    @Test
    void addLaunches() {
        Launch l1 = launchAtRevision(TestData.TRUNK_R2, ++launchNumber);
        Launch l2 = launchAtRevision(TestData.TRUNK_R3, ++launchNumber);
        Launch l3 = launchAtRevision(TestData.TRUNK_R4, ++launchNumber);
        Launch l4 = launchAtRevision(TestData.TRUNK_R5, ++launchNumber);

        tx(() -> {
            timelineService.updateTimelineLaunchItem(l1);
            timelineService.updateTimelineLaunchItem(l2);
            timelineService.updateTimelineLaunchItem(l3);
            timelineService.updateTimelineLaunchItem(l4);
        });

        assertThat(roTx(() -> db.timeline().findAll()))
                .containsExactlyInAnyOrder(
                        entity(TestData.TRUNK_R2, 1, 1).launch(l1.getId()).build(),
                        entity(TestData.TRUNK_R3, 2, 2).launch(l2.getId()).build(),
                        entity(TestData.TRUNK_R4, 3, 3).launch(l3.getId()).build(),
                        entity(TestData.TRUNK_R5, 4, 4).launch(l4.getId()).build()
                );
    }

    @Test
    void addMultipleLaunchesOnRevision() {
        Launch launch71 = launchAtRevision(TestData.TRUNK_R8, 71);
        Launch launch94 = launchAtRevision(TestData.TRUNK_R8, 94);
        tx(() -> {
            timelineService.updateTimelineLaunchItem(launch71);
            timelineService.updateTimelineLaunchItem(launch94);
        });

        assertThat(roTx(() -> db.timeline().findAll()))
                .containsExactlyInAnyOrder(
                        entity(TestData.TRUNK_R8, 1, 1).launch(launch71.getId()).build(),
                        entity(TestData.TRUNK_R8, 2, 2).launch(launch94.getId()).build()
                );
    }

    @Test
    void addBranch() {
        tx(() -> {
            timelineService.addBranch(branchAtRevision(PROCESS_ID, TestData.TRUNK_R6, "green"));
            timelineService.addBranch(branchAtRevision(PROCESS_ID, TestData.TRUNK_R8, "white"));
            timelineService.addBranch(branchAtRevision(PROCESS_ID, TestData.TRUNK_R4, "black"));
        });

        assertThat(getTimelineInTrunk())
                .containsExactly(
                        entity(TestData.TRUNK_R8, 2, 2).branchId(PROCESS_ID, releaseBr("white")).build(),
                        entity(TestData.TRUNK_R6, 1, 1).branchId(PROCESS_ID, releaseBr("green")).build(),
                        entity(TestData.TRUNK_R4, 3, 3).branchId(PROCESS_ID, releaseBr("black")).build()
                );
    }

    @Test
    void addMultipleBranchesOnRevision() {
        tx(() -> timelineService.addBranch(branchAtRevision(PROCESS_ID, TestData.TRUNK_R6, "green")));
        tx(() -> timelineService.addBranch(branchAtRevision(PROCESS_ID, TestData.TRUNK_R6, "white")));

        assertThat(getTimelineInTrunk())
                .containsExactly(
                        entity(TestData.TRUNK_R6, 2, 2).branchId(PROCESS_ID, releaseBr("white")).build(),
                        entity(TestData.TRUNK_R6, 2, 1).branchId(PROCESS_ID, releaseBr("green")).build()
                );
    }

    @Test
    void addBranchAfterRelease() {
        Launch launch = launchAtRevision(TestData.TRUNK_R8, 71);
        tx(() -> timelineService.updateTimelineLaunchItem(launch));

        assertThat(getTimelineInTrunk())
                .containsExactly(
                        entity(TestData.TRUNK_R8, 1, 1).launch(launch.getId()).build()
                );

        var branch = branchAtRevision(PROCESS_ID, TestData.TRUNK_R8, "white");
        var branchName = branch.getArcBranch().asString();
        tx(() -> timelineService.addBranch(branch));

        assertThat(roTx(() -> db.timeline().findAll()))
                .containsExactlyInAnyOrder(
                        entity(TestData.TRUNK_R8, 2, 1).launch(launch.getId()).showInBranch(branchName).build(),
                        entity(TestData.TRUNK_R8, 2, 2).branchId(PROCESS_ID, branchName).build()
                );

        assertThat(getTimelineInTrunk())
                .containsExactly(
                        entity(TestData.TRUNK_R8, 2, 2).branchId(PROCESS_ID, branchName).build()
                );

        assertThat(getTimelineIn(branch.getArcBranch()))
                .containsExactly(
                        entity(TestData.TRUNK_R8, 2, 1).launch(launch.getId()).showInBranch(branchName).build()
                );
    }

    @Test
    void addReleaseAfterBranch() {
        var branch = branchAtRevision(PROCESS_ID, TestData.TRUNK_R8, "white");
        var branchName = branch.getArcBranch().asString();
        tx(() -> timelineService.addBranch(branch));
        assertThat(getTimelineInTrunk())
                .containsExactly(
                        entity(TestData.TRUNK_R8, 1, 1).branchId(PROCESS_ID, branchName).build()
                );

        Launch launch = launchAtRevision(TestData.TRUNK_R8, 71);
        tx(() -> timelineService.updateTimelineLaunchItem(launch));

        assertThat(roTx(() -> db.timeline().findAll()))
                .containsExactlyInAnyOrder(
                        entity(TestData.TRUNK_R8, 2, 2).launch(launch.getId()).showInBranch(branchName).build(),
                        entity(TestData.TRUNK_R8, 1, 1).branchId(PROCESS_ID, branchName).build()
                );

        assertThat(getTimelineInTrunk())
                .containsExactly(
                        entity(TestData.TRUNK_R8, 1, 1).branchId(PROCESS_ID, branchName).build()
                );

        assertThat(getTimelineIn(branch.getArcBranch()))
                .containsExactly(
                        entity(TestData.TRUNK_R8, 2, 2).launch(launch.getId()).showInBranch(branchName).build()
                );
    }

    @Test
    void dontShowItemsFromAnotherBranch() {
        var whiteBranch = branchAtRevision(PROCESS_ID, TestData.TRUNK_R2, TestData.RELEASE_BRANCH_1);
        var blueBranch = branchAtRevision(PROCESS_ID, TestData.TRUNK_R6, TestData.RELEASE_BRANCH_2);

        tx(() -> timelineService.addBranch(whiteBranch));
        tx(() -> timelineService.addBranch(blueBranch));

        Launch atBlue = launchAtRevision(TestData.RELEASE_R6_1, 5);
        Launch atWhite = launchAtRevision(TestData.RELEASE_R2_2, 6);
        tx(() -> timelineService.updateTimelineLaunchItem(atBlue));
        tx(() -> timelineService.updateTimelineLaunchItem(atWhite));

        assertThat(getTimelineIn(blueBranch.getArcBranch()))
                .containsExactly(
                        entity(TestData.RELEASE_R6_1, 3, 1).launch(atBlue.getId())
                                .showInBranch(blueBranch.getArcBranch().asString()).build()
                );

        assertThat(getTimelineIn(whiteBranch.getArcBranch()))
                .containsExactly(
                        entity(TestData.RELEASE_R2_2, 4, 1).launch(atWhite.getId())
                                .showInBranch(whiteBranch.getArcBranch().asString()).build()
                );
    }


    @Test
    void releaseCancelled() {
        Launch launch = launchAtRevision(TestData.TRUNK_R8, 71);
        tx(() -> timelineService.updateTimelineLaunchItem(launch));

        assertThat(getTimelineInTrunk())
                .containsExactly(
                        entity(TestData.TRUNK_R8, 1, 1).launch(launch.getId()).build()
                );

        tx(() -> timelineService.updateTimelineLaunchItem(cancelled(launch)));

        assertThat(roTx(() -> db.timeline().findAll()))
                .containsExactlyInAnyOrder(
                        entity(TestData.TRUNK_R8, 2, 1)
                                .launch(launch.getId())
                                .hidden(true)
                                .status(LaunchState.Status.CANCELED)
                                .build()
                );

        assertThat(getTimelineInTrunk()).isEmpty();
    }

    @Test
    void releaseCancelledAndCreatedBranchThen() {
        Launch launch = launchAtRevision(TestData.TRUNK_R8, 71);
        Branch branch = branchAtRevision(PROCESS_ID, TestData.TRUNK_R8, "white");
        String branchName = branch.getArcBranch().asString();
        tx(() -> timelineService.updateTimelineLaunchItem(launch));
        tx(() -> timelineService.updateTimelineLaunchItem(cancelled(launch)));
        tx(() -> timelineService.addBranch(branch));

        assertThat(roTx(() -> db.timeline().findAll()))
                .containsExactly(
                        entity(TestData.TRUNK_R8, 3, 1)
                                .launch(launch.getId())
                                .hidden(true)
                                .showInBranch(branchName)
                                .status(LaunchState.Status.CANCELED)
                                .build(),
                        entity(TestData.TRUNK_R8, 3, 2).branch(branch.getId()).build()
                );

        assertThat(getTimelineInTrunk())
                .containsExactly(
                        entity(TestData.TRUNK_R8, 3, 2).branch(branch.getId()).build()
                );

        assertThat(getTimelineIn(branch.getArcBranch())).isEmpty();
    }

    @Test
    void getNextAfter() {
        var launch2 = tx(() -> {
            createLaunch(TestData.trunkRevision(1), 1);
            Launch launch = createLaunch(TestData.trunkRevision(2), 2);
            createLaunch(TestData.trunkRevision(5), 9);
            createLaunch(TestData.trunkRevision(7), 11);
            return launch;
        });

        var after2 = roTx(() -> timelineService.getNextAfter(launch2));
        assertThat(after2).isPresent();
        assertThat(after2.get().getLaunch().getId().getLaunchNumber()).isEqualTo(9);
    }

    @Test
    void getNextAfterSameRevision() {
        var launch2 = tx(() -> {
            var revision = TestData.trunkRevision(2);
            createLaunch(revision, 1);
            Launch launch = createLaunch(revision, 2);
            createLaunch(revision, 19);
            createLaunch(revision, 23);
            return launch;
        });

        var after2 = roTx(() -> timelineService.getNextAfter(launch2));
        assertThat(after2).isPresent();
        assertThat(after2.get().getLaunch().getId().getLaunchNumber()).isEqualTo(19);
    }

    @Test
    void getNextBranchInSameRevision() {
        var launch2 = tx(() -> {
            var revision = TestData.trunkRevision(2);
            createLaunch(revision, 77);
            createBranch(revision);
            Launch launch = createLaunch(revision, 2);
            createLaunch(revision, 19);
            createLaunch(revision, 23);
            return launch;
        });

        var after2 = roTx(() -> timelineService.getNextAfter(launch2));
        assertThat(after2).isPresent();
        assertThat(after2.get().getType()).isEqualTo(TimelineItem.Type.BRANCH);
    }

    private Launch createLaunch(OrderedArcRevision revision, int number) {
        var launch = launchAtRevision(revision, number);
        db.launches().save(launch);
        timelineService.updateTimelineLaunchItem(launch);
        return launch;
    }

    private Branch createBranch(OrderedArcRevision revision) {
        var branch = branchAtRevision(PROCESS_ID, revision, TestData.RELEASE_BRANCH_2);
        db.branches().save(branch.getInfo());
        db.timelineBranchItems().save(branch.getItem());
        timelineService.addBranch(branch);
        return branch;
    }

    @Test
    void limitDoesntCountHidden() {
        List<Launch> launches = tx(() -> IntStream.range(1, 11)
                .mapToObj(rev -> launchAtRevision(TestData.trunkRevision(rev), ++launchNumber))
                .peek(timelineService::updateTimelineLaunchItem)
                .collect(Collectors.toList()));

        assertThat(roTx(() -> db.timeline().getTimeline(PROCESS_ID, ArcBranch.trunk(), Offset.EMPTY, 5)))
                .extracting(TimelineItemEntity::getLaunch)
                .extracting(Launch.Id::getLaunchNumber)
                .containsExactly(11, 10, 9, 8, 7);

        tx(() -> launches.stream()
                .filter(l -> Set.of(10, 7).contains(l.getLaunchId().getNumber()))
                .map(TimelineServiceTest::cancelled)
                .forEach(timelineService::updateTimelineLaunchItem)
        );

        assertThat(roTx(() -> db.timeline().getTimeline(PROCESS_ID, ArcBranch.trunk(), Offset.EMPTY, 5)))
                .extracting(TimelineItemEntity::getLaunch)
                .extracting(Launch.Id::getLaunchNumber)
                .containsExactly(11, 9, 8, 6, 5);

        assertThat(roTx(() -> db.timeline().getTimeline(PROCESS_ID, ArcBranch.trunk(), Offset.EMPTY, 5)))
                .extracting(TimelineItemEntity::getLaunch)
                .extracting(Launch.Id::getLaunchNumber)
                .containsExactly(11, 9, 8, 6, 5);
    }

    private static TimelineItemEntity.Builder entity(OrderedArcRevision revision, int version, int itemNumber) {
        return TimelineItemEntity.builder()
                .timelineVersion(version)
                .showInBranch(ArcBranch.trunk().asString())
                .id(TimelineItemEntity.Id.of(PROCESS_ID, revision, itemNumber));
    }

    @Nested
    class GetPreviousStable {
        @Test
        void toLaunch() {
            tx(() -> createLaunch(TestData.TRUNK_R2, ++launchNumber));
            Launch l2 = tx(() -> createLaunch(TestData.TRUNK_R3, ++launchNumber));
            tx(() -> createLaunch(TestData.TRUNK_R4, ++launchNumber));
            Launch l4 = tx(() -> createLaunch(TestData.TRUNK_R5, ++launchNumber));

            assertThat(tx(() -> timelineService.getPreviousStable(l4))).isEmpty();

            tx(() -> timelineService.updateTimelineLaunchItem(success(l2)));

            assertThat(tx(() -> timelineService.getPreviousStable(l4)))
                    .get()
                    .extracting(TimelineItem::getArcRevision)
                    .isEqualTo(TestData.TRUNK_R3);
        }

        @Test
        void toBranch() {
            var branch2 = tx(() -> createBranch(TestData.TRUNK_R3));
            var l4 = tx(() -> createLaunch(TestData.TRUNK_R5, ++launchNumber));

            assertThat(tx(() -> timelineService.getPreviousStable(l4))).isEmpty();

            tx(() -> timelineService.branchUpdated(
                    branchWithRegisteredLaunch(branch2, LaunchState.Status.RUNNING)
            ));

            assertThat(tx(() -> timelineService.getPreviousStable(l4))).isEmpty();

            tx(() -> timelineService.branchUpdated(
                    branchWithRegisteredLaunch(branch2, LaunchState.Status.SUCCESS)
            ));

            assertThat(tx(() -> timelineService.getPreviousStable(l4)))
                    .get()
                    .extracting(TimelineItem::getArcRevision)
                    .isEqualTo(TestData.TRUNK_R3);
        }

        private Branch branchWithRegisteredLaunch(Branch branch, LaunchState.Status launchState) {
            var state = branch.getItem().getState().registerLaunch(99, launchState, false);
            return branch.withItem(branch.getItem().toBuilder().state(state).build());
        }
    }

    @Test
    void getTimeline() {
        List<Launch> launches = tx(() -> IntStream.range(1, 10)
                .mapToObj(rev -> launchAtRevision(TestData.trunkRevision(rev), rev * 100 + rev))
                .peek(timelineService::updateTimelineLaunchItem)
                .collect(Collectors.toList()));


        tx(() -> launches.stream()
                .filter(l -> l.getVcsInfo().getRevision().getNumber() % 3 == 1)
                .map(TimelineServiceTest::cancelled)
                .forEach(timelineService::updateTimelineLaunchItem));

        List<Branch> branches = IntStream.range(1, 30)
                .map(i -> i * 2)
                .filter(i -> i < 10)
                .mapToObj(rev -> branchAtRevision(PROCESS_ID, TestData.trunkRevision(rev), "branch-" + rev))
                .toList();

        List<Branch> secondBranches = IntStream.range(1, 30)
                .map(i -> i * 4)
                .filter(i -> i < 10)
                .mapToObj(rev -> branchAtRevision(PROCESS_ID, TestData.trunkRevision(rev), "second-branch-" + rev))
                .toList();

        db.currentOrTx(() -> {
            db.launches().save(launches);
            branches.forEach(b -> {
                db.branches().save(b.getInfo());
                db.timelineBranchItems().save(b.getItem());
            });
            secondBranches.forEach(b -> {
                db.branches().save(b.getInfo());
                db.timelineBranchItems().save(b.getItem());
            });
        });

        branches.forEach(branch -> tx(() -> timelineService.addBranch(branch)));
        secondBranches.forEach(branch -> tx(() -> timelineService.addBranch(branch)));

        List<TimelineItem> timeline = timelineService.getTimeline(PROCESS_ID, ArcBranch.trunk(), Offset.EMPTY, 6);
        //        selected:       [         ]
        //             rev: 1 2 3 4 5 6 7 8 9 ...
        //        launches:   - -   - -   - -
        //       cancelled: ^     ^     ^
        //        branches:   -   -   -   -
        // second branches:       -       -

        assertThat(timeline)
                .hasSize(6)
                .satisfies(item -> assertItem(item).hasRev(9).hasLaunch(909), at(0))
                .satisfies(item -> assertItem(item).hasRev(8).hasBranch(releaseBr("second-branch-8")), at(1))
                .satisfies(item -> assertItem(item).hasRev(8).hasBranch(releaseBr("branch-8")), at(2))
                .satisfies(item -> assertItem(item).hasRev(6).hasBranch(releaseBr("branch-6")), at(3))
                .satisfies(item -> assertItem(item).hasRev(5).hasLaunch(505), at(4))
                .satisfies(item -> assertItem(item).hasRev(4).hasBranch(releaseBr("second-branch-4")), at(5));

        Offset offset = timeline.get(timeline.size() - 1).getNextStart();
        timeline = timelineService.getTimeline(PROCESS_ID, ArcBranch.trunk(), offset, -1);

        assertThat(timeline)
                .hasSize(3)
                .satisfies(item -> assertItem(item).hasRev(4).hasBranch(releaseBr("branch-4")), at(0))
                .satisfies(item -> assertItem(item).hasRev(3).hasLaunch(303), at(1))
                .satisfies(item -> assertItem(item).hasRev(2).hasBranch(releaseBr("branch-2")), at(2));
    }


    private List<TimelineItemEntity> getTimelineInTrunk() {
        return getTimelineIn(ArcBranch.trunk());
    }

    private List<TimelineItemEntity> getTimelineIn(ArcBranch branch) {
        return roTx(() -> db.timeline().getTimeline(PROCESS_ID, branch, Offset.EMPTY, -1));
    }

    private <T> T roTx(Supplier<T> supplier) {
        return db.currentOrReadOnly(supplier);
    }

    private <T> T tx(Supplier<T> supplier) {
        return db.currentOrTx(supplier);
    }

    private void tx(Runnable runnable) {
        db.currentOrTx(runnable);
    }

    private static Launch cancelled(Launch launch) {
        return withStatus(launch, LaunchState.Status.CANCELED);
    }

    private static Launch success(Launch launch) {
        return withStatus(launch, LaunchState.Status.SUCCESS);
    }

    private static Launch withStatus(Launch launch, LaunchState.Status status) {
        return launch.toBuilder().status(status).build();
    }

    private static Launch launchAtRevision(OrderedArcRevision revision, int launchNumber) {
        return Launch.builder()
                .launchId(LaunchId.of(PROCESS_ID, launchNumber))
                .type(Launch.Type.USER)
                .notifyPullRequest(false)
                .vcsInfo(LaunchVcsInfo.builder()
                        .revision(revision)
                        .build())
                .flowInfo(LaunchFlowInfo.builder()
                        .configRevision(revision)
                        .flowId(TestData.PR_NEW_CONFIG_SAWMILL_FLOW_ID)
                        .runtimeInfo(LaunchRuntimeInfo.ofRuntimeSandboxOwner(TestData.YAV_TOKEN_UUID, "CI"))
                        .build())
                .version(Version.major("23"))
                .build();
    }

    private static Branch branchAtRevision(CiProcessId processId, OrderedArcRevision revision, String simpleName) {
        return branchAtRevision(processId, revision, ArcBranch.ofBranchName(releaseBr(simpleName)));
    }

    private static Branch branchAtRevision(CiProcessId processId, OrderedArcRevision revision, ArcBranch branch) {

        return Branch.of(BranchInfo.builder()
                        .branch(branch.asString())
                        .commitId(revision.getCommitId())
                        .baseRevision(revision)
                        .build(),
                TimelineBranchItem.builder()
                        .idOf(processId, branch)
                        .state(BranchState.builder().build())
                        .vcsInfo(BranchVcsInfo.builder().build())
                        .version(Version.majorMinor("1", "7"))
                        .build()
        );
    }

    private static String releaseBr(String name) {
        return "releases/ci/" + name;
    }

    private static Index at(int i) {
        return Index.atIndex(i);
    }
}
