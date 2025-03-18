package ru.yandex.ci.core.config.a;

import java.time.Duration;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.TreeSet;
import java.util.stream.Collectors;
import java.util.stream.Stream;
import java.util.stream.StreamSupport;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.JsonNode;
import com.github.fge.jsonschema.core.exceptions.ProcessingException;
import com.github.fge.jsonschema.core.report.ProcessingMessage;
import com.google.gson.JsonNull;
import com.google.gson.JsonParser;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;
import org.assertj.core.description.Description;
import org.junit.jupiter.api.Nested;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;
import org.yaml.snakeyaml.error.YAMLException;

import ru.yandex.ci.client.sandbox.api.notification.NotificationStatus;
import ru.yandex.ci.client.sandbox.api.notification.NotificationTransport;
import ru.yandex.ci.core.config.a.model.AYamlConfig;
import ru.yandex.ci.core.config.a.model.ActionConfig;
import ru.yandex.ci.core.config.a.model.AutocheckConfig;
import ru.yandex.ci.core.config.a.model.CiConfig;
import ru.yandex.ci.core.config.a.model.CleanupConditionConfig;
import ru.yandex.ci.core.config.a.model.CleanupConfig;
import ru.yandex.ci.core.config.a.model.CleanupReason;
import ru.yandex.ci.core.config.a.model.DisplacementConfig;
import ru.yandex.ci.core.config.a.model.FlowConfig;
import ru.yandex.ci.core.config.a.model.FlowWithFlowVars;
import ru.yandex.ci.core.config.a.model.JobConfig;
import ru.yandex.ci.core.config.a.model.ManualConfig;
import ru.yandex.ci.core.config.a.model.NeedsType;
import ru.yandex.ci.core.config.a.model.ReleaseConfig;
import ru.yandex.ci.core.config.a.model.ReleaseTitleSource;
import ru.yandex.ci.core.config.a.model.RuntimeConfig;
import ru.yandex.ci.core.config.a.model.RuntimeSandboxConfig;
import ru.yandex.ci.core.config.a.model.SandboxNotificationConfig;
import ru.yandex.ci.core.config.a.model.StageConfig;
import ru.yandex.ci.core.config.a.model.StrongModeConfig;
import ru.yandex.ci.core.config.a.model.TriggerConfig;
import ru.yandex.ci.core.config.a.model.TriggerConfig.Into;
import ru.yandex.ci.core.config.a.validation.ValidationReport;
import ru.yandex.ci.core.launch.LaunchState.Status;
import ru.yandex.ci.test.TestUtils;
import ru.yandex.ci.util.CollectionUtils;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;

@Slf4j
class AYamlParserTest {

    @ParameterizedTest
    @MethodSource
    void validateAYamlConfig(String configPath) throws Exception {
        checkAYaml(configPath, true);
    }

    @ParameterizedTest
    @MethodSource
    void validateAYamlParsed(String configPath, AYamlConfig expect) throws Exception {
        var report = checkAYaml(configPath, true);
        assertThat(report.getConfig()).isEqualTo(expect);
    }

    @ParameterizedTest(name = "[{index}] {0}")
    @MethodSource
    void validateAYamlSchemaError(String configPath, List<ValidationMessage> messages) throws Exception {
        checkInvalidSchema(configPath, Set.copyOf(messages));
    }

    @ParameterizedTest(name = "[{index}] {0}")
    @MethodSource
    void validateAYamlStaticError(String configPath, List<String> staticErrors) throws Exception {
        var report = checkAYaml(configPath, false);
        assertThat(new TreeSet<>(report.getStaticErrors()))
                .isEqualTo(new TreeSet<>(Set.copyOf(staticErrors)));
    }

    @Test
    void noTitlesTest() throws Exception {
        AYamlConfig actual = checkAYaml("ayaml/no-titles.yaml", true).getConfig();
        assertThat(actual.getCi().getFlow("sawmill").getTitle()).isEqualTo("sawmill");
        assertThat(actual.getCi().getReleaseTitleSource()).isEqualTo(ReleaseTitleSource.RELEASE);
        assertThat(actual.getCi().getRelease("my-app-1"))
                .extracting(r -> r.getStages().get(0).getTitle())
                .isEqualTo("Single 1");
        assertThat(actual.getCi().getRelease("my-app-2"))
                .extracting(r -> r.getStages().get(0).getTitle())
                .isEqualTo("Single 2");
        assertThat(actual.getCi().getFlow("sawmill"))
                .extracting(f -> f.getJob("woodcutter1").getTitle())
                .isEqualTo("woodcutter1");
    }

    @Test
    void noTitlesSourceLegacyTest() throws Exception {
        AYamlConfig actual = checkAYaml("ayaml/no-titles-source-legacy.yaml", true).getConfig();
        assertThat(actual.getCi().getFlow("sawmill").getTitle()).isEqualTo("sawmill");
        assertThat(actual.getCi().getReleaseTitleSource()).isEqualTo(ReleaseTitleSource.RELEASE);
        assertThat(actual.getCi().getRelease("my-app-1"))
                .extracting(r -> r.getStages().get(0).getTitle())
                .isEqualTo("Single 1");
        assertThat(actual.getCi().getRelease("my-app-2"))
                .extracting(r -> r.getStages().get(0).getTitle())
                .isEqualTo("Single 2");
        assertThat(actual.getCi().getFlow("sawmill"))
                .extracting(f -> f.getJob("woodcutter1").getTitle())
                .isEqualTo("woodcutter1");
    }

    @Test
    void titlesWithMiscTest() throws Exception {
        AYamlConfig actual = checkAYaml("ayaml/titles-with-misc-symbols.yaml", true).getConfig();
        assertThat(actual.getCi().getFlow("sawmill").getTitle()).isEqualTo("Module /root");
        assertThat(actual.getCi().getRelease("my-app-1"))
                .extracting(ReleaseConfig::getTitle)
                .isEqualTo("Release for [common/test/module]");
        assertThat(actual.getCi().getRelease("my-app-1"))
                .extracting(r -> r.getStages().get(0).getTitle())
                .isEqualTo("Стадия /корневая");
        assertThat(actual.getCi().getRelease("my-app-2"))
                .extracting(ReleaseConfig::getTitle)
                .isEqualTo("My f+-*/@$#%^&*()n release");
        assertThat(actual.getCi().getRelease("my-app-2"))
                .extracting(r -> r.getStages().get(0).getTitle())
                .isEqualTo("single");
        assertThat(actual.getCi().getFlow("sawmill"))
                .extracting(f -> f.getJob("woodcutter1").getTitle())
                .isEqualTo("Задача example/settlers/woodcutter");
    }

    @Test
    void getMapAndEntity() throws Exception {
        CiConfig config = checkAYaml("ayaml/release-and-samwill.yaml", true).getConfig().getCi();

        assertThat(List.copyOf(config.getReleases().values()))
                .isEqualTo(List.of(AYamlParserData.releaseConfig()));

        assertThat(config.getRelease(AYamlParserData.releaseConfig().getId()))
                .isEqualTo(AYamlParserData.releaseConfig());

        assertThat(List.copyOf(config.getFlows().values()))
                .isEqualTo(List.of(AYamlParserData.sawmillFlow(), AYamlParserData.releaseFlow()));

        assertThat(config.getFlow(AYamlParserData.sawmillFlow().getId()))
                .isEqualTo(AYamlParserData.sawmillFlow());

        assertThat(config.getFlow(AYamlParserData.releaseFlow().getId()))
                .isEqualTo(AYamlParserData.releaseFlow());
    }

    @Test
    void resourceOverrideTest() throws Exception {
        var ci = checkAYaml("ayaml/resource-override.yaml", true).getConfig().getCi();
        assertThat(ci.getFlows().get("test").getJob("test").getInput().getAsJsonObject().get("yav_secret"))
                .isEqualTo(JsonNull.INSTANCE);
    }

