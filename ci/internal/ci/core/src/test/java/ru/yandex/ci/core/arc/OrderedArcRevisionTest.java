package ru.yandex.ci.core.arc;


import org.junit.jupiter.api.Test;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;

class OrderedArcRevisionTest {
    @Test
    void isBefore() {
        assertThat(trunk(1).isBefore(trunk(2))).isTrue();
        assertThat(trunk(2).isBefore(trunk(1))).isFalse();
        assertThat(trunk(3).isBefore(trunk(3))).isFalse();

        assertThat(branch(1, "release").isBefore(branch(2, "release"))).isTrue();
        assertThat(branch(2, "user").isBefore(branch(1, "user"))).isFalse();
        assertThat(branch(3, "component").isBefore(branch(3, "component"))).isFalse();
    }

    @Test
    void isBeforeDifferentBranches() {
        assertThatThrownBy(() -> trunk(1).isBefore(branch(2, "release")))
                .hasMessageContaining("Cannot compare revisions from different branches");

        assertThatThrownBy(() -> branch(1, "user").isBefore(trunk(2)))
                .hasMessageContaining("Cannot compare revisions from different branches");

        assertThatThrownBy(() -> branch(1, "user").isBefore(branch(1, "release")))
                .hasMessageContaining("Cannot compare revisions from different branches");
    }

    @Test
    void isBeforeOrSame() {
        assertThat(trunk(1).isBeforeOrSame(trunk(2))).isTrue();
        assertThat(trunk(2).isBeforeOrSame(trunk(1))).isFalse();
        assertThat(trunk(3).isBeforeOrSame(trunk(3))).isTrue();

        assertThat(branch(1, "release").isBeforeOrSame(branch(2, "release"))).isTrue();
        assertThat(branch(2, "user").isBeforeOrSame(branch(1, "user"))).isFalse();
        assertThat(branch(3, "component").isBeforeOrSame(branch(3, "component"))).isTrue();
    }

    @Test
    void isBeforeDifferentCommitId() {
        assertThatThrownBy(() -> {
            OrderedArcRevision one = OrderedArcRevision.fromHash("b1", ArcBranch.trunk(), 1, 0);
            OrderedArcRevision two = OrderedArcRevision.fromHash("b2", ArcBranch.trunk(), 1, 0);
            one.isBefore(two);
        }).hasMessageContaining("have same numbers, but different ids");
    }

    @Test
    void isBeforeOrSameDifferentCommitId() {
        assertThatThrownBy(() -> {
            OrderedArcRevision one = OrderedArcRevision.fromHash("b1", ArcBranch.trunk(), 1, 0);
            OrderedArcRevision two = OrderedArcRevision.fromHash("b2", ArcBranch.trunk(), 1, 0);
            one.isBeforeOrSame(two);
        }).hasMessageContaining("have same numbers, but different ids");
    }

    @Test
    void isBeforeOrSameDifferentBranches() {
        assertThatThrownBy(() -> trunk(1).isBeforeOrSame(branch(2, "release")))
                .hasMessageContaining("Cannot compare revisions from different branches");

        assertThatThrownBy(() -> branch(1, "user").isBeforeOrSame(trunk(2)))
                .hasMessageContaining("Cannot compare revisions from different branches");

        assertThatThrownBy(() -> branch(1, "user").isBeforeOrSame(branch(1, "release")))
                .hasMessageContaining("Cannot compare revisions from different branches");
    }

    private OrderedArcRevision trunk(int number) {
        return OrderedArcRevision.fromHash(Integer.toHexString(number), ArcBranch.trunk(), number, 0);
    }

    private OrderedArcRevision branch(int number, String branchName) {
        return OrderedArcRevision.fromHash(Integer.toHexString(number), ArcBranch.ofBranchName(branchName), number, 0);
    }
}
