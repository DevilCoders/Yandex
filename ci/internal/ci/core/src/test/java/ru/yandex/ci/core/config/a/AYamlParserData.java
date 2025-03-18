package ru.yandex.ci.core.config.a;

import java.time.DayOfWeek;
import java.time.Duration;
import java.time.LocalTime;
import java.time.ZoneId;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import ru.yandex.ci.client.sandbox.api.SandboxTaskStatus;
import ru.yandex.ci.core.config.a.model.AYamlConfig;
import ru.yandex.ci.core.config.a.model.ActionConfig;
import ru.yandex.ci.core.config.a.model.AutocheckConfig;
import ru.yandex.ci.core.config.a.model.Backoff;
import ru.yandex.ci.core.config.a.model.CiConfig;
import ru.yandex.ci.core.config.a.model.FilterConfig;
import ru.yandex.ci.core.config.a.model.FlowConfig;
import ru.yandex.ci.core.config.a.model.FlowWithFlowVars;
import ru.yandex.ci.core.config.a.model.JobAttemptsConfig;
import ru.yandex.ci.core.config.a.model.JobAttemptsSandboxConfig;
import ru.yandex.ci.core.config.a.model.JobAttemptsTaskletConfig;
import ru.yandex.ci.core.config.a.model.JobConfig;
import ru.yandex.ci.core.config.a.model.LargeAutostartConfig;
import ru.yandex.ci.core.config.a.model.ManualConfig;
import ru.yandex.ci.core.config.a.model.NativeBuildConfig;
import ru.yandex.ci.core.config.a.model.PermissionForOwner;
import ru.yandex.ci.core.config.a.model.PermissionRule;
import ru.yandex.ci.core.config.a.model.PermissionScope;
import ru.yandex.ci.core.config.a.model.Permissions;
import ru.yandex.ci.core.config.a.model.ReleaseBranchesConfig;
import ru.yandex.ci.core.config.a.model.ReleaseConfig;
import ru.yandex.ci.core.config.a.model.RuntimeConfig;
import ru.yandex.ci.core.config.a.model.RuntimeSandboxConfig;
import ru.yandex.ci.core.config.a.model.SoxConfig;
import ru.yandex.ci.core.config.a.model.StageConfig;
import ru.yandex.ci.core.config.a.model.TrackerWatchConfig;
import ru.yandex.ci.core.config.a.model.TriggerConfig;
import ru.yandex.ci.core.config.a.model.auto.AutoReleaseConfig;
import ru.yandex.ci.core.config.a.model.auto.Conditions;
import ru.yandex.ci.core.config.a.model.auto.DayType;
import ru.yandex.ci.core.config.a.model.auto.Schedule;
import ru.yandex.ci.core.config.a.model.auto.TimeRange;
import ru.yandex.ci.core.config.registry.RequirementsConfig;
import ru.yandex.ci.core.config.registry.sandbox.SandboxConfig;
import ru.yandex.ci.core.config.registry.sandbox.TaskPriority;
import ru.yandex.ci.util.gson.JsonObjectBuilder;
import ru.yandex.tasklet.api.v2.DataModel;

public class AYamlParserData {

    private AYamlParserData() {
        //
    }

    static AYamlConfig releaseAndSawmillAYaml() {

        CiConfig ciConfig = CiConfig.builder()
                .release(releaseConfig())
                .triggers(sawmillTriggers())
                .flow(sawmillFlow(), releaseFlow())
                .secret("sec-01dy7t26dyht1bj4w3yn94fsa")
                .runtime(runtime())
                .build();
        return new AYamlConfig("ci", "Arcadia", ciConfig, null);
    }

    static RuntimeConfig runtime() {
        return RuntimeConfig.of(RuntimeSandboxConfig.ofOwner("CI"));
    }

    static AYamlConfig sawmillAYaml() {

        CiConfig ciConfig = CiConfig.builder()
                .triggers(sawmillTriggers())
                .flow(sawmillFlow())
                .secret("sec-01dy7t26dyht1bj4w3yn94fsa")
                .runtime(runtime())
                .build();
        return new AYamlConfig(
                "ci", "Woodcutter", ciConfig,
                SoxConfig.builder()
                        .approvalScope("sox-scope")
                        .build()
        );

    }

    static AYamlConfig manualTestAYaml() {
        CiConfig ciConfig = CiConfig.builder()
                .flow(manualTestFlow())
                .secret("sec-01dy7t26dyht1bj4w3yn94fsa")
                .runtime(RuntimeConfig.of(RuntimeSandboxConfig.builder()
                                .owner("CI")
                                .keepPollingStopped(true)
                                .build())
                        .withGetOutputOnFail(true))
                .build();
        return new AYamlConfig("ci", "Manual Test", ciConfig, null);
    }