    @Test
    void sawmillBackAndForth() throws Exception {
        AYamlConfig original = AYamlParserData.sawmillAYaml();

        String yaml = AYamlParser.toYaml(original);
        log.info("Rendered YML: {}", yaml);

        AYamlConfig processed = AYamlParser.parseAndValidate(yaml).getConfig();
        assertThat(processed).isEqualTo(original);
    }

    @Test
    void releaseAndSawmillAYamlBackAndForth() throws Exception {
        AYamlConfig original = AYamlParserData.releaseAndSawmillAYaml();

        String yaml = AYamlParser.toYaml(original);
        log.info("Rendered YML: {}", yaml);

        AYamlConfig processed = AYamlParser.parseAndValidate(yaml).getConfig();
        assertThat(processed).isEqualTo(original);
    }

    @Test
    void prIntoBranch() throws Exception {
        var report = checkAYaml("ayaml/pr-into-branch.yaml", true);
        AYamlConfig config = report.getConfig();

        var actions = config.getCi().getMergedActions();

        var expectTriggers = List.of(
                TriggerConfig.builder()
                        .on(TriggerConfig.On.PR)
                        .targetBranches(List.of(Into.TRUNK))
                        .build(),
                TriggerConfig.builder()
                        .on(TriggerConfig.On.PR)
                        .targetBranches(List.of(Into.RELEASE_BRANCH))
                        .build(),
                TriggerConfig.builder()
                        .on(TriggerConfig.On.PR)
                        .targetBranches(List.of(Into.RELEASE_BRANCH, Into.TRUNK))
                        .build(),
                TriggerConfig.builder()
                        .on(TriggerConfig.On.PR)
                        .build(),
                TriggerConfig.builder()
                        .on(TriggerConfig.On.PR)
                        .required(false)
                        .build(),
                TriggerConfig.builder()
                        .on(TriggerConfig.On.COMMIT)
                        .targetBranches(List.of(Into.TRUNK))
                        .build(),
                TriggerConfig.builder()
                        .on(TriggerConfig.On.COMMIT)
                        .targetBranches(List.of(Into.RELEASE_BRANCH))
                        .build(),
                TriggerConfig.builder()
                        .on(TriggerConfig.On.COMMIT)
                        .targetBranches(List.of(Into.RELEASE_BRANCH, Into.TRUNK))
                        .build()
        );
        assertThat(actions).isEqualTo(Map.of(
                "sample-action", ActionConfig.builder()
                        .id("sample-action")
                        .title("Simple job from action")
                        .flow("sawmill")
                        .trigger(TriggerConfig.builder()
                                .on(TriggerConfig.On.PR)
                                .targetBranches(List.of(Into.TRUNK))
                                .build())
                        .build(),
                "sawmill", ActionConfig.builder()
                        .id("sawmill")
                        .flow("sawmill")
                        .triggers(expectTriggers)
                        .virtual(true)
                        .build()));

        var triggers = actions.get("sawmill").getTriggers();
        assertThat(triggers.get(3).getTargetBranchesOrDefault())
                .containsExactlyInAnyOrder(Into.TRUNK.getAcceptedBranchType());
        assertThat(triggers.get(3).getRequired())
                .isTrue();
    }

    @SuppressWarnings("ClassCanBeStatic")
    @Nested
    class StageValidation {

        @ParameterizedTest
        @MethodSource
        void validateAYamlConfig(String configPath) throws Exception {
            AYamlParserTest.this.validateAYamlConfig(configPath);
        }

        @ParameterizedTest
        @MethodSource
        void validateAYamlSchemaError(String configPath, List<ValidationMessage> messages) throws Exception {
            AYamlParserTest.this.validateAYamlSchemaError(configPath, messages);
        }

        @ParameterizedTest
        @MethodSource
        void validateAYamlStaticError(String configPath, List<String> staticErrors) throws Exception {
            AYamlParserTest.this.validateAYamlStaticError(configPath, staticErrors);
        }

        @Test
        void displacement() throws Exception {
            var report = AYamlParserTest.this.checkAYaml("ayaml/stages/displacement.yaml", true);

            var config = report.getConfig();
            var release = config.getCi().getRelease("release-id-1");

            assertThat(release.getStages()).isEqualTo(List.of(
                    StageConfig.builder()
                            .id("first-stage")
                            .title("First stage")
                            .displace(new DisplacementConfig(Set.of(
                                    Status.WAITING_FOR_MANUAL_TRIGGER,
                                    Status.WAITING_FOR_STAGE)))
                            .build(),
                    StageConfig.builder()
                            .id("second-stage")
                            .title("Seconds stage")
                            .displace(new DisplacementConfig(Set.of(
                                    Status.RUNNING,
                                    Status.RUNNING_WITH_ERRORS,
                                    Status.FAILURE,
                                    Status.WAITING_FOR_MANUAL_TRIGGER,
                                    Status.WAITING_FOR_STAGE,
                                    Status.WAITING_FOR_SCHEDULE)))
                            .build()
            ));
        }

        static List<String> validateAYamlConfig() {
            return List.of(
                    "ayaml/stages/ok.yaml",
                    "ayaml/stages/unused.yaml",
                    "ayaml/stages/duplicate.yaml",
                    "ayaml/stages/implicit-single.yaml",
                    "ayaml/stages/array-stages.yaml"
            );
        }

        static List<Arguments> validateAYamlSchemaError() {
            return List.of(
                    Arguments.of("ayaml/stages/invalid-displacement.yaml", List.of(
                            new ValidationMessage(
                                    "instance failed to match at least one required schema among 2",
                                    "/ci/releases/release-id-1/stages")
                    ))
            );
        }

        static List<Arguments> validateAYamlStaticError() {
            return List.of(
                    Arguments.of("ayaml/stages/missed.yaml", List.of(
                            "stage 'third-stage' in /ci/flows/release-flow-id-1/jobs/verify/stage" +
                                    " not declared in release /ci/releases/release-id-1/stages. Declared: " +
                                    "[second-stage]",
                            "stage 'first-stage' in /ci/flows/release-flow-id-1/jobs/build/stage" +
                                    " not declared in release /ci/releases/release-id-1/stages. Declared: " +
                                    "[second-stage]"
                    )),
                    Arguments.of("ayaml/stages/implicit-single-wrong-declared.yaml", List.of(
                            "stage 'not-exists-stage' in /ci/flows/release-flow-id-1/jobs/deploy/stage" +
                                    " not declared in release /ci/releases/release-id-1/stages." +
                                    " Stages in release config are not provided, one implicit stage 'single' " +
                                    "is available. Declare stages in release section or fix job stage property"
                    )),
                    Arguments.of("ayaml/stages/implicit-single-two-input-output.yaml", List.of(
                            "Implicit stage 'single' must have one entry point. Flow /ci/flows/release-flow-id-1"
                    ))
            );
        }

    }

    @Test
    @SuppressWarnings("unchecked")
    void orderOfParsedMap() throws JsonProcessingException {
        Map<String, Integer> result = AYamlParser.getMapper().readValue(
                """
                        a: 1
                        b: 2
                        c: 3
                        """,
                Map.class
        );
        assertThat(result).isInstanceOf(LinkedHashMap.class);

        var map = new LinkedHashMap<String, Integer>();
        map.put("a", 1);
        map.put("b", 2);
        map.put("c", 3);
        assertThat(result).isEqualTo(map);

        map.clear();
        map.put("b", 2);
        map.put("a", 1);
        map.put("c", 3);
        assertThat(
                AYamlParser.getMapper().readValue(
                        """
                                b: 2
                                a: 1
                                c: 3
                                """,
                        Map.class
                )
        ).isEqualTo(map);

        map.clear();
        map.put("c", 3);
        map.put("a", 1);
        map.put("b", 2);
        assertThat(
                AYamlParser.getMapper().readValue(
                        """
                                c: 3
                                a: 1
                                b: 2
                                """,
                        Map.class
                )
        ).isEqualTo(map);
    }

