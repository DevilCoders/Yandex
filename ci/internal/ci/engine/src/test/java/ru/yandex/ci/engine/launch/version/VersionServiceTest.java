package ru.yandex.ci.engine.launch.version;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.engine.branch.BranchService;

import static org.assertj.core.api.Assertions.assertThat;

class VersionServiceTest extends EngineTestBase {
    @Autowired
    private BranchService branchService;

    @Autowired
    private LaunchVersionService launchVersionService;

    @Autowired
    private BranchVersionService branchVersionService;

    @Test
    void branch() {
        assertThat(branchAt(TestData.TRUNK_R5, TestData.RELEASE_BRANCH_1))
                .isEqualTo(Version.major("1"));

        assertThat(branchAt(TestData.TRUNK_R6, TestData.RELEASE_BRANCH_2))
                .isEqualTo(Version.major("2"));

        assertThat(branchAt(TestData.TRUNK_R2, ArcBranch.ofBranchName("releases/ci/release-component-3")))
                .isEqualTo(Version.major("3"));
    }

    @Test
    void release() {
        assertThat(releaseAt(TestData.TRUNK_R5, ArcBranch.trunk()))
                .isEqualTo(Version.major("1"));

        assertThat(releaseAt(TestData.TRUNK_R6, ArcBranch.trunk()))
                .isEqualTo(Version.major("2"));

        assertThat(releaseAt(TestData.TRUNK_R2, ArcBranch.trunk()))
                .isEqualTo(Version.major("3"));
    }

    @Test
    void branch_release() {
        Version branchVersion = branchAt(TestData.TRUNK_R2, TestData.RELEASE_BRANCH_1);
        assertThat(branchVersion).isEqualTo(Version.major("1"));

        Version releaseVersion = releaseAt(TestData.TRUNK_R2, TestData.RELEASE_BRANCH_1);
        assertThat(branchVersion).isEqualTo(releaseVersion);
    }

    @Test
    void branch_branchInFuture_release() {
        Version branch1Version = branchAt(TestData.TRUNK_R2, TestData.RELEASE_BRANCH_1);
        Version branch2Version = branchAt(TestData.TRUNK_R6, TestData.RELEASE_BRANCH_2);

        assertThat(branch1Version).isEqualTo(Version.major("1"));
        assertThat(branch2Version).isEqualTo(Version.major("2"));

        Version releaseVersion = releaseAt(TestData.TRUNK_R2, TestData.RELEASE_BRANCH_1);
        assertThat(releaseVersion).isEqualTo(branch1Version);
    }

    @Test
    void branch_releaseCancelled_release() {
        branchAt(TestData.TRUNK_R2, TestData.RELEASE_BRANCH_1);
        releaseAt(TestData.TRUNK_R2, TestData.RELEASE_BRANCH_1);

        assertThat(releaseAt(TestData.TRUNK_R2, TestData.RELEASE_BRANCH_1))
                .isEqualTo(Version.majorMinor("1", "1"));
    }

    @Test
    void branch_releaseInTrunk_release() {
        branchAt(TestData.TRUNK_R2, TestData.RELEASE_BRANCH_1);
        releaseAt(TestData.TRUNK_R4, ArcBranch.trunk());

        assertThat(releaseAt(TestData.TRUNK_R2, TestData.RELEASE_BRANCH_1))
                .isEqualTo(Version.major("1"));
    }

    @Test
    void branch_releaseInTrunk_releaseAtBranchCommit() {
        branchAt(TestData.TRUNK_R2, TestData.RELEASE_BRANCH_1);
        releaseAt(TestData.TRUNK_R4, ArcBranch.trunk());

        assertThat(releaseAt(TestData.RELEASE_R2_1, TestData.RELEASE_BRANCH_1))
                .isEqualTo(Version.majorMinor("1", "1"));
    }

    @Test
    void branch_releaseAtBranchCommit() {
        branchAt(TestData.TRUNK_R2, TestData.RELEASE_BRANCH_1);

        assertThat(releaseAt(TestData.RELEASE_R2_1, TestData.RELEASE_BRANCH_1))
                .isEqualTo(Version.majorMinor("1", "1"));
    }

