package ru.yandex.ci.tms.task.release;

import java.nio.file.Path;
import java.time.Duration;
import java.util.List;

import com.fasterxml.jackson.core.JsonProcessingException;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.test.TestUtils;
import ru.yandex.ci.tms.client.SolomonClient;
import ru.yandex.ci.tms.client.SolomonMetrics;
import ru.yandex.ci.tms.spring.CiTmsPropertiesTestConfig;
import ru.yandex.ci.tms.spring.tasks.CiTaskConfig;
import ru.yandex.ci.tms.spring.tasks.autocheck.degradation.JugglerClientTestConfig;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.atLeastOnce;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static ru.yandex.ci.core.launch.LaunchState.Status.CANCELED;
import static ru.yandex.ci.core.launch.LaunchState.Status.CLEANING;
import static ru.yandex.ci.core.launch.LaunchState.Status.IDLE;
import static ru.yandex.ci.core.launch.LaunchState.Status.SUCCESS;
import static ru.yandex.ci.tms.task.release.ReleaseStatusesToJugglerPushCronTaskTest.PATH_ANNA;
import static ru.yandex.ci.tms.task.release.ReleaseStatusesToJugglerPushCronTaskTest.PATH_JACK;
import static ru.yandex.ci.tms.task.release.ReleaseStatusesToJugglerPushCronTaskTest.PATH_MIKE;
import static ru.yandex.ci.tms.task.release.ReleaseStatusesToJugglerPushCronTaskTest.PROJECT_BLUE;
import static ru.yandex.ci.tms.task.release.ReleaseStatusesToJugglerPushCronTaskTest.PROJECT_PINK;
import static ru.yandex.ci.tms.task.release.ReleaseStatusesToJugglerPushCronTaskTest.RELEASE_FAST;
import static ru.yandex.ci.tms.task.release.ReleaseStatusesToJugglerPushCronTaskTest.RELEASE_HIGH;
import static ru.yandex.ci.tms.task.release.ReleaseStatusesToJugglerPushCronTaskTest.RELEASE_NICE;
import static ru.yandex.ci.tms.task.release.ReleaseStatusesToJugglerPushCronTaskTest.configState;

@ContextConfiguration(classes = {
        CiTaskConfig.class,
        JugglerClientTestConfig.class,
        CiTmsPropertiesTestConfig.class
})
class CompleteReleaseToSolomonCronTaskTest extends EngineTestBase {

    @Autowired
    private CompleteReleaseToSolomonCronTask task;

    @MockBean
    private SolomonClient solomonClient;

    @Mock
    private ExecutionContext context;

    @Captor
    private ArgumentCaptor<SolomonMetrics> metrics;

    @BeforeEach
    public void setUp() {
        when(solomonClient.push(any(), any()))
                .thenReturn(new SolomonClient.PushResponse(null, -1, SolomonClient.PushStatus.OK));
    }

    @Test
    void multipleConfigsAndProcesses() throws JsonProcessingException {
        saveConfigs(
                configState(PROJECT_BLUE, PATH_MIKE, List.of(RELEASE_FAST, RELEASE_NICE)),
                configState(PROJECT_BLUE, PATH_ANNA, List.of(RELEASE_FAST)),
                configState(PROJECT_PINK, PATH_JACK, List.of(RELEASE_FAST, RELEASE_HIGH))
        );
        saveLaunches(
                launch(PROJECT_BLUE, PATH_MIKE, RELEASE_FAST, 1, SUCCESS),

                launch(PROJECT_BLUE, PATH_MIKE, RELEASE_NICE, 1, CANCELED),
                launch(PROJECT_BLUE, PATH_MIKE, RELEASE_NICE, 2, SUCCESS),

                launch(PROJECT_BLUE, PATH_ANNA, RELEASE_FAST, 1, SUCCESS),
                launch(PROJECT_BLUE, PATH_ANNA, RELEASE_FAST, 2, CANCELED),

                launch(PROJECT_PINK, PATH_JACK, RELEASE_FAST, 1, IDLE),
                launch(PROJECT_PINK, PATH_JACK, RELEASE_HIGH, 2, CLEANING)
        );

        task.executeImpl(context);

        verify(solomonClient, atLeastOnce()).push(any(), metrics.capture());

        assertThat(TestUtils.readJson(metrics.getValue()))
                .isEqualTo(TestUtils.readJson("multipleConfigsAndProcesses.json"));

    }

    private void saveConfigs(ConfigState... configStates) {
        db.currentOrTx(() -> db.configStates().save(List.of(configStates)));
    }

    private void saveLaunches(Launch... launches) {
        saveLaunches(List.of(launches));
    }

    private void saveLaunches(List<Launch> launches) {
        db.currentOrTx(() -> db.launches().save(launches));
    }

    private Launch launch(
            String projectId,
            Path configPath,
            String releaseId,
            int launchNumber,
            LaunchState.Status status
    ) {
        clock.plus(Duration.ofMinutes(1));
        return TestData.launchBuilder()
                .launchId(LaunchId.of(CiProcessId.ofRelease(configPath, releaseId), launchNumber))
                .version(Version.major(String.valueOf(launchNumber)))
                .project(projectId)
                .status(status)
                .finished(clock.instant().minusSeconds(335))
                .build();
    }
}