    @Test
    void releaseAutoParse() throws Exception {
        AYamlConfig actual = checkAYaml("ayaml/release-auto.yaml", true).getConfig();
        assertThat(List.copyOf(actual.getCi().getReleases().values()))
                .isEqualTo(AYamlParserData.autoParseReleases());
    }

    @Test
    void releaseBranched() throws Exception {
        AYamlConfig actual = checkAYaml("ayaml/release-branches.yaml", true).getConfig();
        assertThat(List.copyOf(actual.getCi().getReleases().values()))
                .isEqualTo(AYamlParserData.releaseBranches());
    }

    @Test
    void releaseWithHotfixFlowOnMultipleStages() throws Exception {
        AYamlConfig actual = checkAYaml("ayaml/release-with-hotfix-flows-on-multiple-stages.yaml", true).getConfig();

        assertThat(actual).isNotNull();
        assertThat(actual.getCi()).isNotNull();

    }

    @Test
    void validateEmptyYaml() throws JsonProcessingException, ProcessingException {
        String ayaml = "    ";
        ValidationReport report = AYamlParser.parseAndValidate(ayaml);
        assertThat(report.isSuccess()).isFalse();
        assertThat(report.getSchemaReport())
                .extracting(ProcessingMessage::getMessage)
                .containsExactly("object has missing required properties ([\"service\",\"title\"])");
    }

    @Test
    void validateInvalidYaml() throws JsonProcessingException, ProcessingException {
        String ayaml = "asd";
        ValidationReport report = AYamlParser.parseAndValidate(ayaml);
        assertThat(report.isSuccess()).isFalse();
        assertThat(report.getSchemaReport())
                .extracting(ProcessingMessage::getMessage)
                .containsExactly(
                        "instance matched a schema which it should not have",
                        "instance type (string) does not match any allowed primitive type (allowed: [\"object\"])");
    }

    @Test
    void sandboxTaskTags() throws Exception {
        var config = checkAYaml("ayaml/tags.yaml", true).getConfig();
        assertThat(config.getCi().getRuntime().getSandbox().getTags())
                .isEqualTo(List.of("MY_CUSTOM_TAG", "ANOTHER_CUSTOM_TAG"));
    }

    @Test
    void sandboxTaskHints() throws Exception {
        var config = checkAYaml("ayaml/hints.yaml", true).getConfig();
        assertThat(config.getCi().getRuntime().getSandbox().getHints())
                .isEqualTo(List.of("MY_CUSTOM_HINT", "ANOTHER_CUSTOM_HINT"));
    }

    @Test
    void emptySandboxTaskTags() throws Exception {
        var config = checkAYaml("ayaml/empty-tags.yaml", true).getConfig();
        assertThat(config.getCi().getRuntime().getSandbox().getTags()).isEmpty();
    }

    @Test
    void runtimeWithoutOwner() throws JsonProcessingException, ProcessingException {
        ValidationReport report = AYamlParser.parseAndValidate("""
                    service: market
                    title: Hello World Project
                    ci:
                      secret: sec-XXXXXX
                      runtime:
                        sandbox: {}
                """);

        assertThat(report.isSuccess()).isFalse();
        assertThat(report.getStaticErrors())
                .containsExactly("ci.runtime must have sandbox.owner or sandbox-owner property");
    }

    @Test
    void actionCustomRuntime() throws Exception {
        ValidationReport report = checkAYaml("ayaml/action-custom-runtime.yaml", true);

        var config = report.getConfig();
        var action = config.getCi().getAction("my-action");

        var sandboxConfig = RuntimeSandboxConfig.builder();
        sandboxConfig.owner("CI-for-actions");
        sandboxConfig.tags(List.of("OVERRIDDEN"));
        var expected = RuntimeConfig.of(sandboxConfig.build());
        assertThat(action.getRuntimeConfig()).isEqualTo(expected);
    }

    @Test
    void releaseCustomRuntime() throws Exception {
        ValidationReport report = checkAYaml("ayaml/release-custom-runtime.yaml", true);

        var config = report.getConfig();
        var release = config.getCi().getRelease("my-release");

        var sandboxConfig = RuntimeSandboxConfig.builder();
        sandboxConfig.tags(List.of("OVERRIDDEN"));
        sandboxConfig.hints(List.of("h3", "h4"));
        sandboxConfig.killTimeout(Duration.ofHours(5));
        sandboxConfig.notification(
                SandboxNotificationConfig.builder()
                        .recipient("pochemuto")
                        .transport(NotificationTransport.EMAIL)
                        .status(NotificationStatus.ASSIGNED)
                        .build());
        var expected = RuntimeConfig.of(sandboxConfig.build());
        assertThat(release.getRuntimeConfig()).isEqualTo(expected);

        var flow = config.getCi().getFlow("simple-flow");
        var job = flow.getJob("single");
        assertThat(job.getKillTimeout())
                .isEqualTo(Duration.ofHours(6));
        assertThat(job.getJobRuntimeConfig())
                .isEqualTo(RuntimeConfig.builder()
                        .sandbox(RuntimeSandboxConfig.builder()
                                .killTimeout(Duration.ofHours(7))
                                .tags(List.of("job-tag-1", "job-tag-2"))
                                .hints(List.of("job-hint-1", "job-hint-2"))
                                .build())
                        .build());
    }

    @Test
    void releaseWithDisplacementOnStart() throws Exception {
        var report = checkAYaml("ayaml/release-displace-on-start.yaml", true);
        var ci = report.getConfig().getCi();

        assertThat(ci.getRelease("r1").getDisplacementOnStart())
                .isEqualTo(ReleaseConfig.DisplacementOnStart.AUTO);
        assertThat(ci.getRelease("r2").getDisplacementOnStart())
                .isEqualTo(ReleaseConfig.DisplacementOnStart.AUTO);
        assertThat(ci.getRelease("r3").getDisplacementOnStart())
                .isEqualTo(ReleaseConfig.DisplacementOnStart.ENABLED);
        assertThat(ci.getRelease("r4").getDisplacementOnStart())
                .isEqualTo(ReleaseConfig.DisplacementOnStart.DISABLED);
    }

    @Test
    void showInActions() throws Exception {
        FlowConfig flow1 = FlowConfig.builder()
                .id("flow-show-in-actions-true")
                .title("Test 1")
                .job(JobConfig.builder()
                        .id("testing")
                        .title("Test flow")
                        .task("dummy")
                        .manual(ManualConfig.of(false))
                        .build()
                )
                .showInActions(true)
                .build();
        FlowConfig flow2 = FlowConfig.builder()
                .id("flow-show-in-actions-false")
                .title("Test 2")
                .job(JobConfig.builder()
                        .id("testing")
                        .title("Test flow")
                        .task("dummy")
                        .manual(ManualConfig.of(false))
                        .build()
                )
                .showInActions(false)
                .build();
        FlowConfig flow3 =
                FlowConfig.builder()
                        .id("flow-show-in-actions-null")
                        .title("Test 3")
                        .job(JobConfig.builder()
                                .id("testing")
                                .title("Test flow")
                                .task("dummy")
                                .manual(ManualConfig.of(false))
                                .build()
                        )
                        .build();

        AYamlConfig actual = checkAYaml("ayaml/show-in-actions.yaml", true).getConfig();

        var ciConfig = CiConfig.builder()
                .flow(flow1, flow2, flow3)
                .secret("sec-01dy7t26dyht1bj4w3yn94fsa")
                .runtime(RuntimeConfig.ofSandboxOwner("CI"))
                .build();
        assertThat(actual).isEqualTo(
                new AYamlConfig("ci", "Show in actions Test", ciConfig, null)
        );
    }

