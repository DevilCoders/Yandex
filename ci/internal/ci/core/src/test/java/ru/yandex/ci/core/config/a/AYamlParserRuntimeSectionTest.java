package ru.yandex.ci.core.config.a;

import java.time.Duration;
import java.util.List;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.client.sandbox.api.notification.NotificationStatus;
import ru.yandex.ci.client.sandbox.api.notification.NotificationTransport;
import ru.yandex.ci.core.config.a.model.AYamlConfig;
import ru.yandex.ci.core.config.a.model.CiConfig;
import ru.yandex.ci.core.config.a.model.RuntimeConfig;
import ru.yandex.ci.core.config.a.model.RuntimeSandboxConfig;
import ru.yandex.ci.core.config.a.model.SandboxNotificationConfig;
import ru.yandex.ci.core.config.a.validation.ValidationReport;
import ru.yandex.ci.core.config.registry.sandbox.TaskPriority;
import ru.yandex.ci.core.config.registry.sandbox.TaskPriority.PriorityClass;
import ru.yandex.ci.core.config.registry.sandbox.TaskPriority.PrioritySubclass;
import ru.yandex.ci.test.TestUtils;

import static org.assertj.core.api.Assertions.assertThat;

class AYamlParserRuntimeSectionTest {

    @Test
    void invalidEmpty() throws Exception {
        assertInvalid("invalid-empty.yaml");
    }

    @Test
    void invalidSandboxWithoutOwner() throws Exception {
        assertInvalid("invalid-sandbox-without-owner.yaml");
    }

    @Test
    void invalidTwoSandboxSections() throws Exception {
        assertInvalid("invalid-two-sb-sections.yaml");
    }

    @Test
    void onlySbOwnerInRuntime() throws Exception {
        assertValid("only-sb-owner-in-runtime.yaml", config(RuntimeSandboxConfig.ofOwner("CI")));
    }

    @Test
    void onlyOwnerInSandbox() throws Exception {
        assertValid("only-owner-in-sandbox.yaml", config(RuntimeSandboxConfig.ofOwner("CI")));
    }

    @Test
    void sandboxOneNotification() throws Exception {
        SandboxNotificationConfig notificationConfig = SandboxNotificationConfig.builder()
                .recipient("andreevdm")
                .transport(NotificationTransport.TELEGRAM)
                .status(NotificationStatus.TIMEOUT)
                .build();
        assertValid(
                "sandbox-one-notification.yaml",
                config(RuntimeSandboxConfig.builder()
                        .owner("CI")
                        .notifications(List.of(notificationConfig))
                        .build())
        );
    }

    @Test
    void sandboxNotifications() throws Exception {
        SandboxNotificationConfig documentationExample = SandboxNotificationConfig.builder()
                .recipients(List.of("andreevdm", "pochemuto"))
                .transport(NotificationTransport.TELEGRAM)
                .status(NotificationStatus.FAILURE)
                .build();

        var singleFieldNotificationConfig = SandboxNotificationConfig.builder()
                .recipient("andreevdm")
                .transport(NotificationTransport.EMAIL)
                .statuses(List.of(
                        NotificationStatus.TIMEOUT,
                        NotificationStatus.FAILURE,
                        NotificationStatus.EXCEPTION
                ))
                .build();

        var singleLineArrayNotificationConfig = SandboxNotificationConfig.builder()
                .recipients(List.of("andreevdm", "user42"))
                .transport(NotificationTransport.EMAIL)
                .statuses(List.of(
                        NotificationStatus.WAIT_MUTEX,
                        NotificationStatus.FINISHING,
                        NotificationStatus.WAIT_RES))
                .build();

        var multilineArrayNotificationConfig = SandboxNotificationConfig.builder()
                .recipients(List.of("andreevdm", "user42"))
                .transport(NotificationTransport.TELEGRAM)
                .statuses(List.of(
                        NotificationStatus.WAIT_MUTEX,
                        NotificationStatus.FINISHING,
                        NotificationStatus.WAIT_RES))
                .build();

        List<SandboxNotificationConfig> notificationConfig = List.of(
                documentationExample,
                singleFieldNotificationConfig,
                singleLineArrayNotificationConfig,
                multilineArrayNotificationConfig
        );

        var sandboxConfig = RuntimeSandboxConfig.builder();
        sandboxConfig.owner("CI");
        sandboxConfig.notifications(notificationConfig);
        sandboxConfig.killTimeout(Duration.ofHours(1).plusMinutes(20));
        sandboxConfig.tags(List.of("WOODCUTTER", "CI_EXAMPLE"));
        sandboxConfig.hint("version-${context.version_info.full}");
        sandboxConfig.priority(new TaskPriority(PriorityClass.BACKGROUND, PrioritySubclass.LOW));

        ValidationReport report = AYamlParser.parseAndValidate(readResource("sandbox-notifications.yaml"));
        assertThat(report.isSuccess()).isTrue();
        assertThat(report.getConfig().getCi().getRuntime().getSandbox()).isEqualTo(sandboxConfig.build());
        assertThat(report.getConfig().getCi().getAction("my-action").getRuntimeConfig())
                .isEqualTo(RuntimeConfig.ofSandboxOwner("CIDEMO"));
    }

    private AYamlConfig config(RuntimeSandboxConfig sandboxConfig) {
        var ciConfig = CiConfig.builder();
        ciConfig.secret("sec-01dy7t26dyht1bj4w3yn94fsa");
        ciConfig.runtime(RuntimeConfig.of(sandboxConfig));
        return new AYamlConfig("ci", "Woodcutter", ciConfig.build(), null);
    }

    private void assertValid(String config, AYamlConfig expected) throws Exception {
        ValidationReport report = AYamlParser.parseAndValidate(readResource(config));
        assertThat(report.isSuccess()).isTrue();
        assertThat(report.getConfig()).isEqualTo(expected);

    }

    private void assertInvalid(String config) throws Exception {
        ValidationReport report = AYamlParser.parseAndValidate(readResource(config));
        assertThat(report.isSuccess()).isFalse();
    }

    private static String readResource(String resource) {
        return TestUtils.textResource("ayaml/runtime/" + resource);
    }

}
