package ru.yandex.ci.engine.branch;

import java.util.List;
import java.util.Map;
import java.util.Set;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.Branch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.core.timeline.BranchVcsInfo;
import ru.yandex.ci.core.timeline.Offset;
import ru.yandex.ci.core.timeline.TimelineItem;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.engine.timeline.TimelineService;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;
import static ru.yandex.ci.core.test.TestData.CI_USER;
import static ru.yandex.ci.core.test.TestData.TRUNK_R2;
import static ru.yandex.ci.core.test.TestData.TRUNK_R3;
import static ru.yandex.ci.core.test.TestData.TRUNK_R4;

public class BranchServiceTest extends EngineTestBase {
    private static final CiProcessId PROCESS_ID =
            CiProcessId.ofRelease(TestData.CONFIG_PATH_WITH_BRANCHES_RELEASE, "with-branches");

    private static final int LIMIT = 10;

    @Autowired
    private BranchService branchService;

    @Autowired
    private TimelineService timelineService;

    @BeforeEach
    public void setUp() {
        mockValidationSuccessful();
        discoveryToR4();
        delegateToken(PROCESS_ID.getPath());
    }

    @Test
    void startVersion() {
        CiProcessId processId = CiProcessId.ofRelease(
                TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-release-with-start-version"
        );

        mockValidationSuccessful();
        discoveryToR2();
        delegateToken(processId.getPath());

        var branch = db.currentOrTx(() -> branchService.createBranch(processId, TRUNK_R2, CI_USER));
        assertThat(branch.getVersion()).isEqualTo(Version.major("42"));
    }

    @Test
    void discoverUnconditional() {
        CiProcessId processId = CiProcessId.ofRelease(
                TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-release-with-start-version"
        );

        mockValidationSuccessful();
        discoveryToR2();
        delegateToken(processId.getPath());

        var branch = db.currentOrTx(() -> branchService.createBranch(processId, TRUNK_R2, CI_USER));
        var arcBranch = branch.getArcBranch();
        var revision = OrderedArcRevision.fromHash("r100500", arcBranch, 1, 0);

        arcServiceStub.addCommit(TestData.toBranchCommit(revision, CI_USER), TRUNK_R2, Map.of());
        discoveryServicePostCommits.processPostCommit(revision.getBranch(), revision.toRevision(), false);

        var updated = db.currentOrTx(() -> branchService.getBranch(arcBranch, processId));
        assertThat(updated.getVcsInfo().getHead())
                .isEqualTo(revision);
    }

    @Test
    void getBranchesAtRevisions() {
        db.currentOrTx(() ->
                List.of(
                        branchService.createBranch(PROCESS_ID, TRUNK_R4, CI_USER),
                        branchService.createBranch(PROCESS_ID, TRUNK_R4, CI_USER),
                        branchService.createBranch(PROCESS_ID, TRUNK_R3, CI_USER),
                        branchService.createBranch(PROCESS_ID, TRUNK_R2, CI_USER),
                        branchService.createBranch(PROCESS_ID, TRUNK_R2, CI_USER)
                )
        );

        var atRevisions = db.currentOrTx(() ->
                branchService.getBranchesAtRevisions(PROCESS_ID, Set.of(TRUNK_R4, TRUNK_R2)));
        assertThat(atRevisions.asMap())
                .containsOnlyKeys(TRUNK_R4.toRevision(), TRUNK_R2.toRevision());

        assertThat(atRevisions.get(TRUNK_R2.toRevision()))
                .extracting(ArcBranch::asString)
                .containsExactlyInAnyOrder(
                        "releases/ci-test/test-sawmill-release-4",
                        "releases/ci-test/test-sawmill-release-5"
                );

        assertThat(atRevisions.get(TRUNK_R4.toRevision()))
                .extracting(ArcBranch::asString)
                .containsExactlyInAnyOrder(
                        "releases/ci-test/test-sawmill-release-1",
                        "releases/ci-test/test-sawmill-release-2"
                );
    }

