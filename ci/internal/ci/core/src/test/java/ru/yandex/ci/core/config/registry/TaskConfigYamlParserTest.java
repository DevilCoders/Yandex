package ru.yandex.ci.core.config.registry;

import java.time.Duration;
import java.util.List;
import java.util.Map;
import java.util.Set;

import com.fasterxml.jackson.core.JsonProcessingException;
import org.junit.jupiter.api.Test;
import org.springframework.util.unit.DataSize;

import ru.yandex.ci.client.sandbox.api.ResourceState;
import ru.yandex.ci.client.sandbox.api.notification.NotificationStatus;
import ru.yandex.ci.client.sandbox.api.notification.NotificationTransport;
import ru.yandex.ci.core.config.a.model.JobAttemptsConfig;
import ru.yandex.ci.core.config.a.model.RuntimeConfig;
import ru.yandex.ci.core.config.a.model.RuntimeSandboxConfig;
import ru.yandex.ci.core.config.a.model.SandboxNotificationConfig;
import ru.yandex.ci.core.config.registry.sandbox.SandboxConfig;
import ru.yandex.ci.core.config.registry.sandbox.TaskPriority;
import ru.yandex.ci.core.config.registry.sandbox.TaskPriority.PriorityClass;
import ru.yandex.ci.core.config.registry.sandbox.TaskPriority.PrioritySubclass;
import ru.yandex.ci.core.launch.TaskVersion;
import ru.yandex.ci.test.TestUtils;
import ru.yandex.ci.util.gson.JsonObjectBuilder;

import static org.assertj.core.api.Assertions.assertThat;

class TaskConfigYamlParserTest {

    @Test
    void parseTaskletDeclaration() throws JsonProcessingException {
        TaskConfig actual = TaskConfigYamlParser.parse(TestUtils.textResource("task/tasklet.yaml"));

        TaskConfig expected = TaskConfig.builder()
                .title("Имя тасклета")
                .description("Описание")
                .maintainers("abc_group")
                .sources("/arcadia/path/to/sources")
                .tasklet(TaskletConfig.builder()
                        .implementation("WoodcutterPy")
                        .singleInput(true)
                        .singleOutput(true)
                        .build())
                .versions(Map.of(
                        TaskVersion.of("latest"), "42423523",
                        TaskVersion.of("testing"), "75674567",
                        TaskVersion.STABLE, "31241241"
                ))
                .resources(JsonObjectBuilder.builder()
                        .startMap("document")
                        .withProperty("title", "Лесопилка")
                        .withProperty("boards_per_timber", 3)
                        .end()
                        .build()
                )
                .requirements(RequirementsConfig.builder()
                        .withCores(2)
                        .withDiskBytes(16106127360L)
                        .withRamBytes(4294967296L)
                        .withTmpBytes(314572800L)
                        .withSandbox(
                                SandboxConfig.builder()
                                        .clientTags("LINUX | WINDOWS")
                                        .build()
                        )
                        .build()
                )
                .attempts(JobAttemptsConfig.ofAttempts(4))
                .build();

        assertThat(actual).isEqualTo(expected);
    }

    @Test
    void canOmitDefaultValues() throws JsonProcessingException {
        TaskConfig task = TaskConfigYamlParser.parse(TestUtils.textResource("task/tasklet-simple.yaml"));
        assertThat(task.getSources()).isNull();
        assertThat(task.getTasklet()).isNotNull();
        assertThat(task.getRequirements()).isNull();
        assertThat(task.getValidations()).isNotNull().isEmpty();
        assertThat(task.getResources()).isNull();
    }

    @Test
    void sandboxTaskDeclaration() throws JsonProcessingException {
        TaskConfig task = TaskConfigYamlParser.parse(TestUtils.textResource("task/sandbox-task.yaml"));
        assertThat(task.getSources()).isNull();
        assertThat(task.getTasklet()).isNull();
        assertThat(task.getSandboxTask()).isEqualTo(SandboxTaskConfig.ofName("YA_PACKAGE"));
    }

    @Test
    void sandboxTaskExtendedDeclaration() throws JsonProcessingException {
        TaskConfig task = TaskConfigYamlParser.parse(TestUtils.textResource("task/sandbox-task-extended.yaml"));
        assertThat(task.getSandboxTask().getRequiredParameters())
                .containsExactly(
                        "build_system",
                        "build_type",
                        "checkout_arcadia_from_url",
                        "package_type",
                        "publish_package",
                        "resource_type",
                        "run_tests"
                );
        assertThat(task.getResources()).isEqualTo(
                JsonObjectBuilder.builder()
                        .withProperty("build_system", "ya")
                        .withProperty("build_type", "release")
                        .withProperty(
                                "checkout_arcadia_from_url",
                                "arcadia-arc:/#${context.target_revision.hash}"
                        )
                        .withProperty("package_type", "tarball")
                        .withProperty("publish_package", true)
                        .withProperty("resource_type", "YA_PACKAGE")
                        .withProperty("run_tests", true)
                        .withProperty("ya_timeout", 10800)
                        .withProperty("parent_layer", 1116539399)
                        .build()
        );
        assertThat(task.getSandboxTask().getAcceptResourceStates())
                .isEqualTo(Set.of(
                        ResourceState.BROKEN,
                        ResourceState.DELETED,
                        ResourceState.READY,
                        ResourceState.NOT_READY));
    }