    @Test
    void dependsFromJobs() throws Exception {
        AYamlConfig actual = checkAYaml("ayaml/needs-type.yaml", true).getConfig();

        var ciConfig = CiConfig.builder()
                .flow(FlowConfig.builder()
                        .id("simple-flow")
                        .title("Test parsing job dependencies")
                        .job(jobTemplate("job-1", List.of(), NeedsType.ALL),
                                jobTemplate("job-2", List.of("job-1"), NeedsType.ALL),
                                jobTemplate("job-3", List.of("job-1"), NeedsType.ALL),
                                jobTemplate("job-4", List.of("job-1"), NeedsType.ANY),
                                jobTemplate("job-5", List.of("job-1"), NeedsType.FAIL))
                        .build())
                .secret("sec-01dy7t26dyht1bj4w3yn94fsa")
                .runtime(RuntimeConfig.ofSandboxOwner("CI"))
                .build();
        assertThat(actual).isEqualTo(
                new AYamlConfig("ci", "Test parsing job dependencies", ciConfig, null)
        );
    }

    @Test
    void startVersion() throws Exception {
        ValidationReport report = checkAYaml("ayaml/release-start-version.yaml", true);

        ReleaseConfig release = report.getConfig().getCi().getRelease("my-release");
        assertThat(release.getStartVersion()).isEqualTo(18);
    }

    @Test
    void flowVars() throws Exception {
        ValidationReport report = checkAYaml("ayaml/release-flow-vars.yaml", true);

        ReleaseConfig release = report.getConfig().getCi().getRelease("my-app");
        assertThat(release).isEqualTo(
                ReleaseConfig.builder()
                        .id("my-app")
                        .title("app4")
                        .flow("release-flow-common")
                        .hotfixFlows(List.of(
                                new FlowWithFlowVars("hotfix-flow", null, null, null),
                                new FlowWithFlowVars("hotfix-flow-common", JsonParser.parseString("""
                                        {
                                         "woodcutter": "Лесоруб hotfix 1"
                                        }
                                        """).getAsJsonObject(), null, null)))
                        .rollbackFlows(List.of(
                                new FlowWithFlowVars("hotfix-flow-common", JsonParser.parseString("""
                                        {
                                         "woodcutter": "Лесоруб rollback 1"
                                        }
                                        """).getAsJsonObject(),
                                        null,
                                        Set.of("hotfix-flow", "hotfix-flow-common"))))
                        .flowVars(JsonParser.parseString("""
                                {
                                 "woodcutter": "Лесоруб"
                                }
                                """).getAsJsonObject())
                        .stages(List.of(StageConfig.IMPLICIT_STAGE))
                        .startVersion(8)
                        .build());

        var actions = report.getConfig().getCi().getMergedActions();
        assertThat(actions).isEqualTo(Map.of(
                "my-action", ActionConfig.builder()
                        .id("my-action")
                        .flow("release-flow-common")
                        .flowVars(JsonParser.parseString("""
                                {
                                 "woodcutter": "Лесоруб2"
                                }
                                """).getAsJsonObject())
                        .build()));
    }

    @Test
    void prWithCleanup() throws Exception {
        var report = checkAYaml("ayaml/pr-with-cleanup.yaml", true);
        AYamlConfig config = report.getConfig();

        var actions = config.getCi().getMergedActions();
        assertThat(actions).isEqualTo(Map.of(
                "sawmill", ActionConfig.builder()
                        .id("sawmill")
                        .flow("sawmill")
                        .trigger(TriggerConfig.builder()
                                .on(TriggerConfig.On.PR)
                                .targetBranches(List.of(Into.TRUNK))
                                .build())
                        .trigger(TriggerConfig.builder()
                                .on(TriggerConfig.On.PR)
                                .build())
                        .cleanupConfig(new CleanupConfig(Duration.ofMinutes(10),
                                Set.of(), List.of())
                        ).build(),
                "sawmill2", ActionConfig.builder()
                        .id("sawmill2")
                        .flow("sawmill")
                        .cleanupConfig(new CleanupConfig(Duration.ofMinutes(9),
                                Set.of(Status.SUCCESS, Status.FAILURE, Status.RUNNING_WITH_ERRORS,
                                        Status.WAITING_FOR_MANUAL_TRIGGER, Status.WAITING_FOR_SCHEDULE),
                                List.of(
                                        CleanupConditionConfig.builder()
                                                .reasons(Set.of(CleanupReason.NEW_DIFF_SET))
                                                .cleanup(false)
                                                .interrupt(false)
                                                .build(),
                                        CleanupConditionConfig.builder()
                                                .reasons(Set.of(CleanupReason.PR_MERGED))
                                                .cleanup(true)
                                                .interrupt(true)
                                                .build(),
                                        CleanupConditionConfig.builder()
                                                .reasons(Set.of(CleanupReason.PR_DISCARDED))
                                                .cleanup(false)
                                                .interrupt(true)
                                                .build(),
                                        CleanupConditionConfig.builder()
                                                .reasons(Set.of(CleanupReason.FINISH))
                                                .cleanup(true)
                                                .interrupt(false)
                                                .build()
                                ))
                        ).build(),
                "sawmill3", ActionConfig.builder()
                        .id("sawmill3")
                        .flow("sawmill")
                        .cleanupConfig(new CleanupConfig(Duration.ofMinutes(8),
                                Set.of(Status.SUCCESS),
                                List.of(
                                        CleanupConditionConfig.builder()
                                                .reasons(Set.of(CleanupReason.NEW_DIFF_SET,
                                                        CleanupReason.PR_MERGED,
                                                        CleanupReason.PR_DISCARDED,
                                                        CleanupReason.FINISH))
                                                .build()
                                ))
                        ).build()
        ));

        var flow = FlowConfig.builder();
        flow.id("sawmill");
        flow.title("Simple job");
        flow.job(
                JobConfig.builder()
                        .id("testing")
                        .title("Выкладка в тестинг")
                        .description("Выкладывает в тестинг")
                        .task("common/deploy/release")
                        .build()
        );
        flow.cleanupJob(
                JobConfig.builder()
                        .id("cleanup-1")
                        .title("Очистка 1")
                        .task("dummy")
                        .build(),
                JobConfig.builder()
                        .id("cleanup-2")
                        .title("Очистка 2")
                        .task("dummy")
                        .build()
        );
        assertThat(List.copyOf(config.getCi().getFlows().values()))
                .isEqualTo(List.of(flow.build()));
    }

    @Test
    void exampleActuallyParses() throws Exception {
        ValidationReport report = checkAYaml("ayaml/example-in-docs.yaml", true);

        var serializedYaml = readResource("ayaml/example-in-docs-serialized.yaml");
        assertThat(AYamlParser.toYaml(report.getConfig())).isEqualTo(serializedYaml);

        ValidationReport serializedReport = AYamlParser.parseAndValidate(serializedYaml);
        assertThat(serializedReport.isSuccess()).withFailMessage(serializedReport.toString()).isTrue();

        assertThat(report.getConfig())
                .isNotNull();
        assertThat(report.getConfig())
                .isEqualTo(serializedReport.getConfig());
    }

    static List<Arguments> strongMode() {
        return List.of(
                Arguments.of(
                        "ayaml/strong-mode-with-scopes.yaml",
                        StrongModeConfig.builder()
                                .enabled(true)
                                .abcScopes(CollectionUtils.linkedSet("development", "devops"))
                                .build()
                ),
                Arguments.of(
                        "ayaml/strong-mode-with-single-scope.yaml",
                        StrongModeConfig.builder()
                                .enabled(true)
                                .abcScopes(CollectionUtils.linkedSet("devops"))
                                .build()
                ),
                Arguments.of(
                        "ayaml/strong-mode.yaml",
                        StrongModeConfig.builder()
                                .enabled(true)
                                .abcScopes(StrongModeConfig.DEFAULT_ABC_SCOPES)
                                .build()
                ),
                Arguments.of(
                        "ayaml/strong-mode-disabled.yaml",
                        StrongModeConfig.of(false)
                )
        );
    }

