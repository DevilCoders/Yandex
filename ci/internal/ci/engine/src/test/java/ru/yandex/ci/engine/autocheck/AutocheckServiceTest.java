package ru.yandex.ci.engine.autocheck;

import java.util.List;
import java.util.Set;

import org.assertj.core.util.Sets;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.ArgumentMatchers;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.engine.autocheck.config.AutocheckConfigurationConfig;
import ru.yandex.ci.engine.autocheck.config.AutocheckPartitionsConfig;
import ru.yandex.ci.engine.autocheck.config.AutocheckYamlConfig;
import ru.yandex.ci.engine.autocheck.config.AutocheckYamlService;
import ru.yandex.ci.engine.autocheck.model.AutocheckLaunchConfig;
import ru.yandex.ci.engine.pcm.PCMSelector;
import ru.yandex.ci.storage.core.CheckIteration.StrongModePolicy;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.doReturn;

class AutocheckServiceTest extends EngineTestBase {
    @Autowired
    protected AutocheckService autocheckService;

    @BeforeEach
    void setUp() {
        mockYav();
        mockValidationSuccessful();
    }

    public static AutocheckYamlService.ConfigBundle expectedConfigBundle() {
        var autocheckYamlConfig = new AutocheckYamlConfig(
                List.of(
                        new AutocheckConfigurationConfig(
                                "linux",
                                new AutocheckPartitionsConfig(42),
                                true
                        )
                )
        );
        return new AutocheckYamlService.ConfigBundle(
                TestData.TRUNK_R1.toRevision(),
                autocheckYamlConfig
        );
    }


    @Test
    void getAYamlAutocheckInfoFirstFastTargetsNotEmpty() {
        ArcRevision revision = ArcRevision.of("r7");
        ArcRevision prevRevision = ArcRevision.of("r6");

        AutocheckLaunchConfig expected = new AutocheckLaunchConfig(
                Set.of(CheckOuterClass.NativeBuild.newBuilder()
                                .setPath("autocheck.native.builds/first.not.empty/a.yaml")
                                .setToolchain("toolchain_1")
                                .addTarget("some/target/first")
                                .build(),
                        CheckOuterClass.NativeBuild.newBuilder()
                                .setPath("autocheck.native.builds/a.yaml")
                                .setToolchain("toolchain_1")
                                .addTarget("some/target/parent")
                                .build(),
                        CheckOuterClass.NativeBuild.newBuilder()
                                .setPath("autocheck.native.builds/a.yaml")
                                .setToolchain("toolchain_2")
                                .addTarget("some/target/parent")
                                .build()),
                Set.of(),
                CheckOuterClass.LargeConfig.getDefaultInstance(),
                Sets.newTreeSet("autocheck"),
                Sets.newTreeSet("path1", "path2"),
                false,
                Set.of(),
                PCMSelector.PRECOMMITS_DEFAULT_POOL,
                false,
                StrongModePolicy.BY_A_YAML,
                expectedConfigBundle(),
                expectedConfigBundle()
        );

        AutocheckLaunchConfig actual = autocheckService
                .findTrunkAutocheckLaunchParams(prevRevision, revision, null);

        assertThat(actual).isEqualTo(expected);
    }

