package ru.yandex.ci.engine.discovery;

import java.util.List;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.core.discovery.DiscoveredCommit;
import ru.yandex.ci.core.project.ReleaseConfigState;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.engine.branch.BranchService;

import static org.assertj.core.api.Assertions.assertThat;

public class DiscoveryServiceInBranchTest extends EngineTestBase {

    @Autowired
    private BranchService branchService;

    @BeforeEach
    void setUp() {
        mockYav();
        mockValidationSuccessful();
    }

    @Test
    void discoverCommitEvenIfPreviousIsNotDiscoveredYet() {
        discoveryToR2();

        discoveryServicePostCommits.processPostCommit(
                TestData.RELEASE_R2_2.getBranch(),
                TestData.RELEASE_R2_2.toRevision(), false
        );

        assertThat(TestData.RELEASE_R2_2.getBranch().isRelease()).isTrue();

        List<DiscoveredCommit> commits = findCommits(
                TestData.SIMPLE_FILTER_RELEASE_PROCESS_ID,
                TestData.RELEASE_R2_2.getBranch(), -1, -1, -1
        );

        assertThat(commits).hasSize(1);
        DiscoveredCommit discoveredCommit = commits.get(0);

        assertThat(discoveredCommit.getProcessId()).isEqualTo(TestData.SIMPLE_FILTER_RELEASE_PROCESS_ID);
        assertThat(discoveredCommit.getArcRevision()).isEqualTo(TestData.RELEASE_R2_2);

        // early commit discovered later
        discoveryServicePostCommits.processPostCommit(
                TestData.RELEASE_R2_1.getBranch(),
                TestData.RELEASE_R2_1.toRevision(), false
        );

        assertThat(findCommits(
                TestData.SIMPLE_FILTER_RELEASE_PROCESS_ID,
                TestData.RELEASE_R2_2.getBranch(), -1, -1, -1
        )).hasSize(2);
    }

    @Test
    void removedConfigInBranchDoesntAffectConfigState() {
        discoveryToR2();

        ConfigState state = db.currentOrTx(() -> db.configStates().get(TestData.CONFIG_PATH_ABC));

        assertThat(state.getStatus()).isEqualTo(ConfigState.Status.OK);

        discoveryServicePostCommits.processPostCommit(
                TestData.RELEASE_R2_1.getBranch(),
                TestData.RELEASE_R2_1.toRevision(), false
        );

        ConfigState stateAfterUpdate = db.currentOrTx(() -> db.configStates().get(TestData.CONFIG_PATH_ABC));
        assertThat(stateAfterUpdate).isEqualTo(state);
    }

    @Test
    void discoverCommitsToBranchesEvenBranchesWereNotConfiguredAtBranchBaseRevisionYet() {
        var processId = CiProcessId.ofRelease(
                TestData.CONFIG_PATH_WITH_BRANCHES_RELEASE, "with-branches-enabled-later");

        discoveryToR4();

        var stateR3 = db.currentOrTx(() -> db.configStates().get(processId.getPath()));

        assertThat(getRelease(stateR3, processId.getSubId()).isReleaseBranchesEnabled())
                .describedAs("config should NOT have release branches enabled")
                .isFalse();

        discoveryToR6();
        delegateToken(processId.getPath());

        var stateR5 = db.currentOrTx(() -> db.configStates().get(processId.getPath()));

        assertThat(getRelease(stateR5, processId.getSubId()).isReleaseBranchesEnabled())
                .describedAs("config should HAVE release branches enabled")
                .isTrue();

        var branch = db.currentOrTx(() -> branchService.createBranch(processId, TestData.TRUNK_R2, TestData.CI_USER));

        assertThat(branch.getItem().getVcsInfo().getHead())
                .describedAs("not branch commits yet")
                .isEqualTo(TestData.TRUNK_R2);
        assertThat(branch.getItem().getVcsInfo().getBranchCommitCount()).isZero();

        discoveryServicePostCommits.processPostCommit(
                branch.getArcBranch(),
                TestData.RELEASE_R2_1.toRevision(), false
        );

        var updatedBranch = db.currentOrTx(() -> branchService.getBranch(branch.getArcBranch(), processId));

        assertThat(updatedBranch.getItem().getVcsInfo().getHead())
                .describedAs("should have one discovered commit")
                .isEqualTo(TestData.projectRevToBranch(TestData.RELEASE_R2_1, branch.getArcBranch()));

        assertThat(updatedBranch.getItem().getVcsInfo().getBranchCommitCount()).isEqualTo(1);
    }

    private List<DiscoveredCommit> findCommits(CiProcessId processId, ArcBranch branch,
                                               long fromCommitNumber, long toCommitNumber, int limit) {
        return db.currentOrReadOnly(() ->
                db.discoveredCommit().findCommits(
                        processId,
                        branch,
                        fromCommitNumber,
                        toCommitNumber,
                        limit));
    }

    private static ReleaseConfigState getRelease(ConfigState configState, String releaseId) {
        var release = configState.findRelease(releaseId);
        assertThat(release).isNotEmpty();
        return release.get();
    }
}