    static AYamlConfig singleFastTargetTestAYaml() {
        AutocheckConfig autocheckConfig = AutocheckConfig.builder()
                .fastTargets(List.of("path"))
                .build();

        CiConfig ciConfig = CiConfig.builder()
                .flow(getSimpleValidTestFlow())
                .secret("sec-01dy7t26dyht1bj4w3yn94fsa")
                .runtime(runtime())
                .autocheck(autocheckConfig)
                .build();
        return new AYamlConfig("ci", "Single Fast Target Test", ciConfig, null);
    }

    static AYamlConfig fastTargetsTestAYaml() {
        AutocheckConfig autocheckConfig = AutocheckConfig.builder()
                .fastTargets(List.of(
                        "path1/subpath1/subpath2",
                        "path2/subpath",
                        "path3"
                ))
                .build();
        CiConfig ciConfig = CiConfig.builder()
                .flow(getSimpleValidTestFlow())
                .secret("sec-01dy7t26dyht1bj4w3yn94fsa")
                .runtime(runtime())
                .autocheck(autocheckConfig)
                .build();
        return new AYamlConfig("ci", "Fast Targets Test", ciConfig, null);
    }

    static AYamlConfig emptyFastTargetsTestAYaml() {
        AutocheckConfig autocheckConfig = AutocheckConfig.builder()
                .fastTargets(List.of())
                .build();
        CiConfig ciConfig = CiConfig.builder()
                .flow(getSimpleValidTestFlow())
                .secret("sec-01dy7t26dyht1bj4w3yn94fsa")
                .runtime(runtime())
                .autocheck(autocheckConfig)
                .build();
        return new AYamlConfig("ci", "Empty Fast Targets Test", ciConfig, null);
    }

    static AYamlConfig noFlowsTestAYaml() {
        CiConfig ciConfig = CiConfig.builder()
                .secret("sec-01dy7t26dyht1bj4w3yn94fsa")
                .runtime(runtime())
                .build();
        return new AYamlConfig("ci", "Woodcutter", ciConfig, null);
    }

    static AYamlConfig autocheckAYaml(AutocheckConfig autocheckConfig) {
        CiConfig ciConfig = CiConfig.builder()
                .secret("sec-01dy7t26dyht1bj4w3yn94fsa")
                .runtime(runtime())
                .autocheck(autocheckConfig)
                .build();
        return new AYamlConfig("ci", "Woodcutter", ciConfig, null);

    }

    static AutocheckConfig autocheckLargeAutostartConfig() {
        return AutocheckConfig.builder()
                .largeSandboxOwner("CI-DEMO")
                .largeAutostart(List.of(
                        new LargeAutostartConfig("some/large/test/target_1", null),
                        new LargeAutostartConfig(
                                "some/large/test/target_2",
                                List.of("default-linux-x86_64-release")
                        ),
                        new LargeAutostartConfig(
                                "some/large/test/target_3",
                                List.of(
                                        "default-linux-x86_64-release-msan",
                                        "default-linux-x86_64-release-asan",
                                        "default-linux-x86_64-release-musl"
                                )
                        ),
                        new LargeAutostartConfig("some/large/test/target-4/*"),
                        new LargeAutostartConfig("some/large/*/target-5")
                ))
                .build();
    }

    static AutocheckConfig autocheckLargeAutostartSimpleConfig() {
        return AutocheckConfig.builder()
                .largeAutostart(List.of(
                        new LargeAutostartConfig("some/large/test/target_1"),
                        new LargeAutostartConfig("some/large/test/target_2"),
                        new LargeAutostartConfig("some/large/test/target_3"),
                        new LargeAutostartConfig("some/large/test/target-4/*"),
                        new LargeAutostartConfig("some/large/*/target-5")
                ))
                .build();
    }

    static AutocheckConfig autocheckNativeBuildsConfig() {
        return AutocheckConfig.builder()
                .nativeSandboxOwner("CI-TEST")
                .nativeBuilds(List.of(
                        new NativeBuildConfig(
                                "msvc2019-x86_64-release",
                                List.of(
                                        "myproject/lib myproject/core myproject/client1",
                                        "myproject/lib myproject/core myproject/client2"
                                )
                        ),
                        new NativeBuildConfig(
                                "msvc2018-x86_64-release",
                                List.of("myproject/lib myproject/core myproject/client2")
                        )
                ))
                .build();
    }

