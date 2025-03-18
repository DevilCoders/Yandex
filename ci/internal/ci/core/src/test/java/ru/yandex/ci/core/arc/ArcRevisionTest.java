package ru.yandex.ci.core.arc;

import org.junit.jupiter.api.Test;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;

public class ArcRevisionTest {
    @Test
    void isSvnRevision() {
        assertThat(ArcRevision.isSvnRevision("a".repeat(40))).isFalse();
        assertThat(ArcRevision.isSvnRevision("1".repeat(40))).isFalse();
        assertThat(ArcRevision.isSvnRevision(Long.toString(Long.MAX_VALUE))).isTrue();
        assertThat(ArcRevision.isSvnRevision("1")).isTrue();
    }

    @Test
    void isArcRevision() {
        assertThat(ArcRevision.isArcRevision("a".repeat(40))).isTrue();
        assertThat(ArcRevision.isArcRevision("1".repeat(40))).isTrue();
        assertThat(ArcRevision.isArcRevision("r" + Long.MAX_VALUE)).isTrue();
        assertThat(ArcRevision.isArcRevision("r1")).isTrue();
        assertThat(ArcRevision.isArcRevision(Long.toString(Long.MAX_VALUE))).isFalse();
        assertThat(ArcRevision.isArcRevision("1")).isFalse();
        assertThat(ArcRevision.isArcRevision("trunk")).isFalse();
    }

    @Test
    void correctRevisionFormat() {
        assertThat(ArcRevision.parse("a".repeat(40)).getCommitId())
                .isEqualTo("a".repeat(40));
        assertThat(ArcRevision.parse("1".repeat(40)).getCommitId())
                .isEqualTo("1".repeat(40));
        assertThat(ArcRevision.parse("r" + Long.MAX_VALUE).getCommitId())
                .isEqualTo("r" + Long.MAX_VALUE);
        assertThat(ArcRevision.parse("r1").getCommitId())
                .isEqualTo("r1");
        assertThat(ArcRevision.parse(Long.toString(Long.MAX_VALUE)).getCommitId())
                .isEqualTo("r" + Long.MAX_VALUE);
        assertThat(ArcRevision.parse("1").getCommitId())
                .isEqualTo("r1");
        assertThatThrownBy(() -> ArcRevision.parse("trunk"))
                .isInstanceOf(BadRevisionFormatException.class);
    }
}
