package ru.yandex.ci.engine.discovery.task;

import java.util.Optional;

import javax.annotation.Nullable;

import org.junit.jupiter.api.Test;
import org.mockito.Mockito;
import org.springframework.boot.test.mock.mockito.SpyBean;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.engine.discovery.arc_reflog.ProcessArcReflogRecordTask;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.isA;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

class ProcessArcReflogRecordTaskTest extends EngineTestBase {

    @SpyBean
    private ArcService arcServiceSpy;

    @Test
    void trunkCommits() {
        bazingaTaskManagerStub.schedule(new ProcessArcReflogRecordTask(
                ArcBranch.trunk(), TestData.TRUNK_R9.toRevision(), TestData.TRUNK_R5.toRevision()
        ));
        executeBazingaTasks(ProcessArcReflogRecordTask.class);

        assertThat(bazingaTaskManagerStub.getJobsParameters(ProcessPostCommitTask.class))
                .containsExactlyInAnyOrder(
                        new ProcessPostCommitTask.Params(ArcBranch.trunk(), TestData.TRUNK_R9.toRevision()),
                        new ProcessPostCommitTask.Params(ArcBranch.trunk(), TestData.TRUNK_R8.toRevision()),
                        new ProcessPostCommitTask.Params(ArcBranch.trunk(), TestData.TRUNK_R7.toRevision()),
                        new ProcessPostCommitTask.Params(ArcBranch.trunk(), TestData.TRUNK_R6.toRevision())
                );
    }

    @Test
    void withMergeBase() {
        mockMergeBase(TestData.TRUNK_R6.toRevision());

        bazingaTaskManagerStub.schedule(new ProcessArcReflogRecordTask(
                TestData.RELEASE_BRANCH_2, TestData.RELEASE_R6_4.toRevision(), TestData.RELEASE_R6_1.toRevision()
        ));

        executeBazingaTasks(ProcessArcReflogRecordTask.class);

        assertThat(bazingaTaskManagerStub.getJobsParameters(ProcessPostCommitTask.class))
                .containsExactlyInAnyOrder(
                        new ProcessPostCommitTask.Params(TestData.RELEASE_BRANCH_2, TestData.RELEASE_R6_4.toRevision()),
                        new ProcessPostCommitTask.Params(TestData.RELEASE_BRANCH_2, TestData.RELEASE_R6_3.toRevision()),
                        new ProcessPostCommitTask.Params(TestData.RELEASE_BRANCH_2, TestData.RELEASE_R6_2.toRevision())
                );
    }

    @Test
    void withoutMergeBase() {
        mockMergeBase(null);

        bazingaTaskManagerStub.schedule(new ProcessArcReflogRecordTask(
                TestData.RELEASE_BRANCH_2, TestData.RELEASE_R6_4.toRevision(), TestData.RELEASE_R6_1.toRevision()
        ));

        Mockito.reset(bazingaTaskManagerStub);
        executeBazingaTasks(ProcessArcReflogRecordTask.class);

        verify(bazingaTaskManagerStub, never()).schedule(isA(ProcessPostCommitTask.class));
    }

    @Test
    void withMergeBaseNotInTrunk() {
        var commitId = ArcRevision.of("r2-1");
        mockMergeBase(commitId);
        assertThat(arcService.getCommit(commitId).isTrunk()).isFalse();

        bazingaTaskManagerStub.schedule(new ProcessArcReflogRecordTask(
                TestData.RELEASE_BRANCH_2, TestData.RELEASE_R6_4.toRevision(), TestData.RELEASE_R6_1.toRevision()
        ));

        executeBazingaTasks(ProcessArcReflogRecordTask.class);

        verify(bazingaTaskManagerStub, never()).schedule(isA(ProcessPostCommitTask.class));
    }

    @Test
    void dontCalculateMergeBaseAgain() {
        mockMergeBase(null);

        for (int i = 0; i < 3; i++) {
            bazingaTaskManagerStub.schedule(new ProcessArcReflogRecordTask(
                    TestData.RELEASE_BRANCH_2, TestData.RELEASE_R6_4.toRevision(), TestData.RELEASE_R6_1.toRevision()
            ));
        }
    }

    private void mockMergeBase(@Nullable ArcRevision mergeBase) {
        doReturn(TestData.TRUNK_R10.toRevision())
                .when(arcServiceSpy).getLastRevisionInBranch(ArcBranch.trunk());

        doReturn(TestData.RELEASE_R6_4.toRevision())
                .when(arcServiceSpy).getLastRevisionInBranch(TestData.RELEASE_BRANCH_2);

        doReturn(Optional.ofNullable(mergeBase))
                .when(arcServiceSpy).getMergeBase(TestData.TRUNK_R10.toRevision(), TestData.RELEASE_R6_4.toRevision());
    }
}
