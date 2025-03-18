package ru.yandex.ci.tms.test;

import java.util.List;
import java.util.Set;

import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.core.arc.branch.Branch;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchDisplacedBy;
import ru.yandex.ci.core.launch.LaunchDisplacementChange;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.launch.LaunchState.Status;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.flow.YavDelegationException;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.model.StagedLaunchStatistics;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;

import static org.assertj.core.api.Assertions.assertThat;

public class EntireTestDisplacement extends AbstractEntireTest {

    @Test
    void woodcutterNoDisplacement() throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2, TestData.TRUNK_COMMIT_3, TestData.TRUNK_COMMIT_4,
                TestData.TRUNK_COMMIT_5, TestData.TRUNK_COMMIT_6);

        var processId = TestData.SAWMILL_RELEASE_NO_DISPLACEMENT_PROCESS_ID;
        engineTester.delegateToken(processId.getPath());

        Launch launchR1 = launch(processId, TestData.TRUNK_R2);

        engineTester.waitJob(launchR1.getLaunchId(), WAIT, "sawmill-join", StatusChangeType.SUCCESSFUL);
        engineTester.waitLaunch(launchR1.getLaunchId(), WAIT, Status.WAITING_FOR_MANUAL_TRIGGER);

        // Вытеснения нет
        Launch launchR5 = launch(processId, TestData.TRUNK_R6);
        engineTester.waitLaunch(launchR5.getLaunchId(), WAIT, Status.WAITING_FOR_STAGE);

        // Осталось без изменений
        engineTester.waitLaunch(launchR1.getLaunchId(), WAIT, Status.WAITING_FOR_MANUAL_TRIGGER);
    }

    @Test
    void woodcutterDisplacement() throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2, TestData.TRUNK_COMMIT_3, TestData.TRUNK_COMMIT_4,
                TestData.TRUNK_COMMIT_5, TestData.TRUNK_COMMIT_6);

        var processId = TestData.SAWMILL_RELEASE_DISPLACEMENT_PROCESS_ID;
        engineTester.delegateToken(processId.getPath());

        Launch launchR1 = launch(processId, TestData.TRUNK_R2);

        engineTester.waitJob(launchR1.getLaunchId(), WAIT, "sawmill-join", StatusChangeType.SUCCESSFUL);
        engineTester.waitLaunch(launchR1.getLaunchId(), WAIT, Status.WAITING_FOR_MANUAL_TRIGGER);

        // Проверим вытеснение
        Launch launchR5 = launch(processId, TestData.TRUNK_R6);
        engineTester.waitJob(launchR5.getLaunchId(), WAIT, "sawmill-join", StatusChangeType.SUCCESSFUL);
        engineTester.waitLaunch(launchR5.getLaunchId(), WAIT, Status.WAITING_FOR_MANUAL_TRIGGER);

        // Вытеснили предыдущее состояние
        var launchR1Complete = engineTester.waitLaunch(launchR1.getLaunchId(), WAIT, Status.CANCELED);
        assertThat(launchR1Complete.getCancelledReason())
                .isEqualTo("Displaced automatically by #2");
        assertThat(launchR1Complete.isDisplaced()).isTrue();
        assertThat(launchR1Complete.getDisplacedBy())
                .isEqualTo(
                        LaunchDisplacedBy.of(launchR5.getLaunchId().toKey(), launchR5.getVersion())
                );


        var flowLaunchR1 = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launchR1Complete.getLaunchId()));
        assertThat(flowLaunchR1.isDisabled()).isTrue();

        assertThat(launchR5.isDisplaced()).isFalse();
        assertThat(launchR5.getDisplacedBy()).isNull();

        var stats = StagedLaunchStatistics.fromLaunch(flowLaunchR1);
        assertThat(stats).isNotNull();

        assertThat(stats.getStats().keySet())
                .isEqualTo(Set.of("start", "prepare-wood", "wait-stage", "build-furniture"));
    }

    @Test
    void woodcutterPreventDisplacement() throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2, TestData.TRUNK_COMMIT_3, TestData.TRUNK_COMMIT_4,
                TestData.TRUNK_COMMIT_5, TestData.TRUNK_COMMIT_6);

        var processId = TestData.SAWMILL_RELEASE_DISPLACEMENT_PROCESS_ID;
        engineTester.delegateToken(processId.getPath());

        Launch launchR1 = launch(processId, TestData.TRUNK_R2);

        engineTester.waitJob(launchR1.getLaunchId(), WAIT, "sawmill-join", StatusChangeType.SUCCESSFUL);
        engineTester.waitLaunch(launchR1.getLaunchId(), WAIT, Status.WAITING_FOR_MANUAL_TRIGGER);

        // Запретим вытеснение
        var change = launchService.changeLaunchDisplacementState(launchR1.getLaunchId(),
                Common.DisplacementState.DS_DENY, "username");

        // Настройки сохранены
        var changedAt = change.getDisplacementChanges().get(0).getChangedAt();
        assertThat(change.getDisplacementChanges())
                .isEqualTo(List.of(
                        LaunchDisplacementChange.of(Common.DisplacementState.DS_DENY, "username", changedAt)));

        // Проверим, что мы ничего не вытеснили
        Launch launchR5 = launch(processId, TestData.TRUNK_R6);
        engineTester.waitLaunch(launchR5.getLaunchId(), WAIT, Status.WAITING_FOR_STAGE);

        // Осталось без изменений
        engineTester.waitLaunch(launchR1.getLaunchId(), WAIT, Status.WAITING_FOR_MANUAL_TRIGGER);

    }

    @Test
    void woodcutterPreventAndAllowDisplacement() throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2, TestData.TRUNK_COMMIT_3, TestData.TRUNK_COMMIT_4,
                TestData.TRUNK_COMMIT_5, TestData.TRUNK_COMMIT_6);

        engineTester.delegateToken(TestData.SAWMILL_RELEASE_DISPLACEMENT_PROCESS_ID.getPath());

        Launch launchR1 = launch(TestData.SAWMILL_RELEASE_DISPLACEMENT_PROCESS_ID, TestData.TRUNK_R2);

        engineTester.waitJob(launchR1.getLaunchId(), WAIT, "sawmill-join", StatusChangeType.SUCCESSFUL);
        engineTester.waitLaunch(launchR1.getLaunchId(), WAIT, Status.WAITING_FOR_MANUAL_TRIGGER);

        // Запретим вытеснение
        var changeDeny = launchService.changeLaunchDisplacementState(launchR1.getLaunchId(),
                Common.DisplacementState.DS_DENY, "username1");

        // Настройки сохранены
        var changedDenyAt = changeDeny.getDisplacementChanges().get(0).getChangedAt();
        assertThat(changeDeny.getDisplacementChanges())
                .isEqualTo(List.of(
                        LaunchDisplacementChange.of(Common.DisplacementState.DS_DENY, "username1", changedDenyAt)));

        // Проверим, что мы ничего не вытеснили
        Launch launchR5 = launch(TestData.SAWMILL_RELEASE_DISPLACEMENT_PROCESS_ID, TestData.TRUNK_R6);
        engineTester.waitLaunch(launchR5.getLaunchId(), WAIT, Status.WAITING_FOR_STAGE);

        // Снова разрешим
        var changeAllow = launchService.changeLaunchDisplacementState(launchR1.getLaunchId(),
                Common.DisplacementState.DS_ALLOW, "username2");

        var changedAllowAt = changeAllow.getDisplacementChanges().get(1).getChangedAt();
        assertThat(changeAllow.getDisplacementChanges())
                .isEqualTo(List.of(
                        LaunchDisplacementChange.of(Common.DisplacementState.DS_DENY, "username1", changedDenyAt),
                        LaunchDisplacementChange.of(Common.DisplacementState.DS_ALLOW, "username2", changedAllowAt)));

        // Проверим вытеснение, которое снова должно сработать
        engineTester.waitJob(launchR5.getLaunchId(), WAIT, "sawmill-join", StatusChangeType.SUCCESSFUL);
        engineTester.waitLaunch(launchR5.getLaunchId(), WAIT, Status.WAITING_FOR_MANUAL_TRIGGER);

        // Вытеснили предыдущее состояние
        var launchR1Complete = engineTester.waitLaunch(launchR1.getLaunchId(), WAIT, Status.CANCELED);
        assertThat(launchR1Complete.getCancelledReason())
                .isEqualTo("Displaced automatically by #2");

        var flowLaunchR1 = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launchR1Complete.getLaunchId()));
        assertThat(flowLaunchR1.isDisabled()).isTrue();
    }

    @Test
    @Disabled("Unstable test, need to figure out why")
    void woodcutterDisplacementParallel() throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2, TestData.TRUNK_COMMIT_3, TestData.TRUNK_COMMIT_4,
                TestData.TRUNK_COMMIT_5, TestData.TRUNK_COMMIT_6);

        engineTester.delegateToken(TestData.SAWMILL_RELEASE_DISPLACEMENT_PROCESS_ID.getPath());

        Launch launchR1 = launch(TestData.SAWMILL_RELEASE_DISPLACEMENT_PROCESS_ID, TestData.TRUNK_R2);
        Launch launchR5 = launch(TestData.SAWMILL_RELEASE_DISPLACEMENT_PROCESS_ID, TestData.TRUNK_R6);

        // Проверим вытеснение при параллельном запуске
        engineTester.waitJob(launchR5.getLaunchId(), WAIT, "sawmill-join", StatusChangeType.SUCCESSFUL);
        engineTester.waitLaunch(launchR5.getLaunchId(), WAIT, Status.WAITING_FOR_MANUAL_TRIGGER);

        // Вытеснили предыдущее состояние
        var launchR1Complete = engineTester.waitLaunch(launchR1.getLaunchId(), WAIT, Status.CANCELED);

        var flowLaunchR1 = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launchR1Complete.getLaunchId()));
        assertThat(flowLaunchR1.isDisabled()).isTrue();
    }


    @Test
    void woodcutterDisplacementDifferentBranches() throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2, TestData.TRUNK_COMMIT_3, TestData.TRUNK_COMMIT_4,
                TestData.TRUNK_COMMIT_5, TestData.TRUNK_COMMIT_6);

        engineTester.delegateToken(TestData.SAWMILL_RELEASE_DISPLACEMENT_PROCESS_ID.getPath());

        Launch launchR5 = launch(TestData.SAWMILL_RELEASE_DISPLACEMENT_PROCESS_ID, TestData.TRUNK_R6);

        Branch branchR5 = db.currentOrTx(() ->
                branchService.createBranch(TestData.SAWMILL_RELEASE_DISPLACEMENT_PROCESS_ID, TestData.TRUNK_R6,
                        TestData.CI_USER));

        processCommits(
                branchR5.getArcBranch(),
                TestData.RELEASE_BRANCH_COMMIT_6_1,
                TestData.RELEASE_BRANCH_COMMIT_6_2
        );

        engineTester.delegateToken(TestData.SAWMILL_RELEASE_DISPLACEMENT_PROCESS_ID.getPath(), branchR5.getArcBranch());

        Launch launchR5n2 = launch(TestData.SAWMILL_RELEASE_DISPLACEMENT_PROCESS_ID, TestData.RELEASE_R6_2);

        engineTester.waitJob(launchR5.getLaunchId(), WAIT, "sawmill-join", StatusChangeType.SUCCESSFUL);
        var launchR5Complete =
                engineTester.waitLaunch(launchR5.getLaunchId(), WAIT, LaunchState.Status.WAITING_FOR_MANUAL_TRIGGER);

        engineTester.waitJob(launchR5n2.getLaunchId(), WAIT, "sawmill-join", StatusChangeType.SUCCESSFUL);
        var launchR5n2Complete =
                engineTester.waitLaunch(launchR5n2.getLaunchId(), WAIT, LaunchState.Status.WAITING_FOR_MANUAL_TRIGGER);

        var flowLaunchR5 =
                flowTestQueries.getFlowLaunch(FlowLaunchId.of(launchR5Complete.getLaunchId()));
        var flowLaunchR5n2 =
                flowTestQueries.getFlowLaunch(FlowLaunchId.of(launchR5n2Complete.getLaunchId()));
        assertThat(flowLaunchR5.isDisabled()).isFalse();
        assertThat(flowLaunchR5n2.isDisabled()).isFalse();
    }

}
