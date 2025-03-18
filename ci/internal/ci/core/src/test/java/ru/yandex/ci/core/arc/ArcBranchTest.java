package ru.yandex.ci.core.arc;

import org.junit.jupiter.api.Test;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;


class ArcBranchTest {

    @Test
    void trunk() {
        assertThat("trunk").isEqualTo(ArcBranch.trunk().asString());
        assertThat(ArcBranch.trunk()).isSameAs(ArcBranch.trunk());
        assertThat(ArcBranch.ofString("trunk")).isSameAs(ArcBranch.trunk());
        assertThatThrownBy(() -> ArcBranch.trunk().getPullRequestId())
                .isInstanceOf(IllegalStateException.class);

        assertThat(ArcBranch.Type.TRUNK).isEqualTo(ArcBranch.ofString("trunk").getType());
    }

    @Test
    void pr() {
        ArcBranch prBranch = ArcBranch.ofPullRequest(42);

        assertThat(ArcBranch.Type.PR).isEqualTo(prBranch.getType());
        assertThat(42).isEqualTo(prBranch.getPullRequestId());
        assertThat("pr:42").isEqualTo(prBranch.asString());
        assertThat(prBranch).isEqualTo(ArcBranch.ofPullRequest(42));

        assertThat(prBranch).isEqualTo(ArcBranch.ofString("pr:42"));
    }

    @Test
    void regularBranchIsTrunk() {
        ArcBranch branch = ArcBranch.ofBranchName("trunk");

        assertThat(branch.isTrunk()).isTrue();
        assertThat(branch).isSameAs(ArcBranch.trunk());
        assertThat(branch.getType()).isEqualTo(ArcBranch.Type.TRUNK);
    }

    @Test
    void userBranch() {
        ArcBranch branch = ArcBranch.ofBranchName("users/pochemuto/release-branches");

        assertThat(branch.isTrunk()).isFalse();
        assertThat(branch.isRelease()).isFalse();
        assertThat(branch.isUser()).isTrue();
        assertThat(branch.asString()).isEqualTo("users/pochemuto/release-branches");
        assertThat(branch.getType()).isEqualTo(ArcBranch.Type.USER_BRANCH);
    }

    @Test
    void releaseBranch() {
        ArcBranch branch = ArcBranch.ofBranchName("releases/ci/release-branches-CI-001");

        assertThat(branch.isTrunk()).isFalse();
        assertThat(branch.isRelease()).isTrue();
        assertThat(branch.isUser()).isFalse();
        assertThat(branch.asString()).isEqualTo("releases/ci/release-branches-CI-001");
        assertThat(branch.getType()).isEqualTo(ArcBranch.Type.RELEASE_BRANCH);
    }

    @Test
    void groupBranch() {
        ArcBranch branch = ArcBranch.ofBranchName("groups/clickhouse_sync/ClickHouse_ClickHouse_pull_13956_5");
        assertThat(branch.getType()).isEqualTo(ArcBranch.Type.GROUP_BRANCH);
        assertThat(branch.isGroup()).isTrue();
        assertThat(branch.isRelease()).isFalse();
        assertThat(branch.asString()).isEqualTo("groups/clickhouse_sync/ClickHouse_ClickHouse_pull_13956_5");
    }

    @Test
    void unknownBranch() {
        ArcBranch branch = ArcBranch.ofBranchName("something-new/abc");
        assertThat(branch.getType()).isEqualTo(ArcBranch.Type.UNKNOWN);
        assertThat(branch.isGroup()).isFalse();
        assertThat(branch.isRelease()).isFalse();
        assertThat(branch.isUnknown()).isTrue();
    }
}