    @ParameterizedTest
    @MethodSource
    void strongMode(String configPath, StrongModeConfig strongModeConfig) throws Exception {
        ValidationReport report = checkAYaml(configPath, true);

        var autocheck = AutocheckConfig.builder()
                .strongMode(strongModeConfig)
                .build();

        var ciConfig = CiConfig.builder()
                .autocheck(autocheck)
                .build();

        assertThat(report.getConfig()).isEqualTo(
                new AYamlConfig("ci", "Strong mode test", ciConfig, null)
        );
    }

    @Test
    void descriptions() throws Exception {
        ValidationReport report = checkAYaml("ayaml/descriptions.yaml", true);

        var config = report.getConfig();
        var ci = config.getCi();

        assertThat(ci.getMergedActions())
                .isEqualTo(Map.of(
                        "action1",
                        ActionConfig.builder()
                                .id("action1")
                                .title("Action Title")
                                .description("Action Description")
                                .flow("flow1")
                                .build()
                ));
        assertThat(ci.getReleases())
                .isEqualTo(Map.of(
                        "release1",
                        ReleaseConfig.builder()
                                .id("release1")
                                .title("Release Title")
                                .description("Release Description\nMultiline\n")
                                .flow("flow1")
                                .stages(List.of(StageConfig.IMPLICIT_STAGE))
                                .build()
                ));

        assertThat(ci.getFlows())
                .isEqualTo(Map.of(
                        "flow1",
                        FlowConfig.builder()
                                .id("flow1")
                                .title("FLow Title")
                                .description("Flow Description")
                                .job(JobConfig.builder()
                                        .id("dummy")
                                        .title("Job Title")
                                        .description("Job Description")
                                        .task("dummy")
                                        .stage(StageConfig.IMPLICIT_STAGE.getId())
                                        .build())
                                .build()
                ));
    }

    @Nested
    class FlowVarsUiTestCase {
        @Test
        void flowVarsUi() throws Exception {
            ValidationReport report = checkAYaml("ayaml/flow-vars-ui/valid.yaml", true);
            var config = report.getConfig();
            var expectedSchema = TestUtils.readJson("ayaml/flow-vars-ui/schema.json");

            assertThat(config.getCi().getRelease("flow-var-release").getFlowVarsUi().getSchema())
                    .isEqualTo(expectedSchema);

            assertThat(config.getCi().getRelease("flow-var-release").getRollbackFlows().get(0)
                    .getFlowVarsUi().getSchema())
                    .isEqualTo(expectedSchema);

            assertThat(config.getCi().getRelease("flow-var-release").getHotfixFlows().get(0)
                    .getFlowVarsUi().getSchema())
                    .isEqualTo(expectedSchema);

            assertThat(config.getCi().getAction("flow-var-action").getFlowVarsUi().getSchema())
                    .isEqualTo(expectedSchema);
        }

        @Test
        void flowVarsUiDeclaredBoth() throws Exception {
            ValidationReport report = checkAYaml("ayaml/flow-vars-ui/declared-both.yaml", false);
            assertThat(report.getStaticErrors())
                    .containsExactly(
                            "Properties [title] declared in flow-vars and flow-vars-ui.schema.properties." +
                                    " Flow var value can either be static and declared in flow-vars section" +
                                    " or be configurable in ui and described in flow-var-ui schema." +
                                    " If you want configurable flow var with default value" +
                                    " set `default` property in schema.",
                            "Properties [iteration] declared in flow-vars and flow-vars-ui.schema.properties." +
                                    " Flow var value can either be static and declared in flow-vars section" +
                                    " or be configurable in ui and described in flow-var-ui schema." +
                                    " If you want configurable flow var with default value" +
                                    " set `default` property in schema."
                    );
        }

        @Test
        void flowVarsUiInconsistentTypes() throws Exception {
            ValidationReport report = checkAYaml("ayaml/flow-vars-ui/inconsistent-types.yaml", false);
            assertThat(String.join("\n", report.getStaticErrors()))
                    .isEqualTo(TestUtils.textResource("ayaml/flow-vars-ui/inconsistent-types.errors.txt").trim());
        }

        @Test
        void flowVarsUiReleaseWithoutDefaults() throws Exception {
            ValidationReport report = checkAYaml("ayaml/flow-vars-ui/autorelease-without-defaults.yaml", false);
            assertThat(report.getStaticErrors())
                    .containsExactly(
                            "Release flow-var-release has enabled auto," +
                                    " but some flow-vars-ui top-level fields don't have defaults: iterations."
                    );
        }

        @Test
        void flowVarsUiActionsWithoutDefaults() throws Exception {
            ValidationReport report = checkAYaml("ayaml/flow-vars-ui/action-without-defaults.yaml", false);
            assertThat(report.getStaticErrors())
                    .containsExactly(
                            "Action flow-var-action has trigger," +
                                    " but some flow-vars-ui top-level fields don't have defaults:" +
                                    " release-resources, deploy."
                    );
        }
    }

    @Test
    void checkNoBooleanTransformation() throws Exception {
        ValidationReport report = checkAYaml("ayaml/flow-with-on.yaml", true);

        var jobs = report.getConfig().getCi().getFlow("flow-with-on").getJobs();

        assertThat(jobs.get("on"))
                .isEqualTo(JobConfig.builder()
                        .id("on")
                        .title("on")
                        .task("dummy")
                        .manual(ManualConfig.of(true))
                        .input(JsonParser.parseString("""
                                { "taskbox_enabled": "on" }
                                """).getAsJsonObject())
                        .build()
                );

        assertThat(jobs.get("off"))
                .isEqualTo(JobConfig.builder()
                        .id("off")
                        .title("off")
                        .task("dummy")
                        .manual(ManualConfig.of(false))
                        .input(JsonParser.parseString("""
                                { "taskbox_enabled": "off" }
                                """).getAsJsonObject())
                        .build()
                );
    }

    @Test
    void testDurationTypeDefinition() throws Exception {
        ValidationReport report = checkAYaml("ayaml/type-duration.yaml", true);
        var jobs = report.getConfig().getCi().getFlow("duration-type-definition-test").getJobs();

        assertThat(jobs.get("duration-3h").getKillTimeout()).isEqualTo(Duration.ofHours(3));
        assertThat(jobs.get("duration-3h-17m").getKillTimeout()).isEqualTo(Duration.ofHours(3).plusMinutes(17));
        assertThat(jobs.get("duration-5h-25s").getKillTimeout()).isEqualTo(Duration.ofHours(5).plusSeconds(25));
        assertThat(jobs.get("duration-3d-25s").getKillTimeout()).isEqualTo(Duration.ofDays(3).plusSeconds(25));
        assertThat(jobs.get("duration-4w-23s").getKillTimeout()).isEqualTo(Duration.ofDays(4 * 7).plusSeconds(23));
        assertThat(jobs.get("duration-4w-3d-23s").getKillTimeout())
                .isEqualTo(Duration.ofDays(4 * 7 + 3).plusSeconds(23));
    }

    @Test
    void testBugSnakeyaml_1_29() throws Exception {
        // 1.28 and 1.30 is OK, 1.29 is not

        // See CI-3209
        // java.lang.ArrayIndexOutOfBoundsException: Index -1 out of bounds for length 1024

        // DO NOT CHANGE THIS FILE, EVERY CHAR MATTERS
        checkAYaml("ayaml/bugs/snakeyaml-1.29-ArrayIndexOutOfBoundsException.yaml", true);
    }

    @Test
    void testRecursiveAnchors() {
        assertThatThrownBy(() -> checkAYaml("ayaml/yaml-with-recursive-anchors.yaml", false))
                .isInstanceOf(YAMLException.class)
                .hasMessage("Detected recursive YAML Anchor: some-anchor");
    }

    @Test
    void testOutOfPlaceAnchors() {
        assertThatThrownBy(() -> checkAYaml("ayaml/yaml-with-out-of-place-anchors.yaml", false))
                .isInstanceOf(YAMLException.class)
                .hasMessage("""
                        found undefined alias some-anchor
                         in 'string', line 5, column 10:
                                key: *some-anchor
                                     ^
                        """);
    }

