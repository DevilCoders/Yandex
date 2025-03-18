package ru.yandex.ci.core.test;

import java.nio.file.Path;
import java.time.Instant;
import java.util.List;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.ConfigSecurityState;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.config.a.AYamlService;
import ru.yandex.ci.core.config.registry.TaskId;
import ru.yandex.ci.core.discovery.DiscoveredCommit;
import ru.yandex.ci.core.discovery.DiscoveredCommitState;
import ru.yandex.ci.core.launch.Launch;
import ru.yandex.ci.core.launch.LaunchFlowInfo;
import ru.yandex.ci.core.launch.LaunchRuntimeInfo;
import ru.yandex.ci.core.launch.LaunchVcsInfo;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.pr.PullRequest;
import ru.yandex.ci.core.pr.PullRequestDiffSet;
import ru.yandex.ci.core.pr.PullRequestVcsInfo;
import ru.yandex.ci.core.security.YavToken;

public class TestData {

    public static final Instant COMMIT_DATE = Instant.ofEpochMilli(1594676509042L);

    public static final String CI_USER = "andreevdm";
    public static final String USER42 = "user42";

    public static final Path FAST_TARGETS_ROOT_AYAML = Path.of(
            "autocheck.fast.targets/a.yaml"
    );

    public static final Path FAST_TARGETS_FIRST_INVALID_FIRST_AYAML_PATH = Path.of(
            "autocheck.fast.targets/first.invalid/a.yaml"
    );

    public static final Path AUTOSTART_LARGE_TESTS_SIMPLE_AYAML_PATH = Path.of(
            "autocheck.autostart.large.tests/simple/a.yaml"
    );
    public static final Path AUTOSTART_LARGE_TESTS_FIRST_INVALID_AYAML_PATH = Path.of(
            "autocheck.autostart.large.tests/first.invalid/a.yaml"
    );

    public static final Path TRIGGER_ON_COMMIT_AYAML_PATH = Path.of("trigger-on-commit/a.yaml");
    public static final Path INTERNAL_CHANGE_AYAML_PATH = Path.of("pr/internal/change/a.yaml");
    public static final Path DUMMY_RELEASE_AYAML_PATH = Path.of("release/dummy/a.yaml");
    public static final Path DISCOVERY_DIR_ON_COMMIT_AYAML_PATH = Path.of("discovery-dir-{{on-commit}}/a.yaml");
    public static final Path DISCOVERY_DIR_ON_COMMIT2_AYAML_PATH = Path.of("discovery-dir-{{on-commit2}}/a.yaml");

    public static final Path NATIVE_BUILDS_ROOT_AYAML = Path.of(
            "autocheck.native.builds/a.yaml"
    );

    public static final Path NATIVE_BUILDS_FIRST_INVALID_FIRST_AYAML_PATH = Path.of(
            "autocheck.native.builds/first.invalid/a.yaml"
    );

    public static final Path CONFIG_PATH_ABC = AYamlService.dirToConfigPath("a/b/c");
    public static final Path CONFIG_PATH_ABD = AYamlService.dirToConfigPath("a/b/d");
    public static final Path CONFIG_PATH_ABE = AYamlService.dirToConfigPath("a/b/e");
    public static final Path CONFIG_PATH_ABF = AYamlService.dirToConfigPath("a/b/f");
    public static final Path CONFIG_PATH_ABJ = AYamlService.dirToConfigPath("a/b/f");

    public static final Path CONFIG_PATH_NO_FLOWS = AYamlService.dirToConfigPath("ci/no/flows");
    public static final Path CONFIG_PATH_NOT_CI = AYamlService.dirToConfigPath("not/ci");
    public static final Path CONFIG_PATH_INVALID_SIMPLE = AYamlService.dirToConfigPath("invalid/schema");
    public static final Path CONFIG_PATH_INVALID_YAML_PARSE = AYamlService.dirToConfigPath("invalid/yaml-parse");
    public static final Path CONFIG_PATH_INVALID_INTERNAL_EXCEPTION = AYamlService.dirToConfigPath(
            "invalid/internal-exception"
    );
    public static final Path CONFIG_PATH_MISSING_TASK_DEFINITION = AYamlService.dirToConfigPath(
            "ci/missing-task-definition"
    );
    public static final Path CONFIG_PATH_WITH_SANDBOX_TEMPLATE_DEFINITION = AYamlService.dirToConfigPath(
            "ci/with-sandbox-template-task"
    );
    public static final Path CONFIG_PATH_UNKNOWN_ABC = AYamlService.dirToConfigPath(
            "invalid/unknown-abc"
    );

