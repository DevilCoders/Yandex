package ru.yandex.ci.core.pr;

import java.util.List;
import java.util.Optional;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.CommonYdbTestBase;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.test.TestData;

import static org.assertj.core.api.Assertions.assertThat;

class PullRequestDiffSetTableTest extends CommonYdbTestBase {

    @Test
    void testFlow() {
        PullRequest pullRequest = new PullRequest(42, "andreevdm", "CI-4242 make ci great again", "Description");

        PullRequestVcsInfo vcsInfo = new PullRequestVcsInfo(
                TestData.REVISION,
                TestData.SECOND_REVISION,
                ArcBranch.trunk(),
                TestData.THIRD_REVISION,
                TestData.USER_BRANCH
        );

        PullRequestDiffSet diffSet = new PullRequestDiffSet(
                PullRequestDiffSet.Id.of(pullRequest.getId(), 21),
                pullRequest.getAuthor(),
                pullRequest.getSummary(),
                pullRequest.getDescription(),
                vcsInfo,
                null,
                List.of("CI-1", "CI-2"),
                PullRequestDiffSet.Status.NEW,
                false,
                List.of("label1", "label2"),
                null,
                null
        );
        db.currentOrTx(() -> db.pullRequestDiffSetTable().save(diffSet));
        db.currentOrTx(() -> {
            PullRequestDiffSet result = db.pullRequestDiffSetTable().getById(42, 21);
            assertThat(diffSet).isEqualTo(result);
        });
    }

    @Test
    void suggestPullRequestId() {
        List.of(
                createPullRequestDiffSet(710),
                createPullRequestDiffSet(711),
                createPullRequestDiffSet(720),

                createPullRequestDiffSet(7_100),
                createPullRequestDiffSet(7_101),
                createPullRequestDiffSet(7_200),

                createPullRequestDiffSet(7_100_000),
                createPullRequestDiffSet(7_100_001),
                createPullRequestDiffSet(7_200_000)
        ).forEach(pr -> db.currentOrTx(() -> db.pullRequestDiffSetTable().save(pr)));

        db.currentOrReadOnly(() -> {
            assertThat(db.pullRequestDiffSetTable().suggestPullRequestId(null, -1, -1))
                    .containsExactly(
                            710L, 711L, 720L,
                            7_100L, 7_101L, 7_200L,
                            7_100_000L, 7_100_001L, 7_200_000L
                    );
            assertThat(db.pullRequestDiffSetTable().suggestPullRequestId(71L, -1, -1))
                    .containsExactly(
                            710L, 711L,
                            7_100L, 7_101L,
                            7_100_000L, 7_100_001L
                    );
            assertThat(db.pullRequestDiffSetTable().suggestPullRequestId(71L, 1, 2))
                    .containsExactly(
                            711L,
                            7_100L
                    );
            assertThat(db.pullRequestDiffSetTable().suggestPullRequestId(710L, -1, -1))
                    .containsExactly(
                            710L,
                            7_100L, 7_101L,
                            7_100_000L, 7_100_001L
                    );
            assertThat(db.pullRequestDiffSetTable().suggestPullRequestId(7_101L, -1, -1))
                    .containsExactly(
                            7_101L
                    );
            assertThat(db.pullRequestDiffSetTable().suggestPullRequestId(8L, -1, -1))
                    .isEmpty();
        });
    }

    @Test
    void findLatestByPullRequestId() {
        List.of(
                createPullRequestDiffSet(710, 21),
                createPullRequestDiffSet(710, 22),
                createPullRequestDiffSet(711, 2),
                createPullRequestDiffSet(711, 1)
        ).forEach(pr -> db.currentOrTx(() -> db.pullRequestDiffSetTable().save(pr)));

        db.currentOrReadOnly(() -> {
            assertThat(db.pullRequestDiffSetTable().findLatestByPullRequestId(710).orElseThrow())
                    .extracting(PullRequestDiffSet::getId)
                    .matches(id -> id.getPullRequestId() == 710 && id.getDiffSetId() == 22);
            assertThat(db.pullRequestDiffSetTable().findLatestByPullRequestId(711).orElseThrow())
                    .extracting(PullRequestDiffSet::getId)
                    .matches(id -> id.getPullRequestId() == 711 && id.getDiffSetId() == 2);
            assertThat(db.pullRequestDiffSetTable().findLatestByPullRequestId(712))
                    .isEqualTo(Optional.empty());
        });
    }

    private static PullRequestDiffSet createPullRequestDiffSet(long pullRequestId) {
        return createPullRequestDiffSet(pullRequestId, 21);
    }

    private static PullRequestDiffSet createPullRequestDiffSet(long pullRequestId, long diffSetId) {
        PullRequest pullRequest = new PullRequest(pullRequestId, "andreevdm", "CI-4242 make ci great again", null);

        PullRequestVcsInfo vcsInfo = new PullRequestVcsInfo(
                TestData.REVISION,
                TestData.SECOND_REVISION,
                ArcBranch.trunk(),
                TestData.THIRD_REVISION,
                TestData.USER_BRANCH
        );

        return new PullRequestDiffSet(
                PullRequestDiffSet.Id.of(pullRequest.getId(), diffSetId),
                pullRequest.getAuthor(),
                pullRequest.getSummary(),
                pullRequest.getDescription(),
                vcsInfo,
                null,
                List.of("CI-1", "CI-2"),
                PullRequestDiffSet.Status.NEW,
                false,
                List.of("label1", "label2"),
                null,
                null
        );
    }

}