    static ReleaseConfig releaseConfig() {
        return ReleaseConfig.builder()
                .id("my-app").title("My Application").flow("my-app-release")
                .filters(List.of(
                        FilterConfig.builder()
                                .discovery(FilterConfig.Discovery.DIR)
                                .stQueues(List.of("TESTENV"))
                                .build(),
                        FilterConfig.builder()
                                .discovery(FilterConfig.Discovery.GRAPH)
                                .authorServices(List.of("ci", "testenv"))
                                .build(),
                        FilterConfig.builder()
                                .discovery(FilterConfig.Discovery.GRAPH)
                                .stQueues(List.of("CI"))
                                .build()
                ))
                .stages(List.of(
                        StageConfig.builder().id("build").title("Сборка").build(),
                        StageConfig.builder().id("testing").title("Тестинг").build(),
                        StageConfig.builder().id("stable").title("Продакшейн").build()
                ))
                .tag("my-app-tag")
                .tag("my-release-custom-tag")
                .build();
    }

    static List<TriggerConfig> sawmillTriggers() {
        TriggerConfig prTrigger = TriggerConfig.builder()
                .on(TriggerConfig.On.PR)
                .flow("sawmill")
                .filters(List.of(
                        FilterConfig.builder()
                                .discovery(FilterConfig.Discovery.DEFAULT)
                                .stQueues(List.of("CI"))
                                .authorServices(List.of("ci", "testenv"))
                                .build(),
                        FilterConfig.builder()
                                .discovery(FilterConfig.Discovery.DEFAULT)
                                .subPaths(List.of("**.java", "ya.make"))
                                .build(),
                        FilterConfig.builder()
                                .discovery(FilterConfig.Discovery.DEFAULT)
                                .featureBranches(List.of("**/user/**", "**/feature-**"))
                                .build(),
                        FilterConfig.builder()
                                .discovery(FilterConfig.Discovery.DEFAULT)
                                .featureBranches(List.of("**/release-**"))
                                .build()
                ))
                .build();

        TriggerConfig trunkTrigger =
                TriggerConfig.builder()
                        .on(TriggerConfig.On.COMMIT)
                        .flow("sawmill")
                        .build();

        return List.of(prTrigger, trunkTrigger);
    }

    static FlowConfig sawmillFlow() {
        JobConfig woodcutter1Job = JobConfig.builder()
                .id("woodcutter1")
                .title("Лесоруб1")
                .task("example/settlers/woodcutter")
                .version("testing")
                .build();

        JobConfig woodcutter2Job = JobConfig.builder()
                .id("woodcutter2")
                .title("Лесоруб2")
                .task("example/settlers/woodcutter")
                .build();

        JobConfig sawmillJob = JobConfig.builder()
                .id("sawmill")
                .title("Лесопилка")
                .task("example/settlers/sawmill")
                .needs(List.of("woodcutter1", "woodcutter2"))
                .build();

        return FlowConfig.builder()
                .id("sawmill")
                .title("Woodcutter")
                .description("sawmill flow")
                .job(woodcutter1Job, woodcutter2Job, sawmillJob)
                .build();
    }

    static FlowConfig releaseFlow() {
        JobConfig build = JobConfig.builder()
                .id("build")
                .title("Сборка")
                .task("common/arcadia/ya_package")
                .stage("build")
                .input(JsonObjectBuilder.builder()
                        .startMap("config")
                        .withProperty("path", "my/path/in/parcadia/package.json")
                        .withProperty("run_test", true)
                        .end()
                        .build()
                )
                .build();

        JobConfig testing = JobConfig.builder()
                .id("testing")
                .title("Выкладка в тестинг")
                .task("common/deploy/release")
                .needs(List.of("build"))
                .stage("testing")
                .input(JsonObjectBuilder.builder()
                        .startMap("config")
                        .withProperty("stage_id", "my-app-testing")
                        .withProperty("timeout", "1h")
                        .end()
                        .build()
                )
                .build();

        JobConfig stable = JobConfig.builder()
                .id("stable")
                .title("Выкладка в прод")
                .description("Выкладывает в прод")
                .task("common/deploy/release")
                .needs(List.of("testing"))
                .stage("stable")
                .input(JsonObjectBuilder.builder()
                        .startMap("config")
                        .withProperty("stage_id", "my-app-stable")
                        .withProperty("timeout", "1h")
                        .end()
                        .build()
                )
                .contextInput(JsonObjectBuilder.builder()
                        .withProperty("task_id", 335)
                        .build()
                )
                .manual(ManualConfig.builder()
                        .prompt("Точно катим?")
                        .abcService("abc_group")
                        .build())
                .build();

        return FlowConfig.builder()
                .id("my-app-release")
                .title("Default deploy")
                .job(build, testing, stable)
                .build();
    }