    @Test
    void sandboxTaskReportConfigsDeclaration() throws JsonProcessingException {
        TaskConfig task = TaskConfigYamlParser.parse(
                TestUtils.textResource("task/sandbox-task-with-reports.yaml")
        );
        assertThat(task.getSources()).isNull();
        assertThat(task.getTasklet()).isNull();
        assertThat(task.getSandboxTask()).isEqualTo(SandboxTaskConfig.builder()
                .name("YA_PACKAGE")
                .badgesConfigs(
                        List.of(
                                SandboxTaskBadgesConfig.of("my_report_id", "SAMOGON"),
                                SandboxTaskBadgesConfig.of("my_other_report_id")))
                .build()
        );
    }

    @Test
    void sandboxTaskTemplateDeclaration() throws JsonProcessingException {
        TaskConfig task = TaskConfigYamlParser.parse(
                TestUtils.textResource("task/sandbox-task-with-template.yaml")
        );
        assertThat(task.getSources()).isNull();
        assertThat(task.getTasklet()).isNull();
        assertThat(task.getSandboxTask()).isEqualTo(SandboxTaskConfig.ofTemplate("MyTemplate"));
    }

    @Test
    void sandboxBinaryTaskDeclaration() throws JsonProcessingException {
        TaskConfig task = TaskConfigYamlParser.parse(TestUtils.textResource("task/sandbox-binary-task.yaml"));
        assertThat(task.getSources()).isNull();
        assertThat(task.getTasklet()).isNull();
        assertThat(task.getSandboxTask()).isEqualTo(SandboxTaskConfig.ofName("YA_PACKAGE"));
        assertThat(task.getVersions()).isEqualTo(Map.of(
                TaskVersion.of("stable"), "1",
                TaskVersion.of("testing"), "2"));
    }

    @Test
    void sandboxTaskWithRuntime() throws JsonProcessingException {
        TaskConfig task = TaskConfigYamlParser.parse(TestUtils.textResource("task/task-with-runtime.yaml"));
        assertThat(task.getSources()).isNull();
        assertThat(task.getTasklet()).isNull();
        assertThat(task.getSandboxTask()).isEqualTo(SandboxTaskConfig.ofName("YA_PACKAGE"));

        assertThat(task.getRuntimeConfig()).isEqualTo(RuntimeConfig.builder()
                .getOutputOnFail(true)
                .sandbox(RuntimeSandboxConfig.builder()
                        .tags(List.of("WOODCUTTER", "CI_EXAMPLE"))
                        .hint("version-${context.version_info.full}")
                        .priority(new TaskPriority(PriorityClass.BACKGROUND, PrioritySubclass.LOW))
                        .killTimeout(Duration.ofHours(1).plusMinutes(20))
                        .notification(SandboxNotificationConfig.builder()
                                .status(NotificationStatus.FAILURE)
                                .transport(NotificationTransport.TELEGRAM)
                                .recipients(List.of("andreevdm", "pochemuto"))
                                .build())
                        .build())
                .build());
    }

    @Test
    void sandboxTaskWithRequirements() throws JsonProcessingException {
        TaskConfig task = TaskConfigYamlParser.parse(TestUtils.textResource("task/task-with-requirements.yaml"));
        assertThat(task.getSources()).isNull();
        assertThat(task.getTasklet()).isNull();
        assertThat(task.getSandboxTask()).isEqualTo(SandboxTaskConfig.ofName("YA_PACKAGE"));

        assertThat(task.getRequirements()).isEqualTo(RequirementsConfig.builder()
                .withDisk(DataSize.ofGigabytes(15))
                .withCores(2)
                .withRam(DataSize.ofGigabytes(4))
                .withTmp(DataSize.ofMegabytes(300))
                .withSandbox(SandboxConfig.builder()
                        .clientTags("GENERIC & LINUX")
                        .build())
                .build());
    }

    @Test
    void parseTaskletDeclarationWithContainer() throws JsonProcessingException {
        var configYson = TaskConfigYamlParser.parse(TestUtils.textResource("task/tasklet-with-container.yaml"));
        var configJson = TestUtils.parseJson("task/tasklet-with-container.json", RequirementsConfig.class);
        assertThat(configYson.getRequirements())
                .isEqualTo(configJson);
    }
}
