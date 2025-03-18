package ru.yandex.ci.tms.test;

import java.util.List;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.launch.LaunchState.Status;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.flow.YavDelegationException;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChangeType;
import ru.yandex.ci.tms.test.woodflow.FurnitureFactoryStub;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.spy;

public class EntireTestHotfixes extends AbstractEntireTest {

    @Test
    void simplestWithCustomFlow() throws YavDelegationException, InterruptedException {
        var taskletSpy = spy(new FurnitureFactoryStub());
        sandboxClientStub.uploadTasklet(TaskletResources.FURNITURE, taskletSpy);

        processCommits(TestData.TRUNK_COMMIT_2);

        engineTester.delegateToken(TestData.SIMPLEST_RELEASE_PROCESS_ID.getPath());

        var launch = launch(
                TestData.SIMPLEST_RELEASE_PROCESS_ID,
                TestData.TRUNK_R2,
                "simplest-hotfix-flow",
                Common.FlowType.FT_HOTFIX,
                null
        );

        var finalLaunch = engineTester.waitLaunch(launch.getLaunchId(), WAIT, LaunchState.Status.SUCCESS);
        assertThat(finalLaunch.getVersion()).isEqualTo(Version.major("1"));

        var resources =
                flowTestQueries.getProducedResources(FlowLaunchId.of(launch.getLaunchId()), "furniture-factory");

        var furnitures = getFurniture(resources);
        assertThat(furnitures).isEqualTo(
                List.of(
                        "Шкаф из 3 досок, полученных из материала 'бревно из липы, hotfix', " +
                                "произведенного [ИП Иванов HOTFIX 3, ОАО Липа не липа hotfix 2, ООО Пилорама hotfix 1]"
                ));
    }

    @Test
    void woodcutterHotfix() throws YavDelegationException, InterruptedException {
        // Build 1, start -> prepare-wood -> wait-stage
        // Build 2, start -> prepare-wood, waiting for wait-stage
        // Hotfix 1, dummy, waiting for prepare-wood
        // Hotfix 2, waiting for dummy
        processCommits(TestData.TRUNK_COMMIT_2, TestData.TRUNK_COMMIT_3, TestData.TRUNK_COMMIT_4,
                TestData.TRUNK_COMMIT_5, TestData.TRUNK_COMMIT_6, TestData.TRUNK_COMMIT_7, TestData.TRUNK_COMMIT_8);

        var processId = TestData.SAWMILL_RELEASE_NO_DISPLACEMENT_PROCESS_ID;
        engineTester.delegateToken(processId.getPath());

        var launch1 = launch(processId, TestData.TRUNK_R2);
        engineTester.waitLaunch(launch1.getLaunchId(), WAIT, Status.WAITING_FOR_MANUAL_TRIGGER);

        var launch2 = launch(processId, TestData.TRUNK_R6);
        engineTester.waitLaunch(launch2.getLaunchId(), WAIT, Status.WAITING_FOR_STAGE);
        engineTester.waitJob(launch2.getLaunchId(), WAIT, "sawmill-2", StatusChangeType.SUCCESSFUL);
        engineTester.waitJob(launch2.getLaunchId(), WAIT, "sawmill-join", StatusChangeType.WAITING_FOR_STAGE);

        var launchHotFix1 = launch(
                processId,
                TestData.TRUNK_R7,
                "release-sawmill-hotfix1",
                Common.FlowType.FT_HOTFIX,
                null);
        engineTester.waitLaunch(launchHotFix1.getLaunchId(), WAIT, Status.WAITING_FOR_STAGE);
        engineTester.waitJob(launchHotFix1.getLaunchId(), WAIT, "woodcutter", StatusChangeType.WAITING_FOR_STAGE);

        var launchHotFix2 = launch(
                processId,
                TestData.TRUNK_R8,
                "release-sawmill-hotfix2",
                Common.FlowType.FT_HOTFIX,
                null);
        // Stage won't be skipped
        engineTester.waitLaunch(launchHotFix2.getLaunchId(), WAIT, Status.WAITING_FOR_STAGE);
        engineTester.waitJob(launchHotFix2.getLaunchId(), WAIT, "start-furniture", StatusChangeType.WAITING_FOR_STAGE);


        disableManualSwitch(launch1, "sawmill-wait");
        engineTester.waitLaunch(launch1.getLaunchId(), WAIT, Status.SUCCESS);

        engineTester.waitLaunch(launch2.getLaunchId(), WAIT, Status.WAITING_FOR_MANUAL_TRIGGER);

        disableManualSwitch(launch2, "sawmill-wait");
        engineTester.waitLaunch(launch2.getLaunchId(), WAIT, Status.SUCCESS);

        engineTester.waitLaunch(launchHotFix1.getLaunchId(), WAIT, Status.SUCCESS);

        engineTester.waitLaunch(launchHotFix2.getLaunchId(), WAIT, Status.SUCCESS);
    }