    static FlowConfig manualTestFlow() {
        JobConfig testingJob = JobConfig.builder()
                .id("testing")
                .title("Выкладка в тестинг")
                .description("Выкладывает в тестинг")
                .task("common/deploy/release")
                .manual(ManualConfig.of(true))
                .build();

        JobConfig stableJob = JobConfig.builder()
                .id("stable")
                .title("Выкладка в прод")
                .description("Выкладывает в прод")
                .task("common/deploy/release")
                .manual(ManualConfig.builder()
                        .prompt("Точно катим?")
                        .abcService("abc_group")
                        .build())
                .jobRuntimeConfig(RuntimeConfig.builder()
                        .getOutputOnFail(true)
                        .build())
                .build();

        JobConfig stableJob2 = JobConfig.builder()
                .id("stable2")
                .title("Выкладка в прод")
                .description("Выкладывает в прод")
                .task("common/deploy/release")
                .manual(ManualConfig.builder()
                        .prompt("Точно катим?")
                        .approver(PermissionRule.ofScopes("ci", "development"))
                        .build())
                .jobRuntimeConfig(RuntimeConfig.builder()
                        .getOutputOnFail(true)
                        .sandbox(RuntimeSandboxConfig.builder()
                                .keepPollingStopped(false)
                                .build())
                        .build())
                .build();

        return FlowConfig.builder()
                .id("my-app-release")
                .title("Manual Test")
                .job(testingJob, stableJob, stableJob2)
                .build();
    }

    static FlowConfig getSimpleValidTestFlow() {
        JobConfig testingJob = JobConfig.builder()
                .id("testing")
                .title("Выкладка в тестинг")
                .task("common/deploy/release")
                .build();

        return FlowConfig.builder()
                .id("my-app-release")
                .title("Test")
                .job(testingJob)
                .build();
    }

    static List<ReleaseConfig> autoParseReleases() {
        return List.of(
                ReleaseConfig.builder()
                        .id("my-app-1").title("app1").flow("release-flow-common")
                        .filters(
                                List.of(FilterConfig.builder()
                                        .discovery(FilterConfig.Discovery.DIR)
                                        .build()
                                )
                        )
                        .auto(new AutoReleaseConfig(true))
                        .stages(List.of(StageConfig.IMPLICIT_STAGE))
                        .build(),
                ReleaseConfig.builder()
                        .id("my-app-2").title("app2").flow("release-flow-common")
                        .filters(
                                List.of(FilterConfig.builder()
                                        .discovery(FilterConfig.Discovery.DIR)
                                        .build()
                                )
                        )
                        .auto(new AutoReleaseConfig(true))
                        .stages(List.of(StageConfig.IMPLICIT_STAGE))
                        .build(),
                ReleaseConfig.builder()
                        .id("my-app-3").title("app3").flow("release-flow-common")
                        .filters(
                                List.of(FilterConfig.builder()
                                        .discovery(FilterConfig.Discovery.DIR)
                                        .build()
                                )
                        )
                        .auto(new AutoReleaseConfig(false))
                        .stages(List.of(StageConfig.IMPLICIT_STAGE))
                        .build(),
                ReleaseConfig.builder()
                        .id("my-app-4").title("app4").flow("release-flow-common")
                        .filters(
                                List.of(FilterConfig.builder()
                                        .discovery(FilterConfig.Discovery.DIR)
                                        .build()
                                )
                        )
                        .stages(List.of(StageConfig.IMPLICIT_STAGE))
                        .build(),
                ReleaseConfig.builder()
                        .id("with-minimal-commits-array").title("app5").flow("release-flow-common")
                        .auto(new AutoReleaseConfig(true, List.of(
                                        new Conditions(18, null, null),
                                        new Conditions(13, null, null)
                                ))
                        )
                        .stages(List.of(StageConfig.IMPLICIT_STAGE))
                        .build(),
                ReleaseConfig.builder()
                        .id("with-single-condition-deprecated").title("app6 dep").flow("release-flow-common")
                        .auto(new AutoReleaseConfig(true,
                                        List.of(
                                                new Conditions(3,
                                                        new Schedule(
                                                                TimeRange.of(
                                                                        LocalTime.of(15, 0),
                                                                        LocalTime.of(20, 0),
                                                                        ZoneId.of("Europe/Moscow")
                                                                ),
                                                                Set.of(
                                                                        DayOfWeek.MONDAY,
                                                                        DayOfWeek.TUESDAY,
                                                                        DayOfWeek.THURSDAY,
                                                                        DayOfWeek.FRIDAY
                                                                ),
                                                                null
                                                        ),
                                                        Duration.ofDays(2 * 7 + 5).plusMinutes(30).plusSeconds(15))
                                        )
                                )
                        )
                        .stages(List.of(StageConfig.IMPLICIT_STAGE))
                        .build(),
                ReleaseConfig.builder()
                        .id("with-single-condition").title("app6").flow("release-flow-common")
                        .auto(new AutoReleaseConfig(true, new Conditions(6, null, null)))
                        .stages(List.of(StageConfig.IMPLICIT_STAGE))
                        .build(),
                ReleaseConfig.builder()
                        .id("with-conditions").title("app7").flow("release-flow-common")
                        .auto(new AutoReleaseConfig(true,
                                        List.of(
                                                new Conditions(30, null, null),
                                                new Conditions(0, null, Duration.ofDays(1)),
                                                new Conditions(4,
                                                        new Schedule(
                                                                TimeRange.of(
                                                                        LocalTime.of(9, 0),
                                                                        LocalTime.of(19, 30),
                                                                        ZoneId.of("Europe/Moscow")
                                                                ),
                                                                Set.of(
                                                                        DayOfWeek.TUESDAY,
                                                                        DayOfWeek.WEDNESDAY,
                                                                        DayOfWeek.THURSDAY,
                                                                        DayOfWeek.SUNDAY
                                                                ),
                                                                DayType.NOT_PRE_HOLIDAYS
                                                        ),
                                                        Duration.ofHours(4))
                                        )
                                )
                        )
                        .stages(List.of(StageConfig.IMPLICIT_STAGE))
                        .build(),
                ReleaseConfig.builder()
                        .id("with-minimal-commits-but-disabled").title("app8").flow("release-flow-common")
                        .auto(new AutoReleaseConfig(false, new Conditions(9, null, null)))
                        .stages(List.of(StageConfig.IMPLICIT_STAGE))
                        .build()
        );
    }

