package ru.yandex.ci.core.arc;

import java.util.Collection;
import java.util.List;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.CommonYdbTestBase;
import ru.yandex.ci.core.test.TestData;

import static org.assertj.core.api.Assertions.assertThat;

class ArcCommitTableTest extends CommonYdbTestBase {

    @Test
    void upsertCommits() {
        db.currentOrTx(() ->
                db.arcCommit().save(List.of(TestData.TRUNK_COMMIT_1, TestData.TRUNK_COMMIT_2)));

        assertThat(getCommit(TestData.TRUNK_COMMIT_1.getId()))
                .isEqualTo(TestData.TRUNK_COMMIT_1);

        assertThat(getCommit(TestData.TRUNK_COMMIT_2.getId()))
                .isEqualTo(TestData.TRUNK_COMMIT_2);
    }

    @Test
    void upsertCommitsWithNullValues() {
        db.currentOrTx(() ->
                db.arcCommit().save(TestData.TRUNK_COMMIT_1));
        assertThat(getCommit(TestData.TRUNK_COMMIT_1.getId()))
                .isEqualTo(TestData.TRUNK_COMMIT_1);
    }

    @Test
    void getCommits() {
        List<ArcCommit> result = find(List.of(TestData.TRUNK_COMMIT_1.getRevision()));
        assertThat(result).isEmpty();

        db.currentOrTx(() ->
                db.arcCommit().save(TestData.TRUNK_COMMIT_1));
        result = find(List.of(TestData.TRUNK_COMMIT_1.getRevision()));
        assertThat(result).isEqualTo(List.of(TestData.TRUNK_COMMIT_1));
    }

    @Test
    void getCommits_whenArgumentsAreEmpty() {
        List<ArcCommit> result = find(List.of());
        assertThat(result).isEmpty();
    }

    private ArcCommit getCommit(ArcCommit.Id commitId) {
        return db.currentOrReadOnly(() -> db.arcCommit().get(commitId));
    }

    private List<ArcCommit> find(Collection<ArcRevision> revisions) {
        return db.currentOrReadOnly(() -> db.arcCommit().findByRevisions(revisions));
    }

}