    @Test
    void forbidBranchesFromBranches() {
        List.of(
                TestData.RELEASE_R6_1,
                TestData.RELEASE_R6_2,
                TestData.RELEASE_R6_3,
                TestData.RELEASE_R6_4
        ).forEach(r -> db.currentOrTx(() ->
                discoveryServicePostCommits.processPostCommit(r.getBranch(), r.toRevision(), false)
        ));

        assertThatThrownBy(() ->
                db.currentOrTx(() -> branchService.createBranch(PROCESS_ID, TestData.RELEASE_R6_2, CI_USER)))
                .hasMessageContaining("cannot create branch from another branch revision");
    }

    @Test
    void createImplicit() {
        OrderedArcRevision revision = TestData.TRUNK_R4;

        ConfigBundle config = configurationService.getLastValidConfig(TestData.CONFIG_PATH_WITH_BRANCHES_RELEASE,
                ArcBranch.trunk());

        // единственный первый запуск будет с версии 1, для него создается неявная ветка
        var version = Version.major("1");
        db.currentOrTx(() ->
                branchService.createImplicitReleaseBranch(PROCESS_ID, config, revision, CI_USER, version)
        );

        String expectedBranch = "releases/ci-test/test-sawmill-release-1";

        assertThat(db.currentOrTx(() -> branchService.getBranches(PROCESS_ID, null, LIMIT)))
                .extracting(Branch::getArcBranch)
                .extracting(ArcBranch::asString)
                .containsExactly(expectedBranch);

        assertThat(db.currentOrTx(() ->
                timelineService.getTimeline(PROCESS_ID, ArcBranch.trunk(), Offset.EMPTY, LIMIT)))
                .isEmpty();

        assertThat(db.currentOrTx(
                () -> branchService.createBranch(PROCESS_ID, revision, CI_USER))
        )
                .extracting(Branch::getArcBranch)
                .extracting(ArcBranch::asString)
                .isEqualTo(expectedBranch);

        assertThat(db.currentOrTx(() ->
                timelineService.getTimeline(PROCESS_ID, ArcBranch.trunk(), Offset.EMPTY, LIMIT)))
                .extracting(TimelineItem::getBranch)
                .extracting(Branch::getArcBranch)
                .extracting(ArcBranch::asString)
                .containsExactly(expectedBranch);
    }

    @Test
    void trunkCommitCount() {
        Branch branchAtR2 = db.currentOrTx(() -> branchService.createBranch(PROCESS_ID, TRUNK_R2, CI_USER));
        BranchVcsInfo vcsInfoR2 = branchAtR2.getVcsInfo();
        assertThat(vcsInfoR2.getBranchCommitCount()).isEqualTo(0);
        assertThat(vcsInfoR2.getTrunkCommitCount()).isEqualTo(1);
        assertThat(vcsInfoR2.getPreviousRevision()).isNull();

        Branch branchAtR4 = db.currentOrTx(() -> branchService.createBranch(PROCESS_ID, TestData.TRUNK_R4, CI_USER));
        BranchVcsInfo vcsInfoR4 = branchAtR4.getVcsInfo();
        assertThat(vcsInfoR4.getBranchCommitCount()).isEqualTo(0);
        assertThat(vcsInfoR4.getTrunkCommitCount()).isEqualTo(2);
        assertThat(vcsInfoR4.getPreviousRevision()).isEqualTo(TRUNK_R2);
    }

    @Test
    void resolveBranchNameConflictAheadOrArc() {
        Branch existing = db.currentOrTx(() -> branchService.createBranch(PROCESS_ID, TRUNK_R2, CI_USER));

        db.currentOrTx(() -> {
            db.versions().deleteAll();
            db.counter().deleteAll();
        });

        Branch withNameConflict = db.currentOrTx(() -> branchService.createBranch(PROCESS_ID, TRUNK_R3, CI_USER));

        assertThat(withNameConflict.getArcBranch().asString())
                .describedAs("new branch name should be with suffix")
                .isEqualTo(existing.getArcBranch().asString() + "-1");
    }
}
