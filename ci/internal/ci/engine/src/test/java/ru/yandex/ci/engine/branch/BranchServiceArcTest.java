package ru.yandex.ci.engine.branch;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.test.context.ActiveProfiles;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.Branch;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.spring.clients.ArcClientTestConfig;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;

@ActiveProfiles(ArcClientTestConfig.CANON_ARC_PROFILE)
class BranchServiceArcTest extends EngineTestBase {

    private static final String NOT_EXIST_COMMIT_ID = "126d149c93ae294240eb2e74ade1772b012614b1";
    private static final CiProcessId PROCESS_ID = TestData.RELEASE_PROCESS_ID;
    private static final Version VERSION = Version.major("1.7");

    @Autowired
    private BranchService branchService;

    @Test
    void successful() {
        OrderedArcRevision configRevision = revision("12345");
        OrderedArcRevision revision = revision("126d149c93ae294240eb2e74ade1772b012614b5");
        Branch branch = db.currentOrTx(() ->
                branchService.doCreateBranch(
                        configRevision,
                        PROCESS_ID,
                        revision,
                        ArcBranch.ofString("releases/ci/test-branch-service-successful"),
                        "pochemuto",
                        VERSION,
                        "TEST_TOKEN"));
        assertThat(branch).isNotNull();
        assertThat(branch.getInfo()).isNotNull();
        assertThat(branch.getInfo().getBaseRevision()).isEqualTo(revision);
        assertThat(branch.getInfo().getConfigRevision()).isEqualTo(configRevision);
    }

    @Test
    void fastForward() {
        OrderedArcRevision configRevision = revision("12345");
        OrderedArcRevision revision = revision("0abe603a91fda0f59fb439e7cf9c9309b87d2bf3");
        ArcBranch branch = ArcBranch.ofString("releases/ci/release-pochemuto-1");

        assertThatThrownBy(() -> db.currentOrTx(() ->
                branchService.doCreateBranch(
                        configRevision,
                        PROCESS_ID,
                        revision,
                        branch,
                        "pochemuto",
                        VERSION,
                        "TEST_TOKEN")))
                .isInstanceOf(BranchConflictException.class)
                .hasMessageContaining("Not fast forward");

        assertThat(db.currentOrTx(() -> branchService.getBranch(branch))).isEmpty();
    }

    @Test
    void rebase() {
        OrderedArcRevision configRevision = revision("12345");
        OrderedArcRevision revision = revision("7b487105c4adc59fc018592d4bbd2e32d0beb753");
        ArcBranch branch = ArcBranch.ofString("releases/ci/release-pochemuto-2");

        assertThatThrownBy(() -> db.currentOrTx(() ->
                branchService.doCreateBranch(
                        configRevision,
                        PROCESS_ID,
                        revision,
                        branch,
                        "pochemuto",
                        VERSION,
                        "TEST_TOKEN")))
                .isInstanceOf(BranchConflictException.class)
                .hasMessageContaining("Not fast forward");

        assertThat(db.currentOrTx(() -> branchService.getBranch(branch))).isEmpty();
    }


    @Test
    void commitNotFound() {
        OrderedArcRevision configRevision = revision("12345");
        OrderedArcRevision revision = revision(NOT_EXIST_COMMIT_ID);
        ArcBranch branch = ArcBranch.ofString("releases/ci/it-branch-service-not-found");
        assertThatThrownBy(() -> db.currentOrTx(() ->
                branchService.doCreateBranch(
                        configRevision,
                        PROCESS_ID,
                        revision,
                        branch,
                        "pochemuto",
                        VERSION,
                        "TEST_TOKEN")))
                .hasMessageContaining("126d149c93ae294240eb2e74ade1772b012614b1")
                .hasMessageContaining("not found");

        assertThat(db.currentOrTx(() -> branchService.getBranch(branch))).isEmpty();
    }

    private static OrderedArcRevision revision(String hash) {
        return OrderedArcRevision.fromHash(hash, ArcBranch.trunk(), 0, 0);
    }
}
