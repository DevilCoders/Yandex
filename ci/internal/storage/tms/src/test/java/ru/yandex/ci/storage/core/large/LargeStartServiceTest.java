package ru.yandex.ci.storage.core.large;

import java.time.Instant;
import java.util.List;
import java.util.function.Consumer;
import java.util.stream.Collectors;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;
import org.mockito.Mockito;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.api.storage.StorageApi;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass.CheckType;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check.LargeTestsConfig;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffEntity;

import static org.assertj.core.api.Assertions.assertThatThrownBy;

class LargeStartServiceTest extends AbstractLargeAutostartServiceTest {

    @Autowired
    LargeStartService startService;

    @Test
    void tryStartNonExistingTests() {
        db.currentOrTx(() -> db.checks().save(sampleCheck));
        var testDiffId = new TestDiffEntity.Id(
                sampleCheck.getId(),
                CheckIteration.IterationType.HEAVY.getNumber(),
                Common.ResultType.RT_TEST_SUITE_LARGE,
                "c1",
                "/a/b/c",
                12345,
                67890,
                1);

        assertThatThrownBy(() -> startService.startLargeTestsManual(List.of(testDiffId), "username"))
                .isInstanceOf(RuntimeException.class)
                .hasMessage("Unable to find requested tests: [[[1/HEAVY/1]]/RT_TEST_SUITE_LARGE/[[12345/c1/67890]]]");
    }

    @Test
    void tryStartWithoutLargeConfig() {
        var settings = LargeTestSettings.builder()
                .check(sampleCheckWithRev)
                .testStatuses(TestStatuses.largeTests(
                        Common.TestStatus.TS_DISCOVERED, Common.TestStatus.TS_DISCOVERED
                ))
                .saveIteration(() -> sampleIteration)
                .checkIteration(updatedIteration -> {
                })
                .expectTasks(List.of())
                .manual(true)
                .executor(diffs -> startService.startLargeTestsManual(
                        diffs.stream()
                                .map(TestDiffEntity::getId)
                                .collect(Collectors.toList()),
                        "username"))
                .build();

        assertThatThrownBy(() -> new LargeTest(settings).test())
                .hasMessage("Large tests configuration cannot be empty for BRANCH_PRE_COMMIT checks");
    }

    @ParameterizedTest
    @MethodSource("startLargeTests")
    void startLargeTestsBranch(
            List<String> toolchains,
            TestStatuses testStatuses,
            List<LargeStart> expectTasks
    ) {

        var largeTestConfig = new LargeTestsConfig("some_other_dir/a.yaml", new StorageRevision("trunk", "rev", 1,
                Instant.ofEpochMilli(123)));
        var delegatedConfig = StorageApi.DelegatedConfig.newBuilder()
                .setPath("some_other_dir/a.yaml")
                .setRevision(ru.yandex.ci.api.proto.Common.OrderedArcRevision.newBuilder()
                        .setBranch("trunk")
                        .setHash("rev")
                        .setNumber(1))
                .build();

        var settings = LargeTestSettings.builder()
                .check(sampleCheckWithRev.toBuilder()
                        .largeTestsConfig(largeTestConfig)
                        .build())
                .testStatuses(testStatuses)
                .saveIteration(() -> sampleIteration)
                .checkIteration(updatedIteration -> {
                })
                .expectTasks(expectTasks)
                .largeTestsDelegatedConfig(delegatedConfig)
                .nativeBuildsDelegatedConfig(delegatedConfig)
                .manual(true)
                .executor(diffs -> startService.startLargeTestsManual(
                        diffs.stream()
                                .map(TestDiffEntity::getId)
                                .filter(id -> toolchains.contains(id.getToolchain()))
                                .collect(Collectors.toList()),
                        "username"))
                .startedBy("username")
                .build();

        new LargeTest(settings).test();
    }

    @ParameterizedTest
    @MethodSource("startLargeTests")
    void startLargeTestsTrunkPreCommitNoDelegated(
            List<String> toolchains,
            TestStatuses testStatuses,
            List<LargeStart> expectTasks
    ) {
        startLargeTestsNoDelegated(toolchains, testStatuses, expectTasks, CheckType.TRUNK_PRE_COMMIT);
    }

    @ParameterizedTest
    @MethodSource("startLargeTests")
    void startLargeTestsTrunkPostCommitNoDelegated(
            List<String> toolchains,
            TestStatuses testStatuses,
            List<LargeStart> expectTasks
    ) {
        startLargeTestsNoDelegated(toolchains, testStatuses, expectTasks, CheckType.TRUNK_POST_COMMIT);
    }


