package ru.yandex.ci.tms.test;

import java.nio.file.Path;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.AYamlService;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.engine.flow.YavDelegationException;

import static org.assertj.core.api.Assertions.assertThat;
import static ru.yandex.ci.core.test.TestData.TRUNK_COMMIT_2;
import static ru.yandex.ci.core.test.TestData.TRUNK_COMMIT_3;

public class AutoReleaseEntireTest extends AbstractEntireTest {
    private static final String ROBOT_CI = "robot-ci";
    private static final CiProcessId PROCESS_ID = CiProcessId.ofRelease(Path.of("a.yaml"), "auto-release");
    private static final LaunchId LAUNCH_1 = LaunchId.of(PROCESS_ID, 1);
    private static final LaunchId LAUNCH_2 = LaunchId.of(PROCESS_ID, 2);

    @Autowired
    private AYamlService aYamlService;

    @Override
    @BeforeEach
    public void setUp() {
        super.setUp();
        aYamlService.reset();
    }

    @Test
    void simple() throws InterruptedException, YavDelegationException {
        arcServiceStub.initRepo("test-repos/auto-release/simple", TRUNK_COMMIT_2);

        processCommits(TRUNK_COMMIT_2);
        engineTester.delegateToken(PROCESS_ID.getPath());

        var launch = engineTester.waitLaunch(LAUNCH_1, WAIT);
        assertThat(launch.getTriggeredBy()).isEqualTo(ROBOT_CI);
        assertIsProcessingOrSuccess(launch.getStatus());
    }

    @Test
    void waitStage() throws InterruptedException, YavDelegationException {
        arcServiceStub.initRepo("test-repos/auto-release/wait-stage", TRUNK_COMMIT_2, TRUNK_COMMIT_3);

        processCommits(TRUNK_COMMIT_2);
        engineTester.delegateToken(PROCESS_ID.getPath());

        var launch1 = engineTester.waitLaunch(LAUNCH_1, WAIT, LaunchState.Status.WAITING_FOR_MANUAL_TRIGGER);
        processCommits(TRUNK_COMMIT_3);
        disableManualSwitch(launch1, "second-task");

        var launch2 = engineTester.waitLaunchAnyStatus(LAUNCH_2, WAIT);
        assertThat(launch2.getVcsInfo().getCommit()).isEqualTo(TRUNK_COMMIT_3);
    }

    @Test
    void createBranch() throws InterruptedException, YavDelegationException {
        arcServiceStub.initRepo("test-repos/auto-release/create-branch", TRUNK_COMMIT_2);

        processCommits(TRUNK_COMMIT_2);
        engineTester.delegateToken(PROCESS_ID.getPath());

        var launch = engineTester.waitLaunchAnyStatus(LAUNCH_1, WAIT);
        assertThat(launch.getVcsInfo().getSelectedBranch().getBranch())
                .isEqualTo("releases/auto-release/ver-1");
    }

    private static void assertIsProcessingOrSuccess(LaunchState.Status status) {
        assertThat(status.isProcessing() || status == LaunchState.Status.SUCCESS)
                .describedAs("Expected processing status or SUCCESS, but status: %s", status)
                .isTrue();
    }

}