    @Test
    void woodcutterHotfix2() throws YavDelegationException, InterruptedException {
        // Build 1, start -> prepare-wood -> wait-stage
        // Hotfix 1, dummy -> prepare-wood, waiting for wait-stage

        processCommits(TestData.TRUNK_COMMIT_2, TestData.TRUNK_COMMIT_3, TestData.TRUNK_COMMIT_4,
                TestData.TRUNK_COMMIT_5, TestData.TRUNK_COMMIT_6);

        var processId = TestData.SAWMILL_RELEASE_NO_DISPLACEMENT_PROCESS_ID;
        engineTester.delegateToken(processId.getPath());

        var launch1 = launch(processId, TestData.TRUNK_R2);
        engineTester.waitLaunch(launch1.getLaunchId(), WAIT, Status.WAITING_FOR_MANUAL_TRIGGER);

        var launchHotFix1 = launch(
                processId,
                TestData.TRUNK_R6,
                "release-sawmill-hotfix1",
                Common.FlowType.FT_HOTFIX,
                null
        );
        engineTester.waitLaunch(launchHotFix1.getLaunchId(), WAIT, Status.WAITING_FOR_STAGE);
        engineTester.waitJob(launchHotFix1.getLaunchId(), WAIT, "woodcutter", StatusChangeType.SUCCESSFUL);
        engineTester.waitJob(launchHotFix1.getLaunchId(), WAIT, "start-furniture", StatusChangeType.WAITING_FOR_STAGE);

        disableManualSwitch(launch1, "sawmill-wait");
        engineTester.waitLaunch(launch1.getLaunchId(), WAIT, Status.SUCCESS);
        engineTester.waitLaunch(launchHotFix1.getLaunchId(), WAIT, Status.SUCCESS);
    }

    @Test
    void fillsPreviousRevision() throws YavDelegationException, InterruptedException {
        processCommits(TestData.TRUNK_COMMIT_2, TestData.TRUNK_COMMIT_3, TestData.TRUNK_COMMIT_4);

        var processId = TestData.SIMPLEST_RELEASE_PROCESS_ID;
        engineTester.delegateToken(processId.getPath());

        var launch = launch(processId, TestData.TRUNK_R2);
        engineTester.waitLaunch(launch.getLaunchId(), WAIT, Status.SUCCESS);

        var launchHotFix = launch(
                TestData.SIMPLEST_RELEASE_PROCESS_ID,
                TestData.TRUNK_R4,
                "simplest-hotfix-flow",
                Common.FlowType.FT_HOTFIX,
                null
        );

        engineTester.waitLaunch(launchHotFix.getLaunchId(), WAIT, Status.SUCCESS);

        var flowLaunch = flowTestQueries.getFlowLaunch(FlowLaunchId.of(launchHotFix.getLaunchId()));

        assertThat(flowLaunch.getVcsInfo().getPreviousRevision()).isEqualTo(TestData.TRUNK_R2);
    }

}