    @ParameterizedTest
    @MethodSource("startLargeTests")
    void startLargeTestsTrunkDelegated(
            List<String> toolchains,
            TestStatuses testStatuses,
            List<LargeStart> expectTasks
    ) {
        var revision = ru.yandex.ci.api.proto.Common.OrderedArcRevision.newBuilder()
                .setBranch("trunk")
                .setHash("rev")
                .setNumber(2);

        var response = StorageApi.GetLastValidPrefixedConfigResponse.newBuilder();
        response.addResponsesBuilder()
                .setPrefixDir("a")
                .setPath("a/a.yaml")
                .setConfigRevision(revision.setNumber(1).build());
        response.addResponsesBuilder()
                .setPrefixDir("a/b/c")
                .setPath("a/b/a.yaml")
                .setConfigRevision(revision);

        var delegatedConfig = StorageApi.DelegatedConfig.newBuilder()
                .setPath("a/b/a.yaml")
                .setRevision(revision)
                .build();

        Mockito.when(ciClient.getLastValidConfigBatch(Mockito.any(StorageApi.GetLastValidPrefixedConfigRequest.class)))
                .thenReturn(response.build());

        Consumer<List<TestDiffEntity>> runTests = diffs ->
                startService.startLargeTestsManual(
                        diffs.stream()
                                .map(TestDiffEntity::getId)
                                .filter(id -> toolchains.contains(id.getToolchain()))
                                .collect(Collectors.toList()),
                        "username");

        var settings = LargeTestSettings.builder()
                .check(sampleCheckWithRev.toBuilder()
                        .type(CheckType.TRUNK_PRE_COMMIT)
                        .build())
                .testStatuses(testStatuses)
                .saveIteration(() -> sampleIteration)
                .checkIteration(updatedIteration -> {
                })
                .largeTestsDelegatedConfig(delegatedConfig)
                .nativeBuildsDelegatedConfig(delegatedConfig)
                .expectTasks(expectTasks)
                .manual(true)
                .executor(diffs -> {
                    runTests.accept(diffs);
                    runTests.accept(diffs); // Run again (and must be completely ignored)
                })
                .startedBy("username")
                .build();

        new LargeTest(settings).test();

        if (!expectTasks.isEmpty()) {
            Mockito.verify(ciClient)
                    .getLastValidConfigBatch(Mockito.any(StorageApi.GetLastValidPrefixedConfigRequest.class));
        }
    }

    private void startLargeTestsNoDelegated(
            List<String> toolchains,
            TestStatuses testStatuses,
            List<LargeStart> expectTasks,
            CheckType checkType
    ) {

        // No delegated configs
        Mockito.when(ciClient.getLastValidConfigBatch(Mockito.any(StorageApi.GetLastValidPrefixedConfigRequest.class)))
                .thenReturn(StorageApi.GetLastValidPrefixedConfigResponse.getDefaultInstance());

        var settings = LargeTestSettings.builder()
                .launchable(false) // Tests will be started in any case, regardless of 'launchable' attribute
                .check(sampleCheckWithRev.toBuilder()
                        .type(checkType)
                        .build())
                .testStatuses(testStatuses)
                .saveIteration(() -> sampleIteration)
                .checkIteration(updatedIteration -> {
                })
                .expectTasks(expectTasks)
                .manual(true)
                .executor(diffs -> startService.startLargeTestsManual(
                        diffs.stream()
                                .map(TestDiffEntity::getId)
                                .filter(id -> toolchains.contains(id.getToolchain()))
                                .collect(Collectors.toList()),
                        "username"))
                .startedBy("username")
                .build();

        new LargeTest(settings).test();

        if (!expectTasks.isEmpty()) {
            Mockito.verify(ciClient)
                    .getLastValidConfigBatch(Mockito.any(StorageApi.GetLastValidPrefixedConfigRequest.class));
        }
    }


    static List<Arguments> startLargeTests() {
        var discovered = TestStatuses.largeTests(Common.TestStatus.TS_DISCOVERED, Common.TestStatus.TS_DISCOVERED);
        return List.of(
                Arguments.of(
                        List.of(),
                        discovered,
                        List.of()
                ),

                Arguments.of(
                        List.of("chain-1", "chain-2"),
                        discovered,
                        List.of(LargeStart.largeTest("chain-1", false),
                                LargeStart.largeTest("chain-1", true),
                                LargeStart.largeTest("chain-2", false),
                                LargeStart.largeTest("chain-2", true))),

                Arguments.of(
                        List.of("chain-3"),
                        discovered,
                        List.of(LargeStart.largeTest("chain-3", false),
                                LargeStart.largeTest("chain-3", true))),

                // All matches
                Arguments.of(
                        List.of("chain-1", "chain-2", "chain-3"),
                        discovered,
                        List.of(LargeStart.largeTest("chain-1", false),
                                LargeStart.largeTest("chain-1", true),
                                LargeStart.largeTest("chain-2", false),
                                LargeStart.largeTest("chain-2", true),
                                LargeStart.largeTest("chain-3", false),
                                LargeStart.largeTest("chain-3", true))
                ),

                // Partial match
                Arguments.of(
                        List.of("chain-1", "chain-2"),
                        TestStatuses.largeTests(Common.TestStatus.TS_INTERNAL, Common.TestStatus.TS_DISCOVERED),
                        List.of(LargeStart.largeTest("chain-1", true),
                                LargeStart.largeTest("chain-2", true))),

                // Partial full match
                Arguments.of(
                        List.of("chain-1", "chain-2", "chain-3"),
                        TestStatuses.largeTests(Common.TestStatus.TS_DISCOVERED, Common.TestStatus.TS_INTERNAL),
                        List.of(LargeStart.largeTest("chain-1", false),
                                LargeStart.largeTest("chain-2", false),
                                LargeStart.largeTest("chain-3", false))),

                // None match
                Arguments.of(
                        List.of("chain-1", "chain-2", "chain-3"),
                        TestStatuses.largeTests(Common.TestStatus.TS_INTERNAL, Common.TestStatus.TS_INTERNAL),
                        List.of())
        );
    }
}