    static List<ReleaseConfig> releaseBranches() {
        return List.of(
                ReleaseConfig.builder()
                        .id("my-app-1").title("app1").flow("release-flow-common")
                        .branches(
                                ReleaseBranchesConfig.builder()
                                        .pattern("releases/ci/release-ci-2.0-${version}")
                                        .forbidTrunkReleases(true)
                                        .autoCreate(false)
                                        .auto(
                                                new AutoReleaseConfig(
                                                        true, List.of()
                                                )
                                        )
                                        .build()
                        )
                        .stages(List.of(StageConfig.IMPLICIT_STAGE))
                        .build(),
                ReleaseConfig.builder()
                        .id("my-app-2").title("app2").flow("release-flow-common")
                        .branches(
                                ReleaseBranchesConfig.builder()
                                        .pattern("releases/ci/release-ci-2.0-${version}")
                                        .forbidTrunkReleases(false)
                                        .autoCreate(true)
                                        .auto(
                                                new AutoReleaseConfig(
                                                        true, List.of(new Conditions(1, null, null))
                                                )
                                        )
                                        .build()
                        )
                        .stages(List.of(StageConfig.IMPLICIT_STAGE))
                        .build(),
                ReleaseConfig.builder()
                        .id("my-app-3").title("app3").flow("release-flow-common")
                        .branches(
                                ReleaseBranchesConfig.builder()
                                        .pattern("releases/ci/${version}")
                                        .forbidTrunkReleases(false)
                                        .autoCreate(false)
                                        .build()
                        )
                        .stages(List.of(StageConfig.IMPLICIT_STAGE))
                        .build(),
                ReleaseConfig.builder()
                        .id("my-app-4").title("app4").flow("release-flow-common")
                        .hotfixFlows(flowRefs("release-flow-custom-1"))
                        .branches(
                                ReleaseBranchesConfig.builder()
                                        .pattern("releases/experimental/release_machine_tests/rm_test_ci" +
                                                "/stable-${version}")
                                        .build()
                        )
                        .stages(List.of(StageConfig.IMPLICIT_STAGE))
                        .build(),
                ReleaseConfig.builder()
                        .id("my-app-5").title("app5").flow("release-flow-common")
                        .hotfixFlows(flowRefs("release-flow-custom-1", "release-flow-custom-2"))
                        .branches(
                                ReleaseBranchesConfig.builder()
                                        .pattern("releases/ci/fix-${version}")
                                        .build()
                        )
                        .stages(List.of(StageConfig.IMPLICIT_STAGE))
                        .build(),
                ReleaseConfig.builder()
                        .id("my-app-6").title("app6").flow("release-flow-common")
                        .rollbackFlows(flowRefs("release-flow-custom-1", "release-flow-custom-2"))
                        .branches(
                                ReleaseBranchesConfig.builder()
                                        .pattern("releases/ci/fix-${version}")
                                        .build()
                        )
                        .stages(List.of(StageConfig.IMPLICIT_STAGE))
                        .build(),
                ReleaseConfig.builder()
                        .id("my-app-7").title("app7").flow("release-flow-common")
                        .hotfixFlows(flowRefs("release-flow-custom-1", "release-flow-custom-2"))
                        .rollbackFlows(flowRefs("release-flow-custom-1", "release-flow-custom-2"))
                        .branches(
                                ReleaseBranchesConfig.builder()
                                        .pattern("releases/ci/fix-${version}")
                                        .build()
                        )
                        .stages(List.of(StageConfig.IMPLICIT_STAGE))
                        .build()
        );
    }

