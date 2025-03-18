package ru.yandex.ci.engine.config;

import java.nio.file.Path;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.NavigableMap;
import java.util.Set;
import java.util.TreeMap;

import com.google.common.base.Preconditions;
import com.google.common.collect.Maps;
import lombok.extern.slf4j.Slf4j;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.arc.api.Repo.ChangelistResponse.ChangeType;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.ConfigProblem;
import ru.yandex.ci.core.config.registry.TaskId;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.engine.config.validation.InputOutputTaskValidator;
import ru.yandex.ci.engine.config.validation.InputOutputTaskValidator.ReportId;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyMap;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.verifyNoInteractions;
import static org.mockito.Mockito.when;

@Slf4j
class ConfigParseServiceTest extends EngineTestBase {

    @BeforeEach
    void setUp() {
        mockValidationSuccessful();
    }

    @AfterEach
    void tearDown() {
        arcServiceStub.resetAndInitTestData();
    }

    @Test
    void testNonCiConfig() {
        ConfigParseResult parseResult = configParseService.parseAndValidate(
                TestData.CONFIG_PATH_NOT_CI, TestData.TRUNK_R6.toRevision(), null
        );

        assertThat(parseResult.getStatus()).isEqualTo(ConfigParseResult.Status.NOT_CI);
        assertThat(parseResult.getAYamlConfig()).isNotNull();
    }

    @Test
    void testDummyJob() {
        Path configPath = Path.of("dummy-job/a.yaml");
        ArcCommit createdCommit = arcServiceStub.addTrunkCommit("ayaml", Map.of(configPath, ChangeType.Add))
                .getCreated();


        NavigableMap<TaskId, ArcRevision> taskRevisions = new TreeMap<>();
        taskRevisions.put(
                TaskId.of("demo/woodflow/woodcutter"),
                TestData.TRUNK_R2.toRevision()
        );

        ConfigParseResult result = configParseService.parseAndValidate(
                configPath,
                createdCommit.getRevision(),
                taskRevisions
        );

        assertThat(result.getStatus()).isEqualTo(ConfigParseResult.Status.VALID);
    }

    @Test
    void flowVarsUi() {
        reset(taskValidator);

        Path configPath = Path.of("flow-vars-ui/in-title/a.yaml");
        ArcCommit createdCommit = arcServiceStub.addTrunkCommit("ayaml", Map.of(configPath, ChangeType.Add))
                .getCreated();

        ConfigParseResult result = configParseService.parseAndValidate(
                configPath,
                createdCommit.getRevision(),
                Maps.newTreeMap()
        );

        assertThat(result.getStatus()).isEqualTo(ConfigParseResult.Status.VALID);
    }

    @Test
    void flowVarsUiNotDeclaredRequired() {
        reset(taskValidator);

        Path configPath = Path.of("flow-vars-ui/not-declared-required/a.yaml");
        ArcCommit createdCommit = arcServiceStub.addTrunkCommit("ayaml", Map.of(configPath, ChangeType.Add))
                .getCreated();

        ConfigParseResult result = configParseService.parseAndValidate(
                configPath,
                createdCommit.getRevision(),
                Maps.newTreeMap()
        );

        assertThat(result.getStatus()).isEqualTo(ConfigParseResult.Status.INVALID);
    }