    @Test
    void getAYamlAutocheckInfoFirstEmpty() {
        ArcRevision revision = ArcRevision.of("r8");
        ArcRevision prevRevision = ArcRevision.of("r7");

        AutocheckLaunchConfig expected = new AutocheckLaunchConfig(
                Set.of(CheckOuterClass.NativeBuild.newBuilder()
                        .setPath("autocheck.native.builds/a.yaml")
                        .setToolchain("toolchain_2")
                        .addTarget("some/target/parent")
                        .build()),
                Set.of(CheckOuterClass.LargeAutostart.newBuilder()
                                .setPath("autocheck.autostart.large.tests/simple/a.yaml")
                                .setTarget("some/large/test/target")
                                .addToolchains("default-linux-x86_64-release")
                                .addToolchains("default-linux-x86_64-release-fuzzing")
                                .build(),
                        CheckOuterClass.LargeAutostart.newBuilder()
                                .setPath("autocheck.native.builds/first.empty/a.yaml")
                                .setTarget("some_test")
                                .build()),
                CheckOuterClass.LargeConfig.getDefaultInstance(),
                Sets.newTreeSet("autocheck"),
                Sets.newTreeSet(),
                false,
                Set.of(),
                PCMSelector.PRECOMMITS_DEFAULT_POOL,
                false,
                StrongModePolicy.BY_A_YAML,
                expectedConfigBundle(),
                expectedConfigBundle()
        );

        AutocheckLaunchConfig actual = autocheckService
                .findTrunkAutocheckLaunchParams(prevRevision, revision, null);

        assertThat(actual).isEqualTo(expected);
    }

    @Test
    void getAYamlFastTargetsFirstNullWithSequential() {
        ArcRevision revision = ArcRevision.of("r9");
        ArcRevision prevRevision = ArcRevision.of("r8");

        AutocheckLaunchConfig expected = new AutocheckLaunchConfig(
                Set.of(),
                Set.of(),
                CheckOuterClass.LargeConfig.getDefaultInstance(),
                Sets.newTreeSet("autocheck"),
                Sets.newTreeSet("path1", "path2", "path42"),
                false, // sequential mode is ignored
                Set.of(),
                PCMSelector.PRECOMMITS_DEFAULT_POOL,
                false,
                StrongModePolicy.BY_A_YAML,
                expectedConfigBundle(),
                expectedConfigBundle()
        );

        AutocheckLaunchConfig actual = autocheckService
                .findTrunkAutocheckLaunchParams(prevRevision, revision, null);

        assertThat(actual).isEqualTo(expected);
    }

    @Test
    public void autoDetectFastTarget() {
        db.currentOrTx(() -> db.keyValue().setValue("AutocheckInfoCollector", "enabledForPercent", 100));
        ArcRevision prevRevision = TestData.TRUNK_COMMIT_R2R1.getRevision();
        ArcRevision revision = TestData.TRUNK_COMMIT_R2R2.getRevision();
        AutocheckLaunchConfig actual = autocheckService
                .findTrunkAutocheckLaunchParams(prevRevision, revision, null);

        assertThat(actual.getFastTargets()).containsExactly("mail");
    }

    @Test
    void getAYamlAutocheckInfoFirstInvalidConfig() {
        ArcRevision revision = ArcRevision.of("r10");
        ArcRevision prevRevision = ArcRevision.of("r9");

        AutocheckLaunchConfig expected = new AutocheckLaunchConfig(
                Set.of(CheckOuterClass.NativeBuild.newBuilder()
                                .setPath("autocheck.native.builds/a.yaml")
                                .setToolchain("toolchain_1")
                                .addTarget("some/target/parent")
                                .build(),
                        CheckOuterClass.NativeBuild.newBuilder()
                                .setPath("autocheck.native.builds/a.yaml")
                                .setToolchain("toolchain_2")
                                .addTarget("some/target/parent")
                                .build()),
                Set.of(CheckOuterClass.LargeAutostart.newBuilder()
                        .setPath("autocheck.autostart.large.tests/simple/a.yaml")
                        .setTarget("some/large/test/target")
                        .build()),
                CheckOuterClass.LargeConfig.getDefaultInstance(),
                Sets.newTreeSet("autocheck"),
                Sets.newTreeSet(),
                false,
                Set.of(
                        TestData.AUTOSTART_LARGE_TESTS_FIRST_INVALID_AYAML_PATH.toString(),
                        TestData.FAST_TARGETS_FIRST_INVALID_FIRST_AYAML_PATH.toString(),
                        TestData.NATIVE_BUILDS_FIRST_INVALID_FIRST_AYAML_PATH.toString()
                ),
                PCMSelector.PRECOMMITS_DEFAULT_POOL,
                false,
                StrongModePolicy.BY_A_YAML,
                expectedConfigBundle(),
                expectedConfigBundle()
        );

        AutocheckLaunchConfig actual = autocheckService
                .findTrunkAutocheckLaunchParams(prevRevision, revision, null);

        assertThat(actual).isEqualTo(expected);
    }