    static AYamlConfig additionalSecretsSingle() {
        CiConfig ciConfig = CiConfig.builder()
                .secret("sec-01dy7t26dyht1bj4w3yn94fsa")
                .additionalSecrets(List.of("sec-12345"))
                .runtime(runtime())
                .build();
        return new AYamlConfig("ci", "Arcadia", ciConfig, null);
    }

    static AYamlConfig additionalSecretsList() {
        CiConfig ciConfig = CiConfig.builder()
                .secret("sec-01dy7t26dyht1bj4w3yn94fsa")
                .additionalSecrets(List.of("sec-12345", "sec-34567"))
                .runtime(runtime())
                .build();
        return new AYamlConfig("ci", "Arcadia", ciConfig, null);
    }

    static AYamlConfig allPermissions() {
        CiConfig ciConfig = CiConfig.builder()
                .secret("sec-01dy7t26dyht1bj4w3yn94fsa")
                .approvals(List.of(
                        PermissionRule.ofScopes("ci-1"),
                        PermissionRule.ofScopes("ci-1", "s-1"),
                        PermissionRule.ofScopes("ci-2", "s-1")
                ))
                .permissions(Permissions.builder()
                        .defaultPermissionsForOwner(List.of())
                        .add(PermissionScope.START_FLOW, PermissionRule.ofScopes("ci-1"))
                        .add(PermissionScope.START_HOTFIX, PermissionRule.ofScopes("ci-2"))
                        .add(PermissionScope.CANCEL_FLOW, PermissionRule.ofScopes("ci-3"))
                        .add(PermissionScope.ROLLBACK_FLOW,
                                PermissionRule.ofScopes("ci-4"), PermissionRule.ofScopes("ci-5"))
                        .add(PermissionScope.ADD_COMMIT,
                                PermissionRule.ofScopes("ci-6", "development"))
                        .add(PermissionScope.CREATE_BRANCH,
                                PermissionRule.ofScopes("ci-7", "development", "administration"))
                        .add(PermissionScope.START_JOB,
                                PermissionRule.ofScopes("ci-8", "development", "administration"),
                                PermissionRule.ofScopes("ci-9", "dutywork"))
                        .add(PermissionScope.KILL_JOB,
                                PermissionRule.ofScopes("ci-10"),
                                PermissionRule.ofScopes("ci-10", "development"),
                                PermissionRule.ofRoles("ci-10", "Development"),
                                PermissionRule.ofRoles("ci-10", "Development2"),
                                PermissionRule.ofDuties("ci-10", "task-duty"),
                                PermissionRule.ofDuties("ci-10", "2267"),
                                PermissionRule.ofRoles("ci-10-1", "Development"),
                                PermissionRule.ofDuties("ci-10-2", "22671"))
                        .add(PermissionScope.SKIP_JOB,
                                PermissionRule.ofScopes("ci-11", "development"),
                                PermissionRule.ofScopes("ci-11", "administration"),
                                PermissionRule.of("ci-11", List.of("support"), List.of("Support"), List.of("2265")),
                                PermissionRule.of("ci-11", List.of("support2"), List.of("Support2"), List.of("22652")))
                        .add(PermissionScope.TOGGLE_AUTORUN,
                                PermissionRule.ofScopes("ci-12", "development"))
                        .add(PermissionScope.MANUAL_TRIGGER, PermissionRule.ofScopes("ci-13"))
                        .build())
                .action(ActionConfig.builder()
                        .id("action-1")
                        .flow("flow-1")
                        .permissions(Permissions.builder()
                                .add(PermissionScope.START_FLOW, PermissionRule.ofScopes("ci-1-1"))
                                .build())
                        .build())
                .action(ActionConfig.builder()
                        .id("action-2")
                        .flow("flow-1")
                        .permissions(Permissions.builder()
                                .defaultPermissionsForOwner(List.of(PermissionForOwner.PR, PermissionForOwner.COMMIT))
                                .build())
                        .build())
                .release(ReleaseConfig.builder()
                        .id("release-1")
                        .flow("flow-1")
                        .permissions(Permissions.builder()
                                .defaultPermissionsForOwner(List.of(PermissionForOwner.RELEASE))
                                .add(PermissionScope.START_FLOW, PermissionRule.ofScopes("citest-1"))
                                .add(PermissionScope.START_HOTFIX, PermissionRule.ofScopes("citest-2"))
                                .add(PermissionScope.CANCEL_FLOW, PermissionRule.ofScopes("citest-3"))
                                .add(PermissionScope.ROLLBACK_FLOW, PermissionRule.ofScopes("citest-4"))
                                .add(PermissionScope.ADD_COMMIT, PermissionRule.ofScopes("citest-5"))
                                .add(PermissionScope.CREATE_BRANCH, PermissionRule.ofScopes("citest-6"))
                                .add(PermissionScope.START_JOB, PermissionRule.ofScopes("citest-7"))
                                .add(PermissionScope.KILL_JOB, PermissionRule.ofScopes("citest-8"))
                                .add(PermissionScope.SKIP_JOB, PermissionRule.ofScopes("citest-9"))
                                .add(PermissionScope.MANUAL_TRIGGER, PermissionRule.ofScopes("citest-10"))
                                .add(PermissionScope.TOGGLE_AUTORUN, PermissionRule.ofScopes("citest-11"))
                                .build())
                        .build())
                .flow(FlowConfig.builder()
                        .id("flow-1")
                        .job(JobConfig.builder()
                                .id("dummy")
                                .title("dummy")
                                .task("dummy")
                                .stage("single")
                                .build())
                        .build())
                .runtime(runtime())
                .build();
        return new AYamlConfig("ci", "All Permissions", ciConfig, null);
    }