    public static final Path CONFIG_PATH_MOVED_FROM = AYamlService.dirToConfigPath("moved/from");
    public static final Path CONFIG_PATH_MOVED_TO = AYamlService.dirToConfigPath("moved/to");

    public static final Path CONFIG_PATH_PR_NEW = AYamlService.dirToConfigPath("pr/new");

    public static final Path CONFIG_PATH_SIMPLE_RELEASE = AYamlService.dirToConfigPath("release/simple");
    public static final Path CONFIG_PATH_ACTION_CUSTOM_RUNTIME =
            AYamlService.dirToConfigPath("release/action-custom-runtime");
    public static final Path CONFIG_PATH_SIMPLE_FILTER_RELEASE = AYamlService.dirToConfigPath("release/simple-filter");
    public static final Path CONFIG_PATH_WITH_BRANCHES_RELEASE = AYamlService.dirToConfigPath("release/with-branches");
    public static final Path CONFIG_PATH_SAWMILL_RELEASE = AYamlService.dirToConfigPath("release/sawmill");
    public static final Path PATH_IN_SIMPLE_RELEASE = Path.of("release/simple/readme.txt");
    public static final Path PATH_IN_SIMPLE_FILTER_RELEASE = Path.of("release/simple-filter/readme.txt");

    public static final Path CONFIG_PATH_IN_ROOT = AYamlService.dirToConfigPath("");
    public static final CiProcessId ROOT_RELEASE_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_IN_ROOT, "some-release"
    );
    public static final CiProcessId ROOT_ANY_FAIL_RELEASE_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_IN_ROOT, "any-fail-release"
    );

    public static final CiProcessId SIMPLE_RELEASE_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_SIMPLE_RELEASE, "simple"
    );
    public static final CiProcessId SIMPLE_FILTER_RELEASE_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_SIMPLE_FILTER_RELEASE, "simple"
    );
    public static final CiProcessId WITH_BRANCHES_RELEASE_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_WITH_BRANCHES_RELEASE, "with-branches"
    );
    public static final CiProcessId WITH_AUTO_BRANCHES_RELEASE_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_WITH_BRANCHES_RELEASE, "with-auto-branches"
    );
    public static final CiProcessId WITH_BRANCHES_FORBID_TRUNK_RELEASE_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_WITH_BRANCHES_RELEASE, "with-branches-forbidden-trunk"
    );
    public static final CiProcessId SAWMILL_RELEASE_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "demo-sawmill-release"
    );
    public static final CiProcessId SAWMILL_RELEASE_NO_DISPLACEMENT_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "demo-sawmill-no-displacement-release"
    );
    public static final CiProcessId SAWMILL_RELEASE_DISPLACEMENT_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "demo-sawmill-displacement-release"
    );
    public static final CiProcessId SAWMILL_RELEASE_CONDITIONAL_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "demo-sawmill-release-conditional"
    );
    public static final CiProcessId SAWMILL_RELEASE_CONDITIONAL_VARS_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "demo-sawmill-release-conditional-vars"
    );
    public static final CiProcessId EMPTY_RELEASE_PROCESS_ID = CiProcessId.ofRelease(
            // пустой флоу, который запускается и завершается
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "empty-release"
    );
    public static final CiProcessId EMPTY_MANUAL_RELEASE_PROCESS_ID = CiProcessId.ofRelease(
            // пустой флоу, который запускается и висит на гендальфе
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "empty-manual-release"
    );
    public static final CiProcessId SIMPLEST_RELEASE_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-release"
    );
    public static final CiProcessId SIMPLEST_RELEASE_WITH_MANUAL_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-release-with-manual"
    );
    public static final CiProcessId DUMMY_RELEASE_PROCESS_ID = CiProcessId.ofRelease(
            // нет задискавернных коммитов
            TestData.DUMMY_RELEASE_AYAML_PATH, "dummy"
    );
    public static final CiProcessId SIMPLEST_FLOW_ID = CiProcessId.ofFlow(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-flow"
    );
    public static final CiProcessId SIMPLEST_WITHOUT_RESTART = CiProcessId.ofFlow(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-flow-without-restart"
    );
    public static final CiProcessId SIMPLEST_ACTION_WITH_CLEANUP_ID = CiProcessId.ofFlow(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-action-with-cleanup"
    );
    public static final CiProcessId SIMPLEST_ACTION_WITH_CLEANUP_WITHOUT_JOBS_ID = CiProcessId.ofFlow(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-action-with-cleanup-without-jobs"
    );
    public static final CiProcessId SIMPLEST_ACTION_WITH_CLEANUP_AUTO_ID = CiProcessId.ofFlow(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-action-with-cleanup-auto"
    );
    public static final CiProcessId SIMPLEST_ACTION_WITH_CLEANUP_FAIL_SUCCESS_ONLY_ID = CiProcessId.ofFlow(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-action-with-cleanup-fail-success-only"
    );
    public static final CiProcessId SIMPLEST_ACTION_WITH_CLEANUP_FAIL_ID = CiProcessId.ofFlow(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-action-with-cleanup-fail"
    );
    public static final CiProcessId SIMPLEST_ACTION_WITH_CLEANUP_FAIL_AUTO_ID = CiProcessId.ofFlow(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-action-with-cleanup-fail-auto"
    );
    public static final CiProcessId SIMPLEST_ACTION_WITH_CLEANUP_DEPENDENT_ID = CiProcessId.ofFlow(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-action-with-cleanup-dependent"
    );
    public static final CiProcessId SIMPLEST_RELEASE_OVERRIDE_MULTIPLE_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-release-override-multiple-resources"
    );
    public static final CiProcessId SIMPLEST_RELEASE_OVERRIDE_SINGLE_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-release-override-single-resource"
    );
    public static final CiProcessId SIMPLEST_MULTIPLY_RELEASE_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-multiply-release"
    );
    public static final CiProcessId SIMPLEST_MULTIPLY_VIRTUAL_RELEASE_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-multiply-virtual-release"
    );
    public static final CiProcessId SIMPLEST_MULTIPLY_VIRTUAL_RELEASE_VARS_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-multiply-virtual-release-vars"
    );
    public static final CiProcessId SIMPLEST_SANDBOX_RELEASE_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-sandbox-release"
    );
    public static final CiProcessId SIMPLEST_SANDBOX_TEMPLATE_RELEASE_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-sandbox-template-release"
    );
    public static final CiProcessId SIMPLEST_SANDBOX_CONTEXT_RELEASE_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-sandbox-context-release"
    );
    public static final CiProcessId SIMPLEST_SANDBOX_MULTIPLY_RELEASE_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-sandbox-multiply-release"
    );
    public static final CiProcessId SIMPLEST_SANDBOX_BINARY_RELEASE_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-sandbox-binary-release"
    );
    public static final CiProcessId SIMPLEST_TASKLETV2_RELEASE_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-tasklet-v2-release"
    );
    public static final CiProcessId SIMPLEST_TASKLETV2_SIMPLE_RELEASE_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-tasklet-v2-simple-release"
    );
    public static final CiProcessId SIMPLEST_TASKLETV2_SIMPLE_INVALID_RELEASE_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "simplest-tasklet-v2-simple-invalid-release"
    );
    public static final CiProcessId SIMPLE_FLOW_PROCESS_ID = CiProcessId.ofFlow(
            TestData.CONFIG_PATH_SIMPLE_RELEASE, "sawmill"
    );
    public static final CiProcessId ON_COMMIT_WOODCUTTER_PROCESS_ID = CiProcessId.ofFlow(
            TestData.TRIGGER_ON_COMMIT_AYAML_PATH, "on-commit-woodcutter"
    );
    public static final CiProcessId ON_COMMIT_INTO_RELEASE_PROCESS_ID = CiProcessId.ofFlow(
            TestData.TRIGGER_ON_COMMIT_AYAML_PATH, "on-commit-into-release"
    );
    public static final CiProcessId ON_COMMIT_INTO_TRUNK_PROCESS_ID = CiProcessId.ofFlow(
            TestData.TRIGGER_ON_COMMIT_AYAML_PATH, "on-commit-into-trunk"
    );
    public static final CiProcessId ON_PR_INTERNAL_PROCESS_ID = CiProcessId.ofFlow(
            TestData.INTERNAL_CHANGE_AYAML_PATH, "internal-task-flow"
    );
    public static final CiProcessId DISCOVERY_DIR_ON_COMMIT_WOODCUTTER_PROCESS_ID = CiProcessId.ofFlow(
            TestData.DISCOVERY_DIR_ON_COMMIT_AYAML_PATH, "on-commit-woodcutter"
    );
    public static final CiProcessId DISCOVERY_DIR_ON_COMMIT2_WOODCUTTER_PROCESS_ID = CiProcessId.ofFlow(
            TestData.DISCOVERY_DIR_ON_COMMIT2_AYAML_PATH, "on-commit-woodcutter"
    );
    public static final CiProcessId DISCOVERY_DIR_NO_FILTER_ON_COMMIT_WOODCUTTER_PROCESS_ID = CiProcessId.ofFlow(
            TestData.DISCOVERY_DIR_ON_COMMIT_AYAML_PATH, "flow-without-filters"
    );
    public static final CiProcessId DISCOVERY_DIR_NO_FILTER_ON_COMMIT2_WOODCUTTER_PROCESS_ID = CiProcessId.ofFlow(
            TestData.DISCOVERY_DIR_ON_COMMIT2_AYAML_PATH, "flow-without-filters"
    );
    public static final CiProcessId SAWMILL_TICKETS_RELEASE_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "tickets-release"
    );

    public static final CiProcessId SAWMILL_CONDITIONAL_FAIL_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "conditional-fail"
    );

    public static final CiProcessId SAWMILL_CONDITIONAL_FAIL_WITH_RESOURCES_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_SAWMILL_RELEASE, "conditional-fail-with-resources"
    );

    public static final Path CONFIG_PATH_AUTO_RELEASE_SIMPLE = AYamlService.dirToConfigPath("release-auto/simple");
    public static final CiProcessId AUTO_RELEASE_SIMPLE_PROCESS_ID = CiProcessId.ofRelease(
            TestData.CONFIG_PATH_AUTO_RELEASE_SIMPLE, "simple"
    );

    public static final Path CONFIG_PATH_CHANGE_DS1 = AYamlService.dirToConfigPath("pr/change");

    public static final TaskId TASK_ID = TaskId.of("test/a");

    public static final FlowFullId PR_NEW_CONFIG_SAWMILL_FLOW_ID = FlowFullId.of(CONFIG_PATH_PR_NEW, "sawmill");

    public static final CiProcessId RELEASE_PROCESS_ID = CiProcessId.ofRelease(TestData.CONFIG_PATH_ABC, "my-release");

    public static final ArcBranch RELEASE_BRANCH_1 = ArcBranch.ofBranchName("releases/ci/release-component-1");
    public static final ArcBranch RELEASE_BRANCH_2 = ArcBranch.ofBranchName("releases/ci/release-component-2");

    public static final ArcBranch RELEASE_UNKNOWN_BRANCH_1 =
            ArcBranch.ofBranchName("releases/ci-unknown/release-component-1");
    public static final ArcBranch RELEASE_INVALID_BRANCH_1 =
            ArcBranch.ofBranchName("releases/ci-invalid/release-component-1");
    public static final ArcBranch RELEASE_UNDELEGATED_BRANCH_1 =
            ArcBranch.ofBranchName("releases/ci-undelegated/release-component-1");
    public static final ArcBranch RELEASE_UNDELEGATED_AUTO_LARGE_BRANCH_1 =
            ArcBranch.ofBranchName("releases/ci-undelegated-auto-large/release-component-1");

    public static final OrderedArcRevision TRUNK_R2R1 = OrderedArcRevision.fromHash("r2r1", ArcBranch.trunk(), 14, 104);
    public static final OrderedArcRevision TRUNK_R2R2 = OrderedArcRevision.fromHash("r2r2", ArcBranch.trunk(), 15, 105);
    public static final OrderedArcRevision TRUNK_R14 = OrderedArcRevision.fromHash("r14", ArcBranch.trunk(), 14, 104);
    public static final OrderedArcRevision TRUNK_R13 = OrderedArcRevision.fromHash("r13", ArcBranch.trunk(), 13, 103);
    public static final OrderedArcRevision TRUNK_R12 = OrderedArcRevision.fromHash("r12", ArcBranch.trunk(), 12, 102);
    public static final OrderedArcRevision TRUNK_R11 = OrderedArcRevision.fromHash("r11", ArcBranch.trunk(), 11, 101);
    public static final OrderedArcRevision TRUNK_R10 = OrderedArcRevision.fromHash("r10", ArcBranch.trunk(), 10, 100);
    public static final OrderedArcRevision TRUNK_R9 = OrderedArcRevision.fromHash("r9", ArcBranch.trunk(), 9, 99);
    public static final OrderedArcRevision TRUNK_R8 = OrderedArcRevision.fromHash("r8", ArcBranch.trunk(), 8, 98);
    public static final OrderedArcRevision TRUNK_R7 = OrderedArcRevision.fromHash("r7", ArcBranch.trunk(), 7, 97);
    public static final OrderedArcRevision RELEASE_R6_4 = OrderedArcRevision.fromHash("r6-4", RELEASE_BRANCH_2, 4, 0);
    public static final OrderedArcRevision RELEASE_R6_3 = OrderedArcRevision.fromHash("r6-3", RELEASE_BRANCH_2, 3, 0);
    public static final OrderedArcRevision RELEASE_R6_2 = OrderedArcRevision.fromHash("r6-2", RELEASE_BRANCH_2, 2, 0);
    public static final OrderedArcRevision RELEASE_R6_1 = OrderedArcRevision.fromHash("r6-1", RELEASE_BRANCH_2, 1, 0);
    public static final OrderedArcRevision TRUNK_R6 = OrderedArcRevision.fromHash("r6", ArcBranch.trunk(), 6, 96);
    public static final OrderedArcRevision TRUNK_R5 = OrderedArcRevision.fromHash("r5", ArcBranch.trunk(), 5, 95);
    public static final OrderedArcRevision TRUNK_R4 = OrderedArcRevision.fromHash("r4", ArcBranch.trunk(), 4, 94);
    public static final OrderedArcRevision TRUNK_R3 = OrderedArcRevision.fromHash("r3", ArcBranch.trunk(), 3, 93);
    public static final OrderedArcRevision RELEASE_R2_1 = OrderedArcRevision.fromHash("r2-1", RELEASE_BRANCH_1, 1, 0);
    public static final OrderedArcRevision RELEASE_R2_2 = OrderedArcRevision.fromHash("r2-2", RELEASE_BRANCH_1, 2, 0);
    public static final OrderedArcRevision RELEASE_UNKNOWN_R1_1 =
            OrderedArcRevision.fromHash("r2-1", RELEASE_UNKNOWN_BRANCH_1, 1, 0);
    public static final OrderedArcRevision RELEASE_INVALID_R1_1 =
            OrderedArcRevision.fromHash("r2-1", RELEASE_INVALID_BRANCH_1, 1, 0);
    public static final OrderedArcRevision RELEASE_UNDELEGATED_R1_1 =
            OrderedArcRevision.fromHash("r2-1", RELEASE_UNDELEGATED_BRANCH_1, 1, 0);
    public static final OrderedArcRevision RELEASE_UNDELEGATED_AUTO_LARGE_R1_1 =
            OrderedArcRevision.fromHash("r2-1", RELEASE_UNDELEGATED_AUTO_LARGE_BRANCH_1, 1, 0);
    public static final OrderedArcRevision TRUNK_R2 = OrderedArcRevision.fromHash("r2", ArcBranch.trunk(), 2, 92);
    public static final OrderedArcRevision TRUNK_R1 = OrderedArcRevision.fromHash("r1", ArcBranch.trunk(), 1, 91);
    private static final List<OrderedArcRevision> TRUNK_REVISIONS = List.of(
            TRUNK_R1,
            TRUNK_R2,
            TRUNK_R3,
            TRUNK_R4,
            TRUNK_R5,
            TRUNK_R6,
            TRUNK_R7,
            TRUNK_R8,
            TRUNK_R9,
            TRUNK_R10
    );
    public static final OrderedArcRevision PR_1 =
            OrderedArcRevision.fromHash("pr1", ArcBranch.ofPullRequest(123), 0, 123);


    public static final ArcRevision REVISION = ArcRevision.of("08caab95bce4e9d40b620a9c1840c4951beb956c");
    public static final ArcRevision SECOND_REVISION = ArcRevision.of("a6bb27178d80cf1f66e62ba6b01ec26bcb8b1f42");
    public static final ArcRevision THIRD_REVISION = ArcRevision.of("9267de1926f485b7ecb6aab90b9dc37d8a82557a");

    public static final ArcCommit REVISION_COMMIT = toCommit(REVISION, "andreevdm", 0, 0);
    public static final ArcCommit SECOND_REVISION_COMMIT = toCommit(SECOND_REVISION, "andreevdm", 0, 0);
    public static final ArcCommit THIRD_REVISION_COMMIT = toCommit(THIRD_REVISION, "andreevdm", 0, 0);

    public static final ArcBranch USER_BRANCH = ArcBranch.ofBranchName("users/andreevdm/CI-606");

    public static final ArcCommit RELEASE_BRANCH_COMMIT_6_4 = toBranchCommit(RELEASE_R6_4, "anmakon");
    public static final ArcCommit RELEASE_BRANCH_COMMIT_6_3 = toBranchCommit(RELEASE_R6_3, "andreevdm");
    public static final ArcCommit RELEASE_BRANCH_COMMIT_6_2 = toBranchCommit(RELEASE_R6_2, "firov");
    public static final ArcCommit RELEASE_BRANCH_COMMIT_6_1 = toBranchCommit(RELEASE_R6_1, "teplosvet");
    // naming: RELEASE_BRANCH_X_Y, where X - base trunk revision, Y - branch revision
    public static final ArcCommit RELEASE_BRANCH_COMMIT_2_1 = toBranchCommit(RELEASE_R2_1, "algebraic");
    public static final ArcCommit RELEASE_BRANCH_COMMIT_2_2 = toBranchCommit(RELEASE_R2_2, "algebraic");
    public static final ArcCommit TRUNK_COMMIT_1 = toCommit(TRUNK_R1, "alkedr"); // первый коммит, без a.yaml
    public static final ArcCommit TRUNK_COMMIT_2 = toCommit(TRUNK_R2, "sid-hugo").withParent(TRUNK_COMMIT_1);
    public static final ArcCommit TRUNK_COMMIT_3 = toCommit(TRUNK_R3, "algebraic").withParent(TRUNK_COMMIT_2);
    public static final ArcCommit TRUNK_COMMIT_4 = toCommit(TRUNK_R4, "firov").withParent(TRUNK_COMMIT_3);
    public static final ArcCommit TRUNK_COMMIT_5 = toCommit(TRUNK_R5, "pochemuto").withParent(TRUNK_COMMIT_4);
    public static final ArcCommit TRUNK_COMMIT_6 = toCommit(TRUNK_R6, "andreevdm").withParent(TRUNK_COMMIT_5);
    public static final ArcCommit TRUNK_COMMIT_7 = toCommit(TRUNK_R7, "teplosvet").withParent(TRUNK_COMMIT_6);
    public static final ArcCommit TRUNK_COMMIT_8 = toCommit(TRUNK_R8, "teplosvet").withParent(TRUNK_COMMIT_7);
    public static final ArcCommit TRUNK_COMMIT_9 = toCommit(TRUNK_R9, "teplosvet").withParent(TRUNK_COMMIT_8);
    public static final ArcCommit TRUNK_COMMIT_10 = toCommit(TRUNK_R10, "teplosvet").withParent(TRUNK_COMMIT_9);
    public static final ArcCommit TRUNK_COMMIT_11 = toCommit(TRUNK_R11, "albazh").withParent(TRUNK_COMMIT_10);
    public static final ArcCommit TRUNK_COMMIT_12 = toCommit(TRUNK_R12, "miroslav2").withParent(TRUNK_COMMIT_11);
    public static final ArcCommit TRUNK_COMMIT_13 = toCommit(TRUNK_R13, "miroslav2").withParent(TRUNK_COMMIT_12);
    public static final ArcCommit TRUNK_COMMIT_14 = toCommit(TRUNK_R14, "miroslav2").withParent(TRUNK_COMMIT_13);
    public static final ArcCommit TRUNK_COMMIT_R2R1 = toCommit(TRUNK_R2R1, "miroslav2").withParent(TRUNK_COMMIT_2);
    public static final ArcCommit TRUNK_COMMIT_R2R2 = toCommit(TRUNK_R2R2, "miroslav2").withParent(TRUNK_COMMIT_R2R1);

    public static final ArcRevision DS1_REVISION = ArcRevision.of("ds1");
    public static final ArcRevision DS2_REVISION = ArcRevision.of("ds2");
    public static final ArcRevision DS3_REVISION = ArcRevision.of("ds3");
    public static final ArcRevision DS4_REVISION = ArcRevision.of("ds4");
    public static final ArcRevision DS5_REVISION = ArcRevision.of("ds5");
    public static final ArcRevision DS6_REVISION = ArcRevision.of("ds6");
    public static final ArcRevision DS7_REVISION = ArcRevision.of("ds7");

    public static final ArcCommit DS1_COMMIT = toCommit(DS1_REVISION, "andreevdm", 0, 0);
    public static final ArcCommit DS2_COMMIT = toCommit(DS2_REVISION, "andreevdm", 0, 0);
    public static final ArcCommit DS3_COMMIT = toCommit(DS3_REVISION, "andreevdm", 0, 0);
    public static final ArcCommit DS4_COMMIT = toCommit(DS4_REVISION, "andreevdm", 0, 0);
    public static final ArcCommit DS5_COMMIT = toCommit(DS5_REVISION, "andreevdm", 0, 0);
    public static final ArcCommit DS6_COMMIT = toCommit(DS6_REVISION, "andreevdm", 0, 0);
    public static final ArcCommit DS7_COMMIT = toCommit(DS7_REVISION, "andreevdm", 0, 0).withParent(TRUNK_COMMIT_2);

    public static final String USER_TICKET = "user-ticket-42";
    public static final String USER_TICKET_2 = "user-ticket-742";

    public static final YavToken.Id YAV_TOKEN_UUID = YavToken.Id.of("uuid-42");
    public static final YavToken.Id YAV_TOKEN_UUID_2 = YavToken.Id.of("uuid-742");

    public static final ConfigSecurityState VALID_SECURITY_STATE = new ConfigSecurityState(
            YavToken.Id.of("abcde"), ConfigSecurityState.ValidationStatus.VALID_USER
    );

    public static final ConfigSecurityState NO_TOKEN_SECURITY_STATE = new ConfigSecurityState(
            null, ConfigSecurityState.ValidationStatus.TOKEN_NOT_FOUND
    );

    public static final String SECRET = "sec-01dy7t26dyht1bj4w3yn94fsa";


    public static final PullRequestDiffSet DIFF_SET_1 = diffSet(1,
            TestData.DS1_REVISION, TestData.TRUNK_R2, TestData.REVISION, List.of(), List.of());
    public static final PullRequestDiffSet DIFF_SET_2 = diffSet(2,
            TestData.DS2_REVISION, TestData.TRUNK_R3, TestData.SECOND_REVISION, List.of("CI-1"), List.of());
    public static final PullRequestDiffSet DIFF_SET_3 = diffSet(3,
            TestData.DS3_REVISION, TestData.TRUNK_R4, TestData.THIRD_REVISION, List.of("CI-1", "CI-2"), List.of());
    public static final PullRequestDiffSet DIFF_SET_4 = diffSet(4,
            TestData.DS4_REVISION, TestData.TRUNK_R2, TestData.SECOND_REVISION, List.of("CI-1"), List.of());
    public static final PullRequestDiffSet DIFF_SET_5 = diffSet(5,
            TestData.DS5_REVISION, TestData.TRUNK_R2, TestData.SECOND_REVISION, List.of(), List.of());

    public static final PullRequestDiffSet DIFF_SET_R6 = diffSet(6,
            TestData.DS6_REVISION, TestData.RELEASE_R2_1, TestData.REVISION, List.of(), List.of());
    public static final PullRequestDiffSet DIFF_SET_UNKNOWN_R6 = diffSet(6,
            TestData.DS6_REVISION, TestData.RELEASE_UNKNOWN_R1_1, TestData.REVISION, List.of(), List.of());
    public static final PullRequestDiffSet DIFF_SET_INVALID_R6 = diffSet(6,
            TestData.DS6_REVISION, TestData.RELEASE_INVALID_R1_1, TestData.REVISION, List.of(), List.of());
    public static final PullRequestDiffSet DIFF_SET_UNDELEGATED_R6 = diffSet(6,
            TestData.DS6_REVISION, TestData.RELEASE_UNDELEGATED_R1_1, TestData.REVISION, List.of(), List.of());
    public static final PullRequestDiffSet DIFF_SET_UNDELEGATED_AUTO_LARGE_R6 = diffSet(6,
            TestData.DS6_REVISION, TestData.RELEASE_UNDELEGATED_AUTO_LARGE_R1_1, TestData.REVISION, List.of(),
            List.of());

    public static final OrderedArcRevision REVISION_AT_USER_BRANCH = OrderedArcRevision
            .fromRevision(REVISION, USER_BRANCH, DIFF_SET_1.getDiffSetId(), DIFF_SET_1.getPullRequestId());
    public static final OrderedArcRevision SECOND_REVISION_AT_USER_BRANCH = OrderedArcRevision
            .fromRevision(SECOND_REVISION, USER_BRANCH, DIFF_SET_2.getDiffSetId(), DIFF_SET_2.getPullRequestId());
    public static final OrderedArcRevision THIRD_REVISION_AT_USER_BRANCH = OrderedArcRevision
            .fromRevision(THIRD_REVISION, USER_BRANCH, DIFF_SET_3.getDiffSetId(), DIFF_SET_3.getPullRequestId());

    private TestData() {
    }

    public static ArcCommit toCommit(OrderedArcRevision revision, String author) {
        return toCommit(revision.toRevision(), author, revision.getNumber(), revision.getPullRequestId());
    }

    public static ArcCommit toBranchCommit(OrderedArcRevision revision, String author) {
        return toCommit(revision.toRevision(), author, -1, 0);
    }

    public static ArcCommit toBranchCommit(OrderedArcRevision revision) {
        return toCommit(revision.toRevision(), TestData.CI_USER, -1, 0);
    }

    public static ArcCommit toCommit(ArcRevision revision, String author, long svnRevision, long pullRequestId) {
        return ArcCommit.builder()
                .id(revision.toCommitId())
                .pullRequestId(pullRequestId)
                .author(author)
                .message("Message")
                .createTime(COMMIT_DATE)
                .parents(List.of())
                .svnRevision(svnRevision)
                .build();
    }

    public static DiscoveredCommit createDiscoveredCommit(CiProcessId processId, OrderedArcRevision rev0) {
        DiscoveredCommitState discoveredCommitState = DiscoveredCommitState.builder()
                .dirDiscovery(true)
                .build();
        return DiscoveredCommit.of(processId, rev0, 1, discoveredCommitState);
    }

    public static OrderedArcRevision trunkRevision(int number) {
        return TRUNK_REVISIONS.get(number - 1);
    }

    public static PullRequestDiffSet diffSet(int diffSetId,
                                             ArcRevision mergeRevision,
                                             OrderedArcRevision upstreamRevision,
                                             ArcRevision featureRevision,
                                             List<String> issues,
                                             List<String> labels) {
        return new PullRequestDiffSet(
                new PullRequest(42, "andreevdm", "CI-42 New CI configs", "Description"),
                diffSetId,
                new PullRequestVcsInfo(
                        mergeRevision,
                        upstreamRevision.toRevision(),
                        upstreamRevision.getBranch(),
                        featureRevision,
                        TestData.USER_BRANCH
                ),
                null,
                issues,
                labels,
                null,
                null
        );
    }

    public static Launch.Builder launchBuilder() {
        return Launch.builder()
                .version(Version.major("1"))
                .vcsInfo(LaunchVcsInfo.builder()
                        .revision(TestData.TRUNK_R2)
                        .build())
                .flowInfo(LaunchFlowInfo.builder()
                        .flowId(FlowFullId.of(TestData.CONFIG_PATH_ABC, "flow"))
                        .configRevision(TRUNK_R1)
                        .runtimeInfo(LaunchRuntimeInfo.ofRuntimeSandboxOwner(TestData.YAV_TOKEN_UUID, TestData.CI_USER))
                        .build()
                );
    }

    /**
     * Stub дерево репозитория построено на захардкоженных OrderedArcRevision, с прикопанными именами веток
     * {@link TestData#RELEASE_BRANCH_1}. Однако при отведении веток в тестах их имя генерируется и как результат
     * ревизии будут принадлежать другой ветке.
     */
    public static OrderedArcRevision projectRevToBranch(OrderedArcRevision revision, ArcBranch arcBranch) {
        return OrderedArcRevision.fromRevision(revision.toRevision(),
                arcBranch,
                revision.getNumber(),
                revision.getPullRequestId()
        );
    }
}
