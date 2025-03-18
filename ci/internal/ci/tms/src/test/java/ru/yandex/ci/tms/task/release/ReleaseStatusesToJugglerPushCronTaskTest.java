package ru.yandex.ci.tms.task.release;

import java.nio.file.Path;
import java.util.List;
import java.util.Objects;
import java.util.stream.Stream;

import com.fasterxml.jackson.core.type.TypeReference;
import one.util.streamex.EntryStream;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.CsvSource;
import org.junit.jupiter.params.provider.MethodSource;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.client.juggler.JugglerPushClient;
import ru.yandex.ci.client.juggler.model.RawEvent;
import ru.yandex.ci.client.juggler.model.Status;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.project.ReleaseConfigState;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.test.TestUtils;
import ru.yandex.ci.tms.spring.CiTmsPropertiesTestConfig;
import ru.yandex.ci.tms.spring.tasks.CiTaskConfig;
import ru.yandex.ci.tms.spring.tasks.autocheck.degradation.JugglerClientTestConfig;
import ru.yandex.ci.tms.spring.tasks.autocheck.degradation.SolomonClientTestConfig;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static ru.yandex.ci.core.launch.LaunchState.Status.CANCELED;
import static ru.yandex.ci.core.launch.LaunchState.Status.CANCELLING;
import static ru.yandex.ci.core.launch.LaunchState.Status.DELAYED;
import static ru.yandex.ci.core.launch.LaunchState.Status.FAILURE;
import static ru.yandex.ci.core.launch.LaunchState.Status.IDLE;
import static ru.yandex.ci.core.launch.LaunchState.Status.POSTPONE;
import static ru.yandex.ci.core.launch.LaunchState.Status.RUNNING;
import static ru.yandex.ci.core.launch.LaunchState.Status.RUNNING_WITH_ERRORS;
import static ru.yandex.ci.core.launch.LaunchState.Status.STARTING;
import static ru.yandex.ci.core.launch.LaunchState.Status.SUCCESS;
import static ru.yandex.ci.core.launch.LaunchState.Status.WAITING_FOR_CLEANUP;
import static ru.yandex.ci.core.launch.LaunchState.Status.WAITING_FOR_MANUAL_TRIGGER;
import static ru.yandex.ci.core.launch.LaunchState.Status.WAITING_FOR_STAGE;

@ContextConfiguration(classes = {
        CiTaskConfig.class,
        SolomonClientTestConfig.class,
        JugglerClientTestConfig.class,
        CiTmsPropertiesTestConfig.class
})
class ReleaseStatusesToJugglerPushCronTaskTest extends EngineTestBase {

    static final String PROJECT_PINK = "project-pink";
    static final String PROJECT_BLUE = "project-blue";

    static final String RELEASE_FAST = "release-fast";
    static final String RELEASE_HIGH = "release-high";
    static final String RELEASE_NICE = "release-nice";

    static final Path PATH_MIKE = Path.of("mike/a.yaml");
    static final Path PATH_JACK = Path.of("jack/a.yaml");
    static final Path PATH_ANNA = Path.of("anna/a.yaml");

    @MockBean
    private JugglerPushClient jugglerPushClient;

    @Mock
    private ExecutionContext context;

    @Autowired
    private ReleaseStatusesToJugglerPushCronTask task;

    @Captor
    private ArgumentCaptor<List<RawEvent>> pushedEvents;

    @Test
    void empty() {
        task.executeImpl(context);

        verify(jugglerPushClient, times(0)).push(pushedEvents.capture());
    }

    @ParameterizedTest
    @MethodSource
    void singleReleaseProcess(List<LaunchState.Status> statuses, Status status, String description, boolean hasActive) {
        saveConfigs(configState(PROJECT_BLUE, PATH_MIKE, List.of(RELEASE_FAST)));
        var launches = EntryStream.of(statuses)
                .mapKeyValue((i, launchStatus) -> launch(PROJECT_BLUE, PATH_MIKE, RELEASE_FAST, i + 1, launchStatus))
                .toList();

        saveLaunches(launches);

        task.executeImpl(context);

        verify(jugglerPushClient).push(pushedEvents.capture());

        var tags = Stream.of(
                "test-namespace-ci-release-status",
                "project-project-blue",
                "path-mike__a.yaml",
                "release-release-fast",
                hasActive ? "has-active-launch" : null,
                "my-release-tag-release-fast"
        ).filter(Objects::nonNull).toList();

        assertThat(pushedEvents.getValue()).isEqualTo(List.of(
                RawEvent.builder()
                        .host("test-namespace-ci-release-status-project-blue")
                        .status(status)
                        .description(description)
                        .service("release-fast")
                        .instance("mike")
                        .tags(tags)
                        .build()
        ));
    }