    @Test
    void branch_releaseAtBranchCommit_release() {
        branchAt(TestData.TRUNK_R2, TestData.RELEASE_BRANCH_1);
        releaseAt(TestData.RELEASE_R2_1, TestData.RELEASE_BRANCH_1);

        assertThat(releaseAt(TestData.TRUNK_R2, TestData.RELEASE_BRANCH_1))
                .isEqualTo(Version.majorMinor("1", "2"));
    }

    @Test
    void releaseCancelled_release() {
        releaseAt(TestData.TRUNK_R4, ArcBranch.trunk());

        assertThat(releaseAt(TestData.TRUNK_R2, ArcBranch.trunk()))
                .isEqualTo(Version.major("2"));
    }

    @Test
    void branch_releaseCancelled_releaseInTrunk_releaseAtBranchCommit() {
        branchAt(TestData.TRUNK_R2, TestData.RELEASE_BRANCH_1);
        releaseAt(TestData.TRUNK_R2, TestData.RELEASE_BRANCH_1);
        releaseAt(TestData.TRUNK_R4, ArcBranch.trunk());

        assertThat(releaseAt(TestData.TRUNK_R2, TestData.RELEASE_BRANCH_1))
                .isEqualTo(Version.majorMinor("1", "1"));
    }

    @Test
    void branch_release_release() {
        branchAt(TestData.TRUNK_R2, TestData.RELEASE_BRANCH_1);
        branchAt(TestData.TRUNK_R6, TestData.RELEASE_BRANCH_2);

        assertThat(releaseAt(TestData.RELEASE_R2_1, TestData.RELEASE_BRANCH_1))
                .isEqualTo(Version.majorMinor("1", "1"));

        assertThat(releaseAt(TestData.TRUNK_R6, TestData.RELEASE_BRANCH_2))
                .isEqualTo(Version.major("2"));

        assertThat(releaseAt(TestData.RELEASE_R6_1, TestData.RELEASE_BRANCH_2))
                .isEqualTo(Version.majorMinor("2", "1"));

        assertThat(releaseAt(TestData.RELEASE_R2_2, TestData.RELEASE_BRANCH_1))
                .isEqualTo(Version.majorMinor("1", "2"));

        assertThat(releaseAt(TestData.RELEASE_R6_3, TestData.RELEASE_BRANCH_2))
                .isEqualTo(Version.majorMinor("2", "2"));

        assertThat(releaseAt(TestData.TRUNK_R6, TestData.RELEASE_BRANCH_2))
                .isEqualTo(Version.majorMinor("2", "3"));
    }

    @Test
    void release_branchInFuture() {
        releaseAt(TestData.TRUNK_R2, ArcBranch.trunk());

        assertThat(branchAt(TestData.TRUNK_R4, TestData.RELEASE_BRANCH_1))
                .isEqualTo(Version.major("2"));
    }

    @Test
    void branch_releaseInFuture() {
        branchAt(TestData.TRUNK_R2, TestData.RELEASE_BRANCH_1);

        assertThat(releaseAt(TestData.TRUNK_R4, ArcBranch.trunk()))
                .isEqualTo(Version.major("2"));
    }

    @Test
    void releaseCancelled_branch() {
        releaseAt(TestData.TRUNK_R2, ArcBranch.trunk());
        assertThat(branchAt(TestData.TRUNK_R2, TestData.RELEASE_BRANCH_1))
                .isEqualTo(Version.major("1"));
    }

    private Version branchAt(OrderedArcRevision revision, ArcBranch branch) {
        return db.currentOrTx(() -> {
            CiProcessId processId = TestData.RELEASE_PROCESS_ID;
            var version = branchVersionService.nextBranchVersion(processId, revision, null);
            branchService.doCreateBranch(
                    revision,
                    processId,
                    revision,
                    branch,
                    TestData.CI_USER,
                    version,
                    "TEST_TOKEN"
            );
            return version;
        });
    }

    private Version releaseAt(OrderedArcRevision revision, ArcBranch selectedBranch) {
        return db.currentOrTx(() ->
                launchVersionService.nextLaunchVersion(TestData.RELEASE_PROCESS_ID, revision, selectedBranch, null)
        );
    }
}