    @Test
    void getPoolName() {
        doReturn("//test/pool").when(pcmSelector)
                .selectPool(ArgumentMatchers.anySet(), anyString());

        ArcRevision prevRevision = ArcRevision.of("r8");
        ArcRevision revision = ArcRevision.of("r9");

        AutocheckLaunchConfig expected = new AutocheckLaunchConfig(
                Set.of(),
                Set.of(),
                CheckOuterClass.LargeConfig.getDefaultInstance(),
                Sets.newTreeSet("autocheck"),
                Sets.newTreeSet("path1", "path2", "path42"),
                false, // sequential mode is ignored
                Set.of(),
                "//test/pool",
                false,
                StrongModePolicy.BY_A_YAML,
                expectedConfigBundle(),
                expectedConfigBundle()
        );

        AutocheckLaunchConfig actual = autocheckService.findTrunkAutocheckLaunchParams(
                prevRevision, revision, "mockUser"
        );

        assertThat(actual).isEqualTo(expected);
    }

    @Test
    void testLargeTestsInBranches() {
        // configs from r1r1
        discovery(TestData.TRUNK_COMMIT_R2R1);
        discovery(TestData.TRUNK_COMMIT_R2R2);
        var branchConfig = autocheckService.findBranchAutocheckLaunchParams(
                TestData.RELEASE_BRANCH_1,
                TestData.TRUNK_COMMIT_R2R2
        );

        assertThat(branchConfig.getLargeTests())
                .isEqualTo(Set.of(
                        CheckOuterClass.LargeAutostart.newBuilder()
                                .setPath("config/branches/releases/ci.yaml")
                                .setTarget("ci/demo-project/large-tests")
                                .addToolchains("default-linux-x86_64-release")
                                .addToolchains("default-linux-x86_64-release-musl")
                                .build()
                ));
        assertThat(branchConfig.getLargeConfig())
                .isEqualTo(CheckOuterClass.LargeConfig.newBuilder()
                        .setPath("ci/a.yaml")
                        .setRevision(Common.OrderedRevision.newBuilder()
                                .setBranch("trunk")
                                .setRevision("r2r1")
                                .setRevisionNumber(14)
                                .build())
                        .build());
    }

    @Test
    public void testStrongModeInBranches_whenStrongModeIsMissing() {
        discovery(TestData.TRUNK_COMMIT_R2R1);
        var branchConfig = autocheckService.findBranchAutocheckLaunchParams(
                ArcBranch.ofBranchName("releases/ci/strong-mode-missing"),
                TestData.TRUNK_COMMIT_R2R1
        );
        assertThat(branchConfig.getStrongModePolicy()).isEqualTo(StrongModePolicy.FORCE_OFF);
    }

    @Test
    public void testStrongModeInBranches_whenStrongModeIsOn() {
        discovery(TestData.TRUNK_COMMIT_R2R1);
        var branchConfig = autocheckService.findBranchAutocheckLaunchParams(
                ArcBranch.ofBranchName("releases/ci/strong-mode-on"),
                TestData.TRUNK_COMMIT_R2R1
        );
        assertThat(branchConfig.getStrongModePolicy()).isEqualTo(StrongModePolicy.FORCE_ON);
    }

    @Test
    public void testStrongModeInBranches_whenStrongModeIsOff() {
        discovery(TestData.TRUNK_COMMIT_R2R1);
        var branchConfig = autocheckService.findBranchAutocheckLaunchParams(
                ArcBranch.ofBranchName("releases/ci/strong-mode-off"),
                TestData.TRUNK_COMMIT_R2R1
        );
        assertThat(branchConfig.getStrongModePolicy()).isEqualTo(StrongModePolicy.FORCE_OFF);
    }

}