    @Test
    void deletedReleaseProcess() {
        saveConfigs(configState(PROJECT_BLUE, PATH_MIKE, List.of(RELEASE_FAST)));
        saveLaunches(launch(PROJECT_BLUE, PATH_ANNA, RELEASE_FAST, 1, FAILURE));

        task.executeImpl(context);

        verify(jugglerPushClient).push(pushedEvents.capture());
        assertThat(pushedEvents.getValue()).containsExactlyInAnyOrder(
                RawEvent.builder()
                        .host("test-namespace-ci-release-status-project-blue")
                        .status(Status.OK)
                        .description("No running releases " +
                                "https://arcanum-test-url/projects/project-blue/ci/releases/timeline?dir=mike&id" +
                                "=release-fast")
                        .service("release-fast")
                        .instance("mike")
                        .tags(List.of(
                                "test-namespace-ci-release-status",
                                "project-project-blue",
                                "path-mike__a.yaml",
                                "release-release-fast",
                                "my-release-tag-release-fast"
                        ))
                        .build()
        );
    }

    @Test
    void deletedConfig() {
        saveLaunches(launch(PROJECT_BLUE, PATH_ANNA, RELEASE_FAST, 1, FAILURE));

        task.executeImpl(context);

        verify(jugglerPushClient, times(0)).push(pushedEvents.capture());
    }

    @Test
    void multipleConfigsAndProcesses() {
        saveConfigs(
                configState(PROJECT_BLUE, PATH_MIKE, List.of(RELEASE_FAST, RELEASE_NICE)),
                configState(PROJECT_BLUE, PATH_ANNA, List.of(RELEASE_FAST)),
                configState(PROJECT_PINK, PATH_JACK, List.of(RELEASE_FAST, RELEASE_HIGH))
        );
        saveLaunches(
                launch(PROJECT_BLUE, PATH_MIKE, RELEASE_FAST, 1, SUCCESS),

                launch(PROJECT_BLUE, PATH_MIKE, RELEASE_NICE, 1, WAITING_FOR_CLEANUP),
                launch(PROJECT_BLUE, PATH_MIKE, RELEASE_NICE, 2, WAITING_FOR_MANUAL_TRIGGER),

                launch(PROJECT_BLUE, PATH_ANNA, RELEASE_FAST, 1, WAITING_FOR_STAGE),
                launch(PROJECT_BLUE, PATH_ANNA, RELEASE_FAST, 2, RUNNING_WITH_ERRORS),

                launch(PROJECT_PINK, PATH_JACK, RELEASE_FAST, 1, RUNNING_WITH_ERRORS),
                launch(PROJECT_PINK, PATH_JACK, RELEASE_HIGH, 2, FAILURE)
        );

        task.executeImpl(context);

        verify(jugglerPushClient).push(pushedEvents.capture());
        var typeRef = new TypeReference<List<RawEvent>>() {
        };
        assertThat(pushedEvents.getValue())
                .isEqualTo(TestUtils.parseJson("juggler-pushed-events.json", typeRef));
    }