    static AYamlConfig trackerWatcher() {
        CiConfig ciConfig = CiConfig.builder()
                .secret("sec-01dy7t26dyht1bj4w3yn94fsa")
                .runtime(RuntimeConfig.ofSandboxOwner("CI"))
                .action(ActionConfig.builder()
                        .id("action")
                        .flow("flow")
                        .trackerWatchConfig(TrackerWatchConfig.builder()
                                .queue("TOARCADIA")
                                .issues(Set.of("TOARCADIA-1"))
                                .status("readyForStart")
                                .closeStatuses(List.of("open", "closed"))
                                .flowVar("issue")
                                .secret(TrackerWatchConfig.YavSecretSpec.builder()
                                        .key("tracker.key")
                                        .build())
                                .build())
                        .build())
                .flow(FlowConfig.builder()
                        .id("flow")
                        .job(JobConfig.builder()
                                .id("job")
                                .task("dummy")
                                .build())
                        .build())
                .build();
        return new AYamlConfig("ci", "Tracker Watcher", ciConfig, null);
    }

    static AYamlConfig jobAttempts() {
        CiConfig ciConfig = CiConfig.builder()
                .secret("sec-01dy7t26dyht1bj4w3yn94fsa")
                .runtime(RuntimeConfig.ofSandboxOwner("CI"))
                .flow(FlowConfig.builder()
                        .id("release-flow-common")
                        .title("Woodcutter")
                        .description("Sawmill flow")
                        .job(JobConfig.builder()
                                .id("inline")
                                .title("Лесоруб")
                                .task("example/settlers/woodcutter")
                                .attempts(JobAttemptsConfig.ofAttempts(5))
                                .build())
                        .job(JobConfig.builder()
                                .id("detailed")
                                .title("Лесоруб")
                                .task("example/settlers/woodcutter")
                                .needs(List.of("inline"))
                                .attempts(JobAttemptsConfig.builder()
                                        .maxAttempts(17)
                                        .backoff(Backoff.EXPONENTIAL)
                                        .initialBackoff(Duration.ofMinutes(5))
                                        .maxBackoff(Duration.ofMinutes(40))
                                        .sandboxConfig(JobAttemptsSandboxConfig.builder()
                                                .excludeStatus(SandboxTaskStatus.STOPPED)
                                                .build())
                                        .build())
                                .build())
                        .build())
                .build();
        return new AYamlConfig("ci", "Arcadia", ciConfig, null);
    }

    static AYamlConfig jobAttemptsConditional() {
        CiConfig ciConfig = CiConfig.builder()
                .secret("sec-01dy7t26dyht1bj4w3yn94fsa")
                .runtime(RuntimeConfig.ofSandboxOwner("CI"))
                .flow(FlowConfig.builder()
                        .id("release-flow-common")
                        .title("Woodcutter")
                        .description("Sawmill flow")
                        .job(JobConfig.builder()
                                .id("inline")
                                .title("Лесоруб")
                                .task("example/settlers/woodcutter")
                                .attempts(JobAttemptsConfig.ofAttempts(5))
                                .build())
                        .job(JobConfig.builder()
                                .id("detailed")
                                .title("Лесоруб")
                                .task("example/settlers/woodcutter")
                                .needs(List.of("inline"))
                                .attempts(JobAttemptsConfig.builder()
                                        .maxAttempts(17)
                                        .ifOutput("${length(output_params.timbers) == 0}")
                                        .build())
                                .build())
                        .build())
                .build();
        return new AYamlConfig("ci", "Arcadia", ciConfig, null);
    }