    private ValidationReport checkAYaml(String configPath, boolean expectValid) throws Exception {
        var report = AYamlParser.parseAndValidate(readResource(configPath));
        log.info(report.toString());
        assertThat(report.isSuccess())
                .describedAs(new Description() {
                                 @Override
                                 public String value() {
                                     return "Expected config is " + str(expectValid)
                                             + ", but it is " + str(report.isSuccess());
                                 }

                                 private static String str(boolean valid) {
                                     return valid ? "valid" : "NOT valid";
                                 }
                             }
                )
                .isEqualTo(expectValid);
        return report;
    }

    private void checkInvalidSchema(String configPath, Set<ValidationMessage> expectedReportMessages) throws Exception {
        var report = checkAYaml(configPath, false);

        assertThat(report.getStaticErrors().isEmpty()).isTrue();

        Set<ValidationMessage> actualReportMessages = StreamSupport.stream(
                Objects.requireNonNull(report.getSchemaReport()).spliterator(),
                false
        ).flatMap(m -> {
            JsonNode root = m.asJson();
            var message = root.get("message").asText();
            var pointer = root.get("instance").get("pointer").asText();
            if (message.startsWith("instance failed to match all required schemas")) {
                return StreamSupport.stream(root.get("reports").spliterator(), false)
                        .filter(subReports -> subReports.size() > 0)
                        .flatMap(subReports -> StreamSupport.stream(subReports.spliterator(), false))
                        .map(sub -> sub.get("message").asText())
                        .map(subMessage -> new ValidationMessage(subMessage, pointer));
            } else {
                return Stream.of(new ValidationMessage(message, pointer));
            }
        }).collect(Collectors.toSet());

        assertThat(actualReportMessages).isEqualTo(expectedReportMessages);
    }

    private static String readResource(String resource) {
        return TestUtils.textResource(resource);
    }

    private static JobConfig jobTemplate(String jobId, List<String> needs, NeedsType needsType) {
        return JobConfig.builder().id(jobId).title(jobId).task("dummy")
                .needs(needs)
                .needsType(needsType)
                .build();
    }

    static List<String> validateAYamlConfig() {
        return List.of(
                "ayaml/arcanum-section.yaml",
                "ayaml/sawmill.yaml",
                "ayaml/release-and-samwill.yaml",
                "ayaml/manual-test.yaml",
                "ayaml/manual-merge-test.yaml",
                "ayaml/trust-example.yaml",
                "ayaml/job-requirements.yaml",
                "ayaml/sandbox-requirements.yaml",
                "ayaml/arcanum-section.yaml",
                "ayaml/autocheck-single-fast-target-test.yaml",
                "ayaml/autocheck-fast-targets-test.yaml",
                "ayaml/only-ci-autocheck-fast-targets-test.yaml",
                "ayaml/filter_by_abs_paths_with_any_discovery.yaml",
                "ayaml/filter_by_abs_paths_with_graph_discovery.yaml",
                "ayaml/filter_by_abs_paths_with_dir_discovery.yaml",
                "ayaml/filter_by_abs_paths_without_discovery.yaml",
                "ayaml/filter_by_discovery_dirs.yaml",
                "ayaml/allow-names-length.yaml",
                "ayaml/filter_by_not_feature_branches.yaml",
                "ayaml/filter_by_not_authors.yaml",
                "ayaml/filter_by_not_author_services.yaml",
                "ayaml/filter_by_not_queues.yaml",
                "ayaml/filter_by_not_abs_paths.yaml",
                "ayaml/filter_by_not_sub_paths.yaml",
                "ayaml/with-runtime.yaml",
                "ayaml/with-requirements.yaml"
        );
    }

    static List<Arguments> validateAYamlParsed() {
        return List.of(
                Arguments.of("ayaml/sawmill.yaml", AYamlParserData.sawmillAYaml()),
                Arguments.of("ayaml/release-and-samwill.yaml", AYamlParserData.releaseAndSawmillAYaml()),
                Arguments.of("ayaml/manual-test.yaml", AYamlParserData.manualTestAYaml()),
                Arguments.of("ayaml/manual-merge-test.yaml", AYamlParserData.manualTestAYaml()),
                Arguments.of("ayaml/no-flows.yaml", AYamlParserData.noFlowsTestAYaml()),
                Arguments.of("ayaml/autocheck-single-fast-target-test.yaml",
                        AYamlParserData.singleFastTargetTestAYaml()),
                Arguments.of("ayaml/autocheck-fast-targets-test.yaml",
                        AYamlParserData.fastTargetsTestAYaml()),
                Arguments.of("ayaml/autocheck-empty-fast-targets-test.yaml",
                        AYamlParserData.emptyFastTargetsTestAYaml()),
                Arguments.of("ayaml/large-autostart.yaml",
                        AYamlParserData.autocheckAYaml(AYamlParserData.autocheckLargeAutostartConfig())),
                Arguments.of("ayaml/large-autostart-simple.yaml",
                        AYamlParserData.autocheckAYaml(AYamlParserData.autocheckLargeAutostartSimpleConfig())),
                Arguments.of("ayaml/native-build.yaml",
                        AYamlParserData.autocheckAYaml(AYamlParserData.autocheckNativeBuildsConfig())),
                Arguments.of("ayaml/additional-secrets-single.yaml",
                        AYamlParserData.additionalSecretsSingle()),
                Arguments.of("ayaml/additional-secrets-list.yaml",
                        AYamlParserData.additionalSecretsList()),
                Arguments.of("ayaml/permissions.yaml",
                        AYamlParserData.allPermissions()),
                Arguments.of("ayaml/tracker-watcher.yaml",
                        AYamlParserData.trackerWatcher()),
                Arguments.of("ayaml/job-attempts/valid.yaml",
                        AYamlParserData.jobAttempts()),
                Arguments.of("ayaml/job-attempts/valid-conditional.yaml",
                        AYamlParserData.jobAttemptsConditional()),
                Arguments.of("ayaml/job-attempts/valid-reuse.yaml",
                        AYamlParserData.jobAttemptsReuse()),
                Arguments.of("ayaml/job-attempts/valid-tasklet-v2.yaml",
                        AYamlParserData.jobAttemptsTaskletV2()),
                Arguments.of("ayaml/job-requirements-priority.yaml",
                        AYamlParserData.jobRequirementsPriority())
        );
    }

