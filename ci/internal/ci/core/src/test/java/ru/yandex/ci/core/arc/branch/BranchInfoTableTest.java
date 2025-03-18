package ru.yandex.ci.core.arc.branch;

import java.util.List;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.CommonYdbTestBase;
import ru.yandex.ci.core.arc.ArcRevision;

import static org.assertj.core.api.Assertions.assertThat;

public class BranchInfoTableTest extends CommonYdbTestBase {

    private static final String BRANCH_PREFIX = "releases/project/";

    @BeforeEach
    public void setUp() {
        db.currentOrTx(() -> db.branches().save(
                branch("r1", "red"),
                branch("r1", "green"),
                branch("r1", "blue"),
                branch("r2", "white"),
                branch("r2", "black"),
                branch("r3", "rose")
        ));
    }

    @Test
    void byCommitId() {
        List<BranchInfo> branches = db.currentOrTx(() -> db.branches().findAtCommit(ArcRevision.of("r2")));

        assertThat(branches)
                .extracting(BranchInfoTableTest::extractName)
                .containsExactlyInAnyOrder("white", "black");
    }

    @Test
    void byCommitIdEmpty() {
        List<BranchInfo> branches = db.currentOrTx(() -> db.branches().findAtCommit(ArcRevision.of("r9")));

        assertThat(branches).isEmpty();
    }

    private static BranchInfo branch(String commitId, String name) {
        return BranchInfo.builder()
                .commitId(commitId)
                .branch(BRANCH_PREFIX + name)
                .build();
    }

    private static String extractName(BranchInfo info) {
        return info.getArcBranch().asString().substring(BRANCH_PREFIX.length());
    }
}