    @Test
    void testAutoRollbackConfig() {
        Path configPath = Path.of("auto-release-job/a.yaml");

        ArcCommit createdCommit = arcServiceStub.addTrunkCommit("ayaml", Map.of(configPath, ChangeType.Add))
                .getCreated();

        var taskRevisions = new TreeMap<>(Map.of(
                TaskId.of("demo/woodflow/woodcutter"), TestData.TRUNK_R2.toRevision(),
                TaskId.of("demo/woodflow/woodcutter_allow"), TestData.TRUNK_R2.toRevision(),
                TaskId.of("demo/woodflow/woodcutter_skip"), TestData.TRUNK_R2.toRevision()
        ));

        ConfigParseResult result = configParseService.parseAndValidate(
                configPath,
                createdCommit.getRevision(),
                taskRevisions
        );

        assertThat(configParseService.parseAndValidate(
                configPath,
                createdCommit.getRevision(),
                taskRevisions
        )).isNotSameAs(result);

        log.info("Result: {}", result.getProblems());
        assertThat(result.getStatus()).isEqualTo(ConfigParseResult.Status.INVALID);


        var title = "Rollbacks";
        var pattern = "Unable to generate rollback flow for release %s, flow %s: %s";

        assertThat(result.getProblems())
                .isEqualTo(List.of(
                        ConfigProblem.crit(title, String.format(pattern, "r1-1", "f1",
                                "task demo/woodflow/woodcutter does not allow rollbacks")),
                        ConfigProblem.crit(title, String.format(pattern, "r1-2", "f1-2",
                                "no actual tasks to run")),
                        ConfigProblem.crit(title, String.format(pattern, "r4-1", "f3",
                                "no actual tasks to run")),
                        ConfigProblem.crit(title,
                                String.format("Invalid rollback configuration for release %s: rollback flows " +
                                                "will not work without at least one stage with 'rollback' attribute",
                                        "r5"))
                ));

        Preconditions.checkState(result.getAYamlConfig() != null, "Config must exists: %s", configPath);
        var ci = result.getAYamlConfig().getCi();
        assertThat(ci.getFlows().keySet())
                .isEqualTo(Set.of(
                        "f1", "f1-2", "f2", "f2-2", "f3",
                        "rollback_r2_f2", "rollback_r2-1_f2", "rollback_r2-1_f2-2", "rollback_r3_f3"));

        // rollback for r2 - skip stage s1

        var f2 = ci.getFlow("f2");
        assertThat(f2.getTitle()).isEqualTo("Flow2");

        var ar2 = ci.getFlow("rollback_r2_f2");
        assertThat(ar2).isNotSameAs(f2);
        assertThat(ar2.getId()).isEqualTo("rollback_r2_f2");
        assertThat(ar2.getTitle()).isEqualTo("Flow2");

        var ar2p1 = ar2.getJob("prepare1");
        assertThat(ar2p1.getTitle()).isEqualTo("Skip by Stage: Prepare 1");
        assertThat(ar2p1.getRunIf()).isEqualTo("${`false`}");

        var f2p1 = f2.getJob("prepare1");
        assertThat(ar2p1).isNotSameAs(f2p1);
        assertThat(ar2p1).isNotEqualTo(f2p1);

        var ar2p2 = ar2.getJob("prepare2");
        var f2p2 = f2.getJob("prepare2");
        assertThat(ar2p2).isSameAs(f2p2);

        var ar2f = ar2.getJob("finish");
        var f2f = f2.getJob("finish");
        assertThat(ar2f).isSameAs(f2f);


        // rollback for r3 - skip task prepare1

        var f3 = ci.getFlow("f3");
        assertThat(f3.getTitle()).isEqualTo("Flow3");

        var ar3 = ci.getFlow("rollback_r3_f3");
        assertThat(ar3).isNotSameAs(f3);
        assertThat(ar3.getId()).isEqualTo("rollback_r3_f3");
        assertThat(ar3.getTitle()).isEqualTo("Flow3");

        var ar3p1 = ar3.getJob("prepare1");
        assertThat(ar3p1.getTitle()).isEqualTo("Skip by Registry: prepare1");
        assertThat(ar3p1.getRunIf()).isEqualTo("${`false`}");

        var f3p1 = f3.getJob("prepare1");
        assertThat(ar3p1).isNotSameAs(f3p1);
        assertThat(ar3p1).isNotEqualTo(f3p1);

        var ar3p2 = ar3.getJob("prepare2");
        var f3p2 = f3.getJob("prepare2");
        assertThat(ar3p2).isSameAs(f3p2);

        var ar3f = ar3.getJob("finish");
        var f3f = f3.getJob("finish");
        assertThat(ar3f).isSameAs(f3f);
    }

    @Test
    void testNoFlow() {
        ConfigParseResult parseResult = configParseService.parseAndValidate(
                TestData.CONFIG_PATH_NO_FLOWS, TestData.TRUNK_R2.toRevision(), null
        );

        assertThat(parseResult.getStatus()).isEqualTo(ConfigParseResult.Status.VALID);
    }

    @Test
    void invalidTaskInRegistry() {
        ConfigParseResult parseResult = configParseService.parseAndValidate(
                TestData.CONFIG_PATH_PR_NEW, TestData.DS7_REVISION, null
        );
        assertThat(parseResult.getStatus()).isEqualTo(ConfigParseResult.Status.INVALID);
        assertThat(parseResult.getProblems()).isEqualTo(List.of(
                ConfigProblem.crit(
                        "Failed to parse Task configuration for task common/invalid-task on revision ds7",
                        "Cannot construct instance of `java.util.LinkedHashMap` (although at least one Creator " +
                                "exists): no String-argument constructor/factory method to deserialize from String " +
                                "value ('<<<<<<< ci/registry/common/misc/run_command.yaml')\n" +
                                " at [Source: (StringReader); line: 8, column: 11] (through reference chain: ru" +
                                ".yandex.ci.core.config.registry.TaskConfig$Builder[\"versions\"])"
                )
        ));
    }

    @Test
    void testWarnTaskletsOutdated() {
        var path = TestData.CONFIG_PATH_NO_FLOWS;
        var flowViolations = InputOutputTaskValidator.FlowViolations.builder()
                .outdatedTasklet("projects/alice/CreateBetaFromTemplate")
                .outdatedTasklet("projects/answers/check_stress_snapshot")
                .build();

        when(taskValidator.validate(eq(path), any(), anyMap()))
                .thenReturn(new InputOutputTaskValidator.Report(
                        Map.of(ReportId.of("some-flow", CiProcessId.Type.FLOW), flowViolations),
                        new LinkedHashMap<>()
                ));

        ConfigParseResult parseResult = configParseService.parseAndValidate(
                path, TestData.TRUNK_R2.toRevision(), null
        );

        assertThat(parseResult.getStatus()).isEqualTo(ConfigParseResult.Status.VALID);
        assertThat(parseResult.getProblems()).isEqualTo(List.of(
                ConfigProblem.warn(
                        "Outdated tasklets are used",
                        "Config uses outdated tasklets which require glycine_token. Please update following tasklets:" +
                                " projects/alice/CreateBetaFromTemplate, projects/answers/check_stress_snapshot" +
                                " (https://nda.ya.ru/t/UpQ8C0r54AMcui)"
                )
        ));
    }