    @SuppressWarnings("MethodLength")
    static List<Arguments> validateAYamlSchemaError() {
        var len90 = "012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789";
        var len270 = "%s%s%s".formatted(len90, len90, len90);
        return List.of(
                Arguments.of("ayaml/autocheck-no-fast-targets-test.yaml", List.of(
                        new ValidationMessage(
                                "instance failed to match exactly one schema (matched 0 out of 2)",
                                "/ci/autocheck/fast-targets")
                )),
                Arguments.of("ayaml/invalid-schema/forbidden-properties.yaml", List.of(
                        new ValidationMessage("instance matched a schema which it should not have", "")
                )),
                Arguments.of("ayaml/invalid-schema/large-autostart-no-deps.yaml", List.of(
                        new ValidationMessage(
                                "property \"large-autostart\" of object has missing property dependencies " +
                                        "(requires [/ci/secret, /ci/runtime]; missing: [/ci/secret, /ci/runtime])",
                                "/ci/autocheck")
                )),
                Arguments.of("ayaml/invalid-schema/invalid-jobs-tasks-1.yaml", List.of(
                        new ValidationMessage(
                                taskIdPathErrorTemplate("example/path/job1.yaml"),
                                "/ci/flows/sawmill/jobs/job1/task")
                )),
                Arguments.of("ayaml/invalid-schema/invalid-jobs-tasks-2.yaml", List.of(
                        new ValidationMessage(
                                taskIdPathErrorTemplate("example/path/job2/"),
                                "/ci/flows/sawmill/jobs/job2/task")
                )),
                Arguments.of("ayaml/invalid-schema/restricted-names.yaml", List.of(
                        new ValidationMessage(abcSlugErrorTemplate("Ci"), "/service")
                )),
                Arguments.of("ayaml/invalid-schema/restricted-names-triggers.yaml", List.of(
                        new ValidationMessage(idErrorTemplate("sawmill_"), "/ci/triggers/0/flow")
                )),
                Arguments.of("ayaml/invalid-schema/invalid-jobs-tasks-3.yaml", List.of(
                        new ValidationMessage(
                                taskIdPathErrorTemplate("/example/path/job3"),
                                "/ci/flows/sawmill/jobs/job3/task")
                )),
                Arguments.of("ayaml/invalid-schema/invalid-jobs-tasks-4.yaml", List.of(
                        new ValidationMessage(
                                taskIdPathErrorTemplate("example/path/job4.yml"),
                                "/ci/flows/sawmill/jobs/job4/task")
                )),
                Arguments.of("ayaml/invalid-schema/native-build-invalid-empty_targets.yaml", List.of(
                        new ValidationMessage(
                                "instance failed to match exactly one schema (matched 0 out of 2)",
                                "/ci/autocheck/native-builds/toolchain-2")
                )),
                Arguments.of("ayaml/invalid-schema/autocheck-invalid-fast-targets-test.yaml", List.of(
                        new ValidationMessage(
                                "instance failed to match exactly one schema (matched 0 out of 2)",
                                "/ci/autocheck/fast-targets")
                )),
                Arguments.of("ayaml/invalid-schema/native-build-invalid.yaml", List.of(
                        new ValidationMessage(
                                "object instance has properties which are not allowed by the schema: [\"toolchain-\"]",
                                "/ci/autocheck/native-builds")
                )),
                Arguments.of("ayaml/autocheck-no-fast-targets-test.yaml", List.of(
                        new ValidationMessage(
                                "instance failed to match exactly one schema (matched 0 out of 2)",
                                "/ci/autocheck/fast-targets")
                )),
                Arguments.of("ayaml/release-branches-wrong-nested.yaml", List.of(
                        new ValidationMessage(
                                releaseBranchPatternErrorTemplate(
                                        "releases/experimental/release_machine_tests//rm_test_ci/stable-${version}"),
                                "/ci/releases/release-1/branches/pattern")
                )),
                Arguments.of("ayaml/invalid-schema/invalid-on-trunk.yaml", List.of(
                        new ValidationMessage(
                                "instance value (\"trunk\") not found in enum (possible values: [\"pr\",\"commit\"])",
                                "/ci/triggers/0/on")
                )),
                Arguments.of("ayaml/invalid-schema/restricted-names-length-outer.yaml", List.of(
                        new ValidationMessage("""
                                object instance has properties which are not allowed by the schema: \
                                ["action270-%s"]""".formatted(len270),
                                "/ci/actions"),
                        new ValidationMessage("""
                                object instance has properties which are not allowed by the schema: \
                                ["release270-%s"]""".formatted(len270),
                                "/ci/releases"),
                        new ValidationMessage("""
                                object instance has properties which are not allowed by the schema: \
                                ["flow270-%s"]""".formatted(len270),
                                "/ci/flows")
                )),
                Arguments.of("ayaml/invalid-schema/restricted-names-length-inner-action.yaml", List.of(
                        new ValidationMessage(
                                titleErrorTemplate("Title flow270-%s".formatted(len270)),
                                "/ci/actions/action90-%s/title".formatted(len90)),
                        new ValidationMessage(
                                shortMessageErrorTemplate("Title flow270-%s".formatted(len270)),
                                "/ci/actions/action90-%s/title".formatted(len90)),
                        new ValidationMessage(
                                idErrorTemplate("flow270-%s".formatted(len270)),
                                "/ci/actions/action90-%s/flow".formatted(len90)),
                        new ValidationMessage(
                                shortMessageErrorTemplate("flow270-%s".formatted(len270)),
                                "/ci/actions/action90-%s/flow".formatted(len90))
                )),
                Arguments.of("ayaml/invalid-schema/restricted-names-length-inner-release1.yaml", List.of(
                        new ValidationMessage(
                                titleErrorTemplate("Title release270-%s".formatted(len270)),
                                "/ci/releases/release90-1-%s/title".formatted(len90)),
                        new ValidationMessage(
                                shortMessageErrorTemplate("Title release270-%s".formatted(len270)),
                                "/ci/releases/release90-1-%s/title".formatted(len90))
                )),
                Arguments.of("ayaml/invalid-schema/restricted-names-length-inner-release2.yaml", List.of(
                        new ValidationMessage("instance failed to match at least one required schema among 2",
                                "/ci/releases/release90-2-%s/stages".formatted(len90))
                )),
                Arguments.of("ayaml/invalid-schema/restricted-names-length-inner-release3.yaml", List.of(
                        new ValidationMessage("instance failed to match at least one required schema among 2",
                                "/ci/releases/release90-3-%s/stages".formatted(len90))
                )),
                Arguments.of("ayaml/invalid-schema/restricted-names-length-inner-release4.yaml", List.of(
                        new ValidationMessage(
                                idErrorTemplate("flow270-%s".formatted(len270)),
                                "/ci/releases/release90-4-%s/flow".formatted(len90)),
                        new ValidationMessage(
                                shortMessageErrorTemplate("flow270-%s".formatted(len270)),
                                "/ci/releases/release90-4-%s/flow".formatted(len90))
                )),
                Arguments.of("ayaml/invalid-schema/restricted-names-length-inner-flow1.yaml", List.of(
                        new ValidationMessage(
                                titleErrorTemplate("Title flow270-%s".formatted(len270)),
                                "/ci/flows/flow90-1-%s/title".formatted(len90)),
                        new ValidationMessage(
                                shortMessageErrorTemplate("Title flow270-%s".formatted(len270)),
                                "/ci/flows/flow90-1-%s/title".formatted(len90)),
                        new ValidationMessage(
                                titleErrorTemplate("Title job270-1-%s".formatted(len270)),
                                "/ci/flows/flow90-1-%s/jobs/job90-1-%s/title".formatted(len90, len90)),
                        new ValidationMessage(
                                shortMessageErrorTemplate("Title job270-1-%s".formatted(len270)),
                                "/ci/flows/flow90-1-%s/jobs/job90-1-%s/title".formatted(len90, len90)),
                        new ValidationMessage(
                                idErrorTemplate("stage270-%s".formatted(len270)),
                                "/ci/flows/flow90-1-%s/jobs/job90-1-%s/stage".formatted(len90, len90)),
                        new ValidationMessage(
                                shortMessageErrorTemplate("stage270-%s".formatted(len270)),
                                "/ci/flows/flow90-1-%s/jobs/job90-1-%s/stage".formatted(len90, len90)),
                        new ValidationMessage("instance failed to match exactly one schema (matched 0 out of 2)",
                                "/ci/flows/flow90-1-%s/jobs/job90-1-%s/needs".formatted(len90, len90))
                )),
                Arguments.of("ayaml/invalid-schema/restricted-names-length-inner-flow2.yaml", List.of(
                        new ValidationMessage("""
                                object instance has properties which are not allowed by the schema: \
                                ["job90-1-%s"]""".formatted(len270),
                                "/ci/flows/flow90-1-%s/jobs".formatted(len90))
                )),
                Arguments.of("ayaml/strong-mode-with-empty-scopes.yaml", List.of(
                        new ValidationMessage(
                                "instance failed to match exactly one schema (matched 0 out of 2)",
                                "/ci/autocheck/strong")
                )),
                Arguments.of("ayaml/strong-mode-with-duplicates-in-scopes.yaml", List.of(
                        new ValidationMessage(
                                "instance failed to match exactly one schema (matched 0 out of 2)",
                                "/ci/autocheck/strong")
                )),
                Arguments.of("ayaml/release-start-version-illegal.yaml", List.of(
                        new ValidationMessage(
                                "numeric instance is lower than the required minimum (minimum: 1, found: 0)",
                                "/ci/releases/my-release/start-version")
                )),
                Arguments.of("ayaml/invalid-schema/invalid-permissions.yaml", List.of(
                        new ValidationMessage(
                                "object instance has properties which are not allowed by the schema: " +
                                        "[\"start-flow-unknown\"]",
                                "/ci/permissions")
                ))
        );
    }