    static AYamlConfig jobAttemptsReuse() {
        CiConfig ciConfig = CiConfig.builder()
                .secret("sec-01dy7t26dyht1bj4w3yn94fsa")
                .runtime(RuntimeConfig.ofSandboxOwner("CI"))
                .flow(FlowConfig.builder()
                        .id("release-flow-common")
                        .title("Woodcutter")
                        .description("Sawmill flow")
                        .job(JobConfig.builder()
                                .id("inline")
                                .title("Лесоруб")
                                .task("example/settlers/woodcutter")
                                .attempts(JobAttemptsConfig.ofAttempts(5))
                                .build())
                        .job(JobConfig.builder()
                                .id("detailed")
                                .title("Лесоруб")
                                .task("example/settlers/woodcutter")
                                .needs(List.of("inline"))
                                .attempts(JobAttemptsConfig.builder()
                                        .maxAttempts(4)
                                        .sandboxConfig(JobAttemptsSandboxConfig.builder()
                                                .reuseTasks(true)
                                                .useAttempts(3)
                                                .excludeStatus(SandboxTaskStatus.STOPPED)
                                                .build())
                                        .build())
                                .build())
                        .job(JobConfig.builder()
                                .id("misc")
                                .title("Лесоруб")
                                .task("example/settlers/woodcutter")
                                .needs(List.of("detailed"))
                                .attempts(JobAttemptsConfig.builder()
                                        .maxAttempts(4)
                                        .sandboxConfig(JobAttemptsSandboxConfig.builder()
                                                .reuseTasks(true)
                                                .excludeStatus(SandboxTaskStatus.STOPPED)
                                                .build())
                                        .build())
                                .build())
                        .build())
                .build();
        return new AYamlConfig("ci", "Arcadia", ciConfig, null);
    }

    static AYamlConfig jobAttemptsTaskletV2() {
        CiConfig ciConfig = CiConfig.builder()
                .secret("sec-01dy7t26dyht1bj4w3yn94fsa")
                .runtime(RuntimeConfig.ofSandboxOwner("CI"))
                .flow(FlowConfig.builder()
                        .id("release-flow-common")
                        .title("Woodcutter")
                        .description("Sawmill flow")
                        .job(JobConfig.builder()
                                .id("inline")
                                .title("Лесоруб")
                                .task("example/settlers/woodcutter")
                                .attempts(JobAttemptsConfig.ofAttempts(5))
                                .build())
                        .job(JobConfig.builder()
                                .id("detailed")
                                .title("Лесоруб")
                                .task("example/settlers/woodcutter")
                                .needs(List.of("inline"))
                                .attempts(JobAttemptsConfig.builder()
                                        .maxAttempts(17)
                                        .backoff(Backoff.EXPONENTIAL)
                                        .initialBackoff(Duration.ofMinutes(5))
                                        .maxBackoff(Duration.ofMinutes(40))
                                        .taskletConfig(JobAttemptsTaskletConfig.builder()
                                                .excludeServerError(DataModel.ErrorCodes.ErrorCode.ERROR_CODE_ABORTED)
                                                .build())
                                        .build())
                                .build())
                        .build())
                .build();
        return new AYamlConfig("ci", "Arcadia", ciConfig, null);
    }

    static AYamlConfig jobRequirementsPriority() {
        CiConfig ciConfig = CiConfig.builder()
                .secret("sec-01dy7t26dyht1bj4w3yn94fsa")
                .runtime(RuntimeConfig.ofSandboxOwner("CI"))
                .requirements(RequirementsConfig.builder()
                        .withSandbox(SandboxConfig.builder()
                                .priority(new TaskPriority("BACKGROUND:HIGH"))
                                .build())
                        .build())
                .flow(FlowConfig.builder()
                        .id("my-flow")
                        .job(JobConfig.builder()
                                .id("job")
                                .task("dummy")
                                .requirements(RequirementsConfig.builder()
                                        .withSandbox(SandboxConfig.builder()
                                                .priority(new TaskPriority("${flow-vars.priority}"))
                                                .build())
                                        .build())
                                .build())
                        .build())
                .build();
        return new AYamlConfig("ci", "Requirements Test", ciConfig, null);
    }

    private static List<FlowWithFlowVars> flowRefs(String... flowIds) {
        return Stream.of(flowIds).map(FlowWithFlowVars::new).collect(Collectors.toList());
    }
}