    @Test
    void testWarnTasksDeprecated() {
        var path = TestData.CONFIG_PATH_NO_FLOWS;
        var deprecatedTasks = new LinkedHashMap<String, String>();
        deprecatedTasks.put("common/deploy/release1", "Consider migration");
        deprecatedTasks.put("common/deploy/release2", "Is totally deprecated");
        when(taskValidator.validate(eq(path), any(), anyMap()))
                .thenReturn(new InputOutputTaskValidator.Report(
                        Map.of(),
                        deprecatedTasks)
                );

        ConfigParseResult parseResult = configParseService.parseAndValidate(
                path, TestData.TRUNK_R2.toRevision(), null
        );

        assertThat(parseResult.getStatus()).isEqualTo(ConfigParseResult.Status.VALID);
        assertThat(parseResult.getProblems()).isEqualTo(List.of(
                ConfigProblem.warn(
                        "Deprecated tasks found",
                        " * Task common/deploy/release1 is deprecated. Consider migration\n" +
                                " * Task common/deploy/release2 is deprecated. Is totally deprecated"
                )
        ));
    }

    @Test
    void testNoTaskDefinition() {
        ConfigParseResult parseResult = configParseService.parseAndValidate(
                TestData.CONFIG_PATH_MISSING_TASK_DEFINITION, TestData.TRUNK_R2.toRevision(), null
        );
        assertThat(parseResult.getStatus()).isEqualTo(ConfigParseResult.Status.INVALID);
        verifyNoInteractions(taskValidator);
        assertThat(parseResult.getProblems()).containsExactlyInAnyOrder(
                ConfigProblem.crit("Task path/not/exists not found"),
                ConfigProblem.crit(
                        "Failed to parse Task configuration for task a-yaml-as-dir/dir on revision r2",
                        "yaml for a-yaml-as-dir/dir expected at ci/registry/a-yaml-as-dir/dir.yaml in " +
                                "revision r2 not found"
                )
        );
    }

    @Test
    void invalidYaml() {
        ConfigParseResult parseResult = configParseService.parseAndValidate(
                TestData.CONFIG_PATH_INVALID_YAML_PARSE, TestData.TRUNK_R2.toRevision(), null
        );
        assertThat(parseResult.getStatus()).isEqualTo(ConfigParseResult.Status.INVALID);
        assertThat(parseResult.getProblems()).isEqualTo(List.of(
                ConfigProblem.crit("Failed to parse a.yaml", """
                        while scanning a simple key
                         in 'string', line 13, column 9:
                                    job2:2
                                    ^
                        could not find expected ':'
                         in 'string', line 14, column 9:
                                    title: job
                                    ^
                        """)
        ));
    }

    @Test
    void internalValidationException() {
        ConfigParseResult parseResult = configParseService.parseAndValidate(
                TestData.CONFIG_PATH_INVALID_INTERNAL_EXCEPTION, TestData.TRUNK_R2.toRevision(), null
        );
        assertThat(parseResult.getStatus()).isEqualTo(ConfigParseResult.Status.INVALID);
        assertThat(parseResult.getProblems()).isEqualTo(List.of(
                ConfigProblem.crit(
                        "Internal error while validating config. Please contact CI team",
                        "ru.yandex.ci.core.config.a.validation.AYamlValidationInternalException: " +
                                "java.lang.IllegalStateException: Just for test")
        ));
    }

    @Test
    void testSandboxTemplateFromCommon() {
        ConfigParseResult parseResult = configParseService.parseAndValidate(
                TestData.CONFIG_PATH_WITH_SANDBOX_TEMPLATE_DEFINITION, TestData.TRUNK_R2.toRevision(), null
        );
        assertThat(parseResult.getStatus()).isEqualTo(ConfigParseResult.Status.INVALID);
        assertThat(parseResult.getProblems()).isEqualTo(List.of(
                ConfigProblem.crit("Invalid registry task common/sawmill_sandbox_template, " +
                        "all tasks located in common/ must not use Sandbox Templates")
        ));
    }


    @Test
    void invalidAbcSlug() {
        ConfigParseResult parseResult = configParseService.parseAndValidate(
                TestData.CONFIG_PATH_UNKNOWN_ABC, TestData.TRUNK_R2.toRevision(), null
        );
        assertThat(parseResult.getStatus()).isEqualTo(ConfigParseResult.Status.INVALID);
        assertThat(parseResult.getProblems()).isEqualTo(List.of(
                ConfigProblem.crit("ABC service is invalid", "Unable to find ABC service: unknown-abc-slug")
        ));
    }
}