    static List<Arguments> validateAYamlStaticError() {
        return List.of(
                Arguments.of("ayaml/pr-into-branch-trunk-trigger.yaml", List.of(
                        "trigger at /ci/triggers/2 has 'required', but 'on' option is not a PR"
                )),
                Arguments.of("ayaml/pr-into-branch-trunk-trigger.yaml", List.of(
                        "trigger at /ci/triggers/2 has 'required', but 'on' option is not a PR"
                )),
                Arguments.of("ayaml/release-auto-forbid-trunk.yaml", List.of(
                        "release at /ci/releases/my-app-1 has enabled auto" +
                                " but /ci/releases/my-app-1/branches/forbid-trunk-releases is true"
                )),
                Arguments.of("ayaml/no-jobs.yaml", List.of(
                        "flow at /ci/flows/declared-empty-list must have at least one job"
                )),
                Arguments.of("ayaml/release-wrong-with-unknown-hotfix-flow.yaml", List.of(
                        "flow 'unknown-flow' in /ci/releases/my-app-1/hotfix-flows not found in /ci/flows"
                )),
                Arguments.of("ayaml/release-wrong-with-unknown-rollback-flow.yaml", List.of(
                        "flow 'unknown-flow' in /ci/releases/my-app-1/rollback-flows not found in /ci/flows"
                )),
                Arguments.of("ayaml/release-wrong-with-duplicate-hotfix-flow.yaml", List.of(
                        "flow 'release-flow-common' in /ci/releases/my-app-1/hotfix-flows " +
                                "is same as /ci/releases/my-app-1/flow"
                )),
                Arguments.of("ayaml/release-wrong-with-duplicate-rollback-flow.yaml", List.of(
                        "flow 'release-flow-common' in /ci/releases/my-app-1/rollback-flows " +
                                "is same as /ci/releases/my-app-1/flow"
                )),
                Arguments.of("ayaml/release-wrong-with-hotfix-flow-invalid-stage.yaml", List.of(
                        "stage 'stage2-unknown' in /ci/flows/release-flow-hotfix/jobs/task2/stage " +
                                "not declared in release /ci/releases/my-app-1/stages. Declared: [stage1, stage2]"
                )),
                Arguments.of("ayaml/release-flow-missed.yaml", List.of(
                        "flow 'not-existing-flow' in /ci/releases/my-app-1/flow not found in /ci/flows"
                )),
                Arguments.of("ayaml/invalid-schema/invalid-needs-with-stages.yaml", List.of(
                        "Job 'unexisting-job' in /ci/flows/flow1/jobs/job1/needs not found in /ci/flows/flow1/jobs"
                )),
                Arguments.of("ayaml/invalid-schema/invalid-feature-branch-on-trunk.yaml", List.of(
                        "flow at /ci/flows/declared-empty-list must have at least one job",
                        "trigger at /ci/triggers/0/filters has 'feature-branches', but 'on' option is not a PR"
                )),
                Arguments.of("ayaml/invalid-schema/invalid-feature-branch-on-commit.yaml", List.of(
                        "flow at /ci/flows/declared-empty-list must have at least one job",
                        "trigger at /ci/triggers/0/filters has 'feature-branches', but 'on' option is not a PR"
                )),
                Arguments.of("ayaml/invalid-schema/invalid-duplicate-jobs-id.yaml", List.of(
                        "Job with id job1-1 is duplicate, all jobs id must be unique"
                )),
                Arguments.of("ayaml/invalid-schema/invalid-cleanup-jobs-id-reference.yaml", List.of(
                        "Job 'job1' in /ci/flows/my-flow-1/cleanup-jobs/cleanup-job1/needs " +
                                "not found in /ci/flows/my-flow-1/cleanup-jobs",
                        "Job 'cleanup-job1' in /ci/flows/my-flow-2/jobs/job1/needs " +
                                "not found in /ci/flows/my-flow-2/jobs"
                )),
                Arguments.of("ayaml/pr_filter_by_abs_paths_with_any_discovery.yaml", List.of(
                        "trigger on PR does not support abs-paths [/ci/triggers/0]"
                )),
                Arguments.of("ayaml/pr_filter_by_abs_paths_with_graph_discovery.yaml", List.of(
                        "trigger on PR does not support GRAPH discovery [/ci/triggers/0]"
                )),
                Arguments.of("ayaml/pr_filter_by_abs_paths_with_dir_discovery.yaml", List.of(
                        "trigger on PR does not support abs-paths [/ci/triggers/0]"
                )),
                Arguments.of("ayaml/pr_filter_by_abs_paths_without_discovery.yaml", List.of(
                        "trigger on PR does not support abs-paths [/ci/triggers/0]"
                )),
                Arguments.of("ayaml/filter_by_discovery_dirs_invalid_prefix.yaml", List.of(
                        "filter at /ci/triggers/0 has has invalid abs-paths configuration [*/core]: " +
                                "Path must contains at least one top-level folder and cannot have " +
                                "wildcard chars: *?[]{}",
                        "filter at /ci/triggers/0 has has invalid abs-paths configuration [/ci/**]: " +
                                "Path cannot starts from / or \\",
                        "filter at /ci/triggers/0 has has invalid abs-paths configuration [/]: " +
                                "Path cannot starts from / or \\",
                        "filter at /ci/triggers/0 has has invalid abs-paths configuration [../ci/**]: " +
                                "Path cannot be relative",
                        "filter at /ci/triggers/0 has has invalid sub-paths configuration [/ci/**]: " +
                                "Path cannot starts from / or \\",
                        "filter at /ci/triggers/0 has has invalid sub-paths configuration [/]: " +
                                "Path cannot starts from / or \\",
                        "filter at /ci/triggers/0 has has invalid sub-paths configuration [../ci/**]: " +
                                "Path cannot be relative"
                )),
                Arguments.of("ayaml/autorelease-min-commits-without-since-last-release.yaml", List.of(
                        "cannot have min-commits: 0 with undefined since-last-release"
                ))
        );
    }

    @Value
    private static class ValidationMessage {
        String message;
        String instancePointer;
    }

    private static String shortMessageErrorTemplate(String value) {
        return "string \"%s\" is too long (length: %s, maximum allowed: 256)".formatted(value, value.length());
    }

    private static String idErrorTemplate(String value) {
        return "ECMA 262 regex \"^((\\w|-){0,255}[A-Za-z0-9])?$\" does not match input string \"%s\"".formatted(value);
    }

    private static String titleErrorTemplate(String value) {
        return "ECMA 262 regex \"^.{0,256}?$\" does not match input string \"%s\"".formatted(value);
    }

    private static String abcSlugErrorTemplate(String value) {
        return "ECMA 262 regex \"^[a-z0-9_\\-.]*$\" does not match input string \"%s\"".formatted(value);
    }

    private static String taskIdPathErrorTemplate(String value) {
        return String.format("ECMA 262 regex \"^[a-zA-Z0-9_\\- .~,()\\[\\]\\{\\}+=#$@!]+(/{1}([a-zA-Z0-9_\\- .~," +
                "()\\[\\]\\{\\}+=#$@!](?!(\\.yaml$)|(\\.yml$)))+)*$\" does not match input string \"%s\"", value);
    }

    private static String releaseBranchPatternErrorTemplate(String value) {
        return String.format("ECMA 262 regex \"^releases/[a-zA-Z0-9_-]+/([a-zA-Z-0-9_\\.$\\{\\}-]+/?)+$\" " +
                "does not match input string \"%s\"", value);
    }
}