    @CsvSource({",ci-releases-cidemo", "testing,testing-ci-releases-cidemo"})
    @ParameterizedTest
    void namespace(String namespace, String expectedValue) {
        var cronTask = new ReleaseStatusesToJugglerPushCronTask(null, jugglerPushClient, db, namespace, null);
        assertThat(cronTask.withNamespace("ci-releases-cidemo")).isEqualTo(expectedValue);
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

    static Launch launch(
            String projectId,
            Path configPath,
            String releaseId,
            int launchNumber,
            LaunchState.Status status
    ) {
        return TestData.launchBuilder()
                .launchId(LaunchId.of(CiProcessId.ofRelease(configPath, releaseId), launchNumber))
                .version(Version.major(String.valueOf(launchNumber)))
                .project(projectId)
                .status(status)
                .build();
    }

    static ConfigState configState(String projectId, Path configPath, List<String> releases) {
        var releaseStates = releases.stream()
                .map(releaseId ->
                        ReleaseConfigState.builder()
                                .releaseId(releaseId)
                                .tag("my-release-tag-" + releaseId)
                                .build()
                ).toList();

        return ConfigState.builder()
                .id(ConfigState.Id.of(configPath))
                .project(projectId)
                .status(ConfigState.Status.OK)
                .releases(releaseStates)
                .build();
    }

    private static Arguments testCaseNoActive(
            List<LaunchState.Status> statuses,
            Status jugglerStatus,
            String description
    ) {
        return Arguments.arguments(statuses, jugglerStatus, description, false);
    }

    private static Arguments testCase(List<LaunchState.Status> statuses, Status jugglerStatus, String description) {
        return Arguments.arguments(statuses, jugglerStatus, description, true);
    }

    static Stream<Arguments> singleReleaseProcess() {
        return Stream.of(
                testCaseNoActive(List.of(CANCELED, SUCCESS),
                        Status.OK,
                        "No running releases" +
                                " https://arcanum-test-url/projects/project-blue/ci/releases/" +
                                "timeline?dir=mike&id=release-fast"
                ),
                testCase(List.of(CANCELED, RUNNING),
                        Status.OK,
                        "One release is running" +
                                " https://arcanum-test-url/projects/project-blue/ci/releases/" +
                                "flow?dir=mike&id=release-fast&version=2"
                ),
                testCase(List.of(CANCELLING, RUNNING, STARTING),
                        Status.OK,
                        "3 releases are running " +
                                "https://arcanum-test-url/projects/project-blue/ci/releases/timeline?dir=mike&id" +
                                "=release-fast"
                ),
                testCase(List.of(WAITING_FOR_MANUAL_TRIGGER, RUNNING, STARTING),
                        Status.WARN,
                        "Release is waiting for manual trigger" +
                                " https://arcanum-test-url/projects/project-blue/ci/releases/" +
                                "flow?dir=mike&id=release-fast&version=1"
                ),
                testCase(List.of(WAITING_FOR_MANUAL_TRIGGER, WAITING_FOR_MANUAL_TRIGGER, RUNNING),
                        Status.WARN,
                        "2 releases are waiting for manual trigger" +
                                " https://arcanum-test-url/projects/project-blue/ci/releases/" +
                                "timeline?dir=mike&id=release-fast"
                ),
                testCase(List.of(CANCELED, WAITING_FOR_MANUAL_TRIGGER, RUNNING_WITH_ERRORS),
                        Status.CRIT,
                        "Release is running but has failed jobs" +
                                " https://arcanum-test-url/projects/project-blue/ci/releases/" +
                                "flow?dir=mike&id=release-fast&version=3"
                ),
                testCase(List.of(CANCELED, RUNNING_WITH_ERRORS, RUNNING_WITH_ERRORS),
                        Status.CRIT,
                        "2 releases are running but have failed jobs" +
                                " https://arcanum-test-url/projects/project-blue/ci/releases/" +
                                "timeline?dir=mike&id=release-fast"
                ),
                testCase(List.of(CANCELED, RUNNING_WITH_ERRORS, FAILURE),
                        Status.CRIT,
                        "Release is failed and not running" +
                                " https://arcanum-test-url/projects/project-blue/ci/releases/" +
                                "flow?dir=mike&id=release-fast&version=3"
                ),
                testCase(List.of(RUNNING_WITH_ERRORS, FAILURE, FAILURE, FAILURE, FAILURE),
                        Status.CRIT,
                        "4 releases are failed and not running " +
                                "https://arcanum-test-url/projects/project-blue/ci/releases/" +
                                "timeline?dir=mike&id=release-fast"
                ),
                testCase(List.of(WAITING_FOR_MANUAL_TRIGGER, DELAYED, RUNNING),
                        Status.WARN,
                        "Release requires token delegation " +
                                "https://arcanum-test-url/projects/project-blue/ci/releases/" +
                                "flow?dir=mike&id=release-fast&version=2"
                ),
                testCase(List.of(WAITING_FOR_MANUAL_TRIGGER, POSTPONE, RUNNING),
                        Status.WARN,
                        "Release is waiting for manual trigger " +
                                "https://arcanum-test-url/projects/project-blue/ci/releases/" +
                                "flow?dir=mike&id=release-fast&version=1"
                ),
                testCase(List.of(WAITING_FOR_STAGE, IDLE, RUNNING),
                        Status.WARN,
                        "Release is not running and requires manual action " +
                                "https://arcanum-test-url/projects/project-blue/ci/releases/" +
                                "flow?dir=mike&id=release-fast&version=2"
                )
        );
    }

}
