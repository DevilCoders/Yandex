package ru.yandex.ci.observer.api.stress_test;

import java.time.Duration;
import java.util.List;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.MockBean;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.observer.api.ObserverApiYdbTestBase;
import ru.yandex.ci.observer.core.db.model.check.CheckEntity;
import ru.yandex.ci.observer.core.db.model.check.CheckTable;
import ru.yandex.ci.observer.core.db.model.stress_test.StressTestUsedCommitEntity;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;

import static org.assertj.core.api.Assertions.assertThat;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doReturn;
import static ru.yandex.ci.core.test.TestData.TRUNK_COMMIT_2;

class StressTestServiceTest extends ObserverApiYdbTestBase {

    private static final ArcCommit MERGE_COMMIT_1 = ArcCommit.builder().createTime(TIME)
            .id(ArcCommit.Id.of("merge_commit_1")).svnRevision(100).build();

    private static final ArcCommit MERGE_COMMIT_2 = ArcCommit.builder().createTime(TIME)
            .id(ArcCommit.Id.of("merge_commit_2")).svnRevision(200).build();

    private static final CheckEntity CHECK_1 = SAMPLE_CHECK
            .withLeft(StorageRevision.from(ArcBranch.trunk().asString(), TRUNK_COMMIT_2))
            .withRight(StorageRevision.from("pr:1234", MERGE_COMMIT_1));

    private static final CheckEntity CHECK_2 = SAMPLE_CHECK
            .withId(CheckEntity.Id.of(2L))
            .withLeft(StorageRevision.from(ArcBranch.trunk().asString(), TRUNK_COMMIT_2))
            .withRight(StorageRevision.from("pr:1234", MERGE_COMMIT_2));

    @Autowired
    StressTestService stressTestService;

    @MockBean
    ArcService arcService;

    @Test
    void getNotUsedRevisions() {
        doReturn(TRUNK_COMMIT_2).when(arcService).getCommit(any());
        db.currentOrTx(() -> db.checks().save(CHECK_1));

        assertThat(
                stressTestService.getNotUsedRevisions(TRUNK_COMMIT_2.getRevision(), Duration.ofHours(1), 1, "FR")
        ).containsExactlyInAnyOrder(
                CheckTable.RevisionsView.of(CHECK_1)
        );
    }

    @Test
    void getNotUsedRevisions_shouldThrowExceptionWhenNotEnoughCommitFound() {
        doReturn(TRUNK_COMMIT_2).when(arcService).getCommit(any());
        assertThrows(
                IllegalArgumentException.class,
                () -> stressTestService.getNotUsedRevisions(TRUNK_COMMIT_2.getRevision(), Duration.ofHours(1), 1, "FR")
        );
    }

    @Test
    void getNotUsedRevisions_whenThereAreUsedRevisions() {
        doReturn(TRUNK_COMMIT_2).when(arcService).getCommit(any());
        db.currentOrTx(() -> {
            db.checks().save(CHECK_1);
            db.checks().save(CHECK_2);
        });

        assertThat(
                stressTestService.getNotUsedRevisions(TRUNK_COMMIT_2.getRevision(), Duration.ofHours(1), 1, "FR")
        ).containsExactlyInAnyOrder(
                CheckTable.RevisionsView.of(CHECK_1)
        );

        db.currentOrTx(() -> db.stressTestUsedCommitTable().save(
                StressTestUsedCommitEntity.builder()
                        .id(StressTestUsedCommitEntity.Id.of("merge_commit_1", "FR", TRUNK_COMMIT_2.getSvnRevision()))
                        .flowLaunchId("flow-launch-id")
                        .build()
        ));

        assertThat(
                stressTestService.getNotUsedRevisions(TRUNK_COMMIT_2.getRevision(), Duration.ofHours(1), 1, "FR")
        ).containsExactlyInAnyOrder(
                CheckTable.RevisionsView.of(CHECK_2)
        );
    }

    @Test
    void findUsedRightRevisions() {
        assertThat(stressTestService.findUsedRightRevisions(List.of(), "FR"))
                .isEmpty();

        var rightRevisions = List.of(
                MERGE_COMMIT_1.getCommitId(),
                MERGE_COMMIT_2.getCommitId()
        );
        assertThat(stressTestService.findUsedRightRevisions(rightRevisions, "FR"))
                .isEmpty();
        assertThat(stressTestService.findUsedRightRevisions(rightRevisions, "FR"))
                .isEmpty();

        var usedRevision = StressTestUsedCommitEntity.builder()
                .id(StressTestUsedCommitEntity.Id.of("merge_commit_1", "FR", TRUNK_COMMIT_2.getSvnRevision()))
                .flowLaunchId("flow-launch-id")
                .build();
        db.currentOrTx(() -> db.stressTestUsedCommitTable().save(usedRevision));

        assertThat(stressTestService.findUsedRightRevisions(rightRevisions, "FR"))
                .containsExactlyInAnyOrder(usedRevision);
    }

}
