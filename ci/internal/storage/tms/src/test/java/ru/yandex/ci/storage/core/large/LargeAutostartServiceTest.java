package ru.yandex.ci.storage.core.large;

import java.time.Instant;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;
import org.mockito.Mockito;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.api.storage.StorageApi;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check.LargeAutostart;
import ru.yandex.ci.storage.core.db.model.check.LargeTestsConfig;
import ru.yandex.ci.storage.core.db.model.check.NativeBuild;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckTaskStatus;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;
import static ru.yandex.ci.util.CollectionUtils.linkedMap;

class LargeAutostartServiceTest extends AbstractLargeAutostartServiceTest {

    @Autowired
    LargeAutostartService autostartService;

    @Test
    void tryAutostartNoCheck() {
        assertThatThrownBy(() -> autostartService.tryAutostart(sampleIterationId))
                .hasMessage("Unable to find key [1] in table [Checks]");
    }

    @Test
    void tryAutostartSettingsForCheck() {
        db.currentOrTx(() -> {
            db.checks().save(sampleCheck);
            db.checkIterations().save(sampleIteration);
        });
        autostartService.tryAutostart(sampleIterationId);
    }

    @ParameterizedTest
    @MethodSource("allCheckTaskStatuses")
    void tryAutostartSettingsUnmatched(
            Map<Common.CheckTaskType, CheckTaskStatus> setStatuses,
            Map<Common.CheckTaskType, CheckTaskStatus> expectStatuses
    ) {
        db.currentOrTx(() -> {
            db.checks().save(sampleCheck);
            db.checkIterations().save(sampleIteration.updateCheckTaskStatuses(setStatuses));
        });
        autostartService.tryAutostart(sampleIterationId);

        var iteration = db.currentOrReadOnly(() ->
                db.checkIterations().get(sampleIteration.getId()));
        assertThat(iteration.getCheckTaskStatuses())
                .isEqualTo(expectStatuses);
    }


    @Test
    void tryAutostartLargeTestsWithoutLargeConfig() {
        var iteration = sampleIteration.updateCheckTaskStatuses(
                Map.of(Common.CheckTaskType.CTT_LARGE_TEST, CheckTaskStatus.SCHEDULED)
        );

        var settings = LargeTestSettings.builder()
                .check(sampleCheckWithRev.toBuilder()
                        .autostartLargeTests(List.of(
                                new LargeAutostart("some_dir/a.yaml", "a/b/c", List.of("chain-1", "chain-2")))
                        )
                        .build())
                .testStatuses(TestStatuses.largeTests(
                        Common.TestStatus.TS_DISCOVERED, Common.TestStatus.TS_DISCOVERED
                ))
                .saveIteration(() -> iteration)
                .checkIteration(updatedIteration -> {
                })
                .expectTasks(List.of())
                .executor(diffs -> autostartService.tryAutostart(iteration.getId()))
                .reportEmptyTests(true)
                .build();

        assertThatThrownBy(() -> new LargeTest(settings).test())
                .hasMessage("Large tests configuration cannot be empty for BRANCH_PRE_COMMIT checks");
    }

    @Test
    void tryAutostartNativeBuildsWithoutLargeConfig() {
        var iteration = sampleIteration.updateCheckTaskStatuses(
                Map.of(Common.CheckTaskType.CTT_NATIVE_BUILD, CheckTaskStatus.SCHEDULED)
        );

        var settings = LargeTestSettings.builder()
                .check(sampleCheckWithRev.toBuilder()
                        .nativeBuilds(List.of(
                                new NativeBuild("some_dir/a.yaml", "chain-1", List.of("a/b", "b/c"))
                        ))
                        .build())
                .testStatuses(TestStatuses.largeTests(
                        Common.TestStatus.TS_DISCOVERED, Common.TestStatus.TS_DISCOVERED
                ))
                .saveIteration(() -> iteration)
                .checkIteration(updatedIteration -> {
                })
                .expectTasks(List.of())
                .executor(diffs -> autostartService.tryAutostart(iteration.getId()))
                .reportEmptyTests(true)
                .build();

        assertThatThrownBy(() -> new LargeTest(settings).test())
                .hasMessage("Large tests configuration cannot be empty for BRANCH_PRE_COMMIT checks");
    }

    @ParameterizedTest
    @MethodSource("tryAutostartLargeTests")
    void tryAutostartLargeTestsBranch(
            Boolean runLargeTestsAfterDiscovery,
            List<LargeAutostart> largeTests,
            List<NativeBuild> nativeBuilds,
            TestStatuses testStatuses,
            Map<Common.CheckTaskType, CheckTaskStatus> setStatuses,
            Map<Common.CheckTaskType, CheckTaskStatus> expectStatuses,
            List<LargeStart> expectTasks
    ) {
        var iteration = sampleIteration.updateCheckTaskStatuses(setStatuses);

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
                .launchable(true) // Same as `null` - i.e. accept
                .check(sampleCheckWithRev.toBuilder()
                        .autostartLargeTests(largeTests)
                        .largeTestsConfig(largeTestConfig)
                        .nativeBuilds(nativeBuilds)
                        .runLargeTestsAfterDiscovery(runLargeTestsAfterDiscovery)
                        .build())
                .testStatuses(testStatuses)
                .saveIteration(() -> iteration)
                .checkIteration(updatedIteration ->
                        assertThat(updatedIteration.getCheckTaskStatuses()).isEqualTo(expectStatuses))
                .expectTasks(expectTasks)
                .largeTestsDelegatedConfig(delegatedConfig)
                .nativeBuildsDelegatedConfig(delegatedConfig)
                .executor(diffs -> autostartService.tryAutostart(iteration.getId()))
                .reportEmptyTests(true)
                .build();

        new LargeTest(settings).test();
    }

    @ParameterizedTest
    @MethodSource("tryAutostartLargeTests")
    void tryAutostartLargeTestsTrunkPreCommitNoDelegated(
            Boolean runLargeTestsAfterDiscovery,
            List<LargeAutostart> largeTests,
            List<NativeBuild> nativeBuilds,
            TestStatuses testStatuses,
            Map<Common.CheckTaskType, CheckTaskStatus> setStatuses,
            Map<Common.CheckTaskType, CheckTaskStatus> expectStatuses,
            List<LargeStart> expectTasks) {
        tryAutostartLargeTestsNoDelegated(
                runLargeTestsAfterDiscovery,
                largeTests,
                nativeBuilds,
                testStatuses,
                setStatuses,
                expectStatuses,
                expectTasks,
                CheckOuterClass.CheckType.TRUNK_PRE_COMMIT
        );
    }

    @ParameterizedTest
    @MethodSource("tryAutostartLargeTests")
    void tryAutostartLargeTestsTrunkPostCommitNoDelegated(
            Boolean runLargeTestsAfterDiscovery,
            List<LargeAutostart> largeTests,
            List<NativeBuild> nativeBuilds,
            TestStatuses testStatuses,
            Map<Common.CheckTaskType, CheckTaskStatus> setStatuses,
            Map<Common.CheckTaskType, CheckTaskStatus> expectStatuses,
            List<LargeStart> expectTasks
    ) {
        tryAutostartLargeTestsNoDelegated(
                runLargeTestsAfterDiscovery,
                largeTests,
                nativeBuilds,
                testStatuses,
                setStatuses,
                expectStatuses,
                expectTasks,
                CheckOuterClass.CheckType.TRUNK_POST_COMMIT
        );
    }

    @ParameterizedTest
    @MethodSource("tryAutostartLargeTests")
    void tryAutostartLargeTestsTrunkDelegated(
            Boolean runLargeTestsAfterDiscovery,
            List<LargeAutostart> largeTests,
            List<NativeBuild> nativeBuilds,
            TestStatuses testStatuses,
            Map<Common.CheckTaskType, CheckTaskStatus> setStatuses,
            Map<Common.CheckTaskType, CheckTaskStatus> expectStatuses,
            List<LargeStart> expectTasks
    ) {
        var revision = ru.yandex.ci.api.proto.Common.OrderedArcRevision.newBuilder()
                .setBranch("trunk")
                .setHash("rev")
                .setNumber(3);

        var response = StorageApi.GetLastValidPrefixedConfigResponse.newBuilder();
        response.addResponsesBuilder()
                .setPrefixDir("a")
                .setPath("a/a.yaml")
                .setConfigRevision(revision.setNumber(1).build());
        response.addResponsesBuilder()
                .setPrefixDir("a/b/c")
                .setPath("a/b/a.yaml")
                .setConfigRevision(revision.setNumber(2).build());


        // Well, actually we'll pick configuration from large autostart settings
        for (var large : largeTests) {
            response.addResponsesBuilder()
                    .setPrefixDir(Objects.requireNonNull(large.getPath()))
                    .setPath("large/a.yaml")
                    .setConfigRevision(revision);
        }
        for (var nativeBuild : nativeBuilds) {
            response.addResponsesBuilder()
                    .setPrefixDir(nativeBuild.getPath())
                    .setPath("large/a.yaml")
                    .setConfigRevision(revision);
        }


        var largeTestsPath = Objects.equals(Boolean.TRUE, runLargeTestsAfterDiscovery)
                ? "a/b/a.yaml" // Default settings if running all tests
                : "large/a.yaml";

        var largeTestsDelegatedConfig = StorageApi.DelegatedConfig.newBuilder()
                .setPath(largeTestsPath)
                .setRevision(revision)
                .build();

        var nativeBuildsDelegatedConfig = StorageApi.DelegatedConfig.newBuilder()
                .setPath("large/a.yaml")
                .setRevision(revision)
                .build();

        Mockito.when(ciClient.getLastValidConfigBatch(Mockito.any(StorageApi.GetLastValidPrefixedConfigRequest.class)))
                .thenReturn(response.build());

        var iteration = sampleIteration.updateCheckTaskStatuses(setStatuses);

        var settings = LargeTestSettings.builder()
                .check(sampleCheckWithRev.toBuilder()
                        .type(CheckOuterClass.CheckType.TRUNK_PRE_COMMIT)
                        .autostartLargeTests(largeTests)
                        .nativeBuilds(nativeBuilds)
                        .runLargeTestsAfterDiscovery(runLargeTestsAfterDiscovery)
                        .build())
                .testStatuses(testStatuses)
                .saveIteration(() -> iteration)
                .checkIteration(updatedIteration ->
                        assertThat(updatedIteration.getCheckTaskStatuses()).isEqualTo(expectStatuses))
                .expectTasks(expectTasks)
                .largeTestsDelegatedConfig(largeTestsDelegatedConfig)
                .nativeBuildsDelegatedConfig(nativeBuildsDelegatedConfig)
                .executor(diffs -> autostartService.tryAutostart(iteration.getId()))
                .reportEmptyTests(true)
                .build();

        var checkTypes = new LargeTest(settings).test();

        if (!expectTasks.isEmpty()) {
            Mockito.verify(ciClient, Mockito.times(checkTypes.size()))
                    .getLastValidConfigBatch(Mockito.any(StorageApi.GetLastValidPrefixedConfigRequest.class));
        }
    }

    @Test
    void tryAutostartLargeTestsBranchNonLaunchable() {

        var largeTests = List.of(new LargeAutostart("some_dir/a.yaml", "a/b/c", List.of("chain-1", "chain-2")));
        var testStatuses = TestStatuses.largeTests(Common.TestStatus.TS_DISCOVERED, Common.TestStatus.TS_DISCOVERED);

        var iteration = sampleIteration.updateCheckTaskStatuses(
                Map.of(Common.CheckTaskType.CTT_LARGE_TEST, CheckTaskStatus.SCHEDULED));

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
                .launchable(false)
                .check(sampleCheckWithRev.toBuilder()
                        .autostartLargeTests(largeTests)
                        .largeTestsConfig(largeTestConfig)
                        .build())
                .testStatuses(testStatuses)
                .saveIteration(() -> iteration)
                .checkIteration(updatedIteration ->
                        assertThat(updatedIteration.getCheckTaskStatuses())
                                .isEqualTo(Map.of(Common.CheckTaskType.CTT_LARGE_TEST, CheckTaskStatus.COMPLETE)))
                .expectTasks(List.of())
                .largeTestsDelegatedConfig(delegatedConfig)
                .nativeBuildsDelegatedConfig(delegatedConfig)
                .executor(diffs -> autostartService.tryAutostart(iteration.getId()))
                .reportEmptyTests(true)
                .build();

        new LargeTest(settings).test();
    }

    private void tryAutostartLargeTestsNoDelegated(
            Boolean runLargeTestsAfterDiscovery,
            List<LargeAutostart> largeTests,
            List<NativeBuild> nativeBuilds,
            TestStatuses testStatuses,
            Map<Common.CheckTaskType, CheckTaskStatus> setStatuses,
            Map<Common.CheckTaskType, CheckTaskStatus> expectStatuses,
            List<LargeStart> expectTasks,
            CheckOuterClass.CheckType checkType) {
        // No delegated configs
        Mockito.when(ciClient.getLastValidConfigBatch(Mockito.any(StorageApi.GetLastValidPrefixedConfigRequest.class)))
                .thenReturn(StorageApi.GetLastValidPrefixedConfigResponse.getDefaultInstance());

        var iteration = sampleIteration.updateCheckTaskStatuses(setStatuses);

        var settings = LargeTestSettings.builder()
                .check(sampleCheckWithRev.toBuilder()
                        .type(checkType)
                        .autostartLargeTests(largeTests)
                        .nativeBuilds(nativeBuilds)
                        .runLargeTestsAfterDiscovery(runLargeTestsAfterDiscovery)
                        .build())
                .testStatuses(testStatuses)
                .saveIteration(() -> iteration)
                .checkIteration(updatedIteration ->
                        assertThat(updatedIteration.getCheckTaskStatuses()).isEqualTo(expectStatuses))
                .expectTasks(expectTasks)
                .executor(diffs -> autostartService.tryAutostart(iteration.getId()))
                .reportEmptyTests(true)
                .build();

        var checkTypes = new LargeTest(settings).test();

        if (!expectTasks.isEmpty()) {
            Mockito.verify(ciClient, Mockito.times(checkTypes.size()))
                    .getLastValidConfigBatch(Mockito.any(StorageApi.GetLastValidPrefixedConfigRequest.class));
        }
    }

    static List<Arguments> allCheckTaskStatuses() {
        var scenarios = new ArrayList<Arguments>();
        scenarios.add(Arguments.of(Map.of(), Map.of()));

        var allTypes = List.of(Common.CheckTaskType.CTT_LARGE_TEST, Common.CheckTaskType.CTT_NATIVE_BUILD);

        for (var status : CheckTaskStatus.values()) {
            var targetStatus = status == CheckTaskStatus.SCHEDULED
                    ? CheckTaskStatus.COMPLETE
                    : status;
            for (var type : allTypes) {
                scenarios.add(Arguments.of(
                        Map.of(type, status),
                        Map.of(type, targetStatus)
                ));
            }

            scenarios.add(Arguments.of(
                    toMap(allTypes, status),
                    toMap(allTypes, targetStatus)
            ));
        }

        scenarios.add(Arguments.of(
                toMap(Common.CheckTaskType.CTT_LARGE_TEST, CheckTaskStatus.SCHEDULED,
                        Common.CheckTaskType.CTT_NATIVE_BUILD, CheckTaskStatus.NOT_REQUIRED),
                toMap(Common.CheckTaskType.CTT_LARGE_TEST, CheckTaskStatus.COMPLETE,
                        Common.CheckTaskType.CTT_NATIVE_BUILD, CheckTaskStatus.NOT_REQUIRED)
        ));

        scenarios.add(Arguments.of(
                toMap(Common.CheckTaskType.CTT_LARGE_TEST, CheckTaskStatus.NOT_REQUIRED,
                        Common.CheckTaskType.CTT_NATIVE_BUILD, CheckTaskStatus.SCHEDULED),
                toMap(Common.CheckTaskType.CTT_LARGE_TEST, CheckTaskStatus.NOT_REQUIRED,
                        Common.CheckTaskType.CTT_NATIVE_BUILD, CheckTaskStatus.COMPLETE)
        ));

        return scenarios;
    }

    private static Map<Common.CheckTaskType, CheckTaskStatus> toMap(List<Common.CheckTaskType> types,
                                                                    CheckTaskStatus status) {
        var map = new LinkedHashMap<Common.CheckTaskType, CheckTaskStatus>();
        for (var type : types) {
            map.put(type, status);
        }
        return map;
    }

    private static Map<Common.CheckTaskType, CheckTaskStatus> toMap(
            Common.CheckTaskType key1,
            CheckTaskStatus value1,
            Common.CheckTaskType key2,
            CheckTaskStatus value2
    ) {
        var map = new LinkedHashMap<Common.CheckTaskType, CheckTaskStatus>();
        map.put(key1, value1);
        map.put(key2, value2);
        return map;
    }

    @SuppressWarnings("MethodLength")
    static List<Arguments> tryAutostartLargeTests() {
        var largeDiscovered = TestStatuses.largeTests(Common.TestStatus.TS_DISCOVERED, Common.TestStatus.TS_DISCOVERED);

        var setLargeTests = linkedMap(Common.CheckTaskType.CTT_LARGE_TEST, CheckTaskStatus.SCHEDULED);
        var expectLargeTests = linkedMap(Common.CheckTaskType.CTT_LARGE_TEST, CheckTaskStatus.COMPLETE);

        // Large tests
        var allArguments = new ArrayList<Arguments>();
        allArguments.addAll(List.of(

                // No matches att all
                Arguments.of(
                        null,
                        List.of(),
                        List.of(),
                        largeDiscovered,
                        setLargeTests,
                        expectLargeTests,
                        List.of()
                ),

                // All matches because runLargeTestsAfterDiscovery = true
                Arguments.of(
                        true,
                        List.of(),
                        List.of(),
                        largeDiscovered,
                        setLargeTests,
                        expectLargeTests,
                        List.of(LargeStart.largeTest("chain-1", false),
                                LargeStart.largeTest("chain-1", true),
                                LargeStart.largeTest("chain-2", false),
                                LargeStart.largeTest("chain-2", true),
                                LargeStart.largeTest("chain-3", false),
                                LargeStart.largeTest("chain-3", true))
                ),


                // No matches
                Arguments.of(
                        null,
                        List.of(new LargeAutostart("some_dir/a.yaml", "a/b/c", List.of("chain-4", "chain-5")),
                                new LargeAutostart("some_dir2/a.yaml", "a/b", List.of()),
                                new LargeAutostart("some_dir3/a.yaml", "a/c", List.of("chain-3"))),
                        List.of(),
                        largeDiscovered,
                        setLargeTests,
                        expectLargeTests,
                        List.of()
                ),

                // No matches, explicit runLargeTestsAfterDiscovery = false
                Arguments.of(
                        false,
                        List.of(new LargeAutostart("some_dir/a.yaml", "a/b/c", List.of("chain-4", "chain-5")),
                                new LargeAutostart("some_dir2/a.yaml", "a/b", List.of()),
                                new LargeAutostart("some_dir3/a.yaml", "a/c", List.of("chain-3"))),
                        List.of(),
                        largeDiscovered,
                        setLargeTests,
                        expectLargeTests,
                        List.of()
                ),

                // All matches because runLargeTestsAfterDiscovery = true
                Arguments.of(
                        true,
                        List.of(new LargeAutostart("some_dir/a.yaml", "a/b/c", List.of("chain-4", "chain-5")),
                                new LargeAutostart("some_dir2/a.yaml", "a/b", List.of()),
                                new LargeAutostart("some_dir3/a.yaml", "a/c", List.of("chain-3"))),
                        List.of(),
                        largeDiscovered,
                        setLargeTests,
                        expectLargeTests,
                        List.of(LargeStart.largeTest("chain-1", false),
                                LargeStart.largeTest("chain-1", true),
                                LargeStart.largeTest("chain-2", false),
                                LargeStart.largeTest("chain-2", true),
                                LargeStart.largeTest("chain-3", false),
                                LargeStart.largeTest("chain-3", true))
                ),

                // Default match
                Arguments.of(
                        null,
                        List.of(new LargeAutostart("some_dir/a.yaml", "a/b/c", List.of("chain-1", "chain-2"))),
                        List.of(),
                        largeDiscovered,
                        setLargeTests,
                        expectLargeTests,
                        List.of(LargeStart.largeTest("chain-1", false),
                                LargeStart.largeTest("chain-1", true),
                                LargeStart.largeTest("chain-2", false),
                                LargeStart.largeTest("chain-2", true))
                ),

                // Default single match
                Arguments.of(
                        null,
                        List.of(new LargeAutostart("some_dir/a.yaml", "a/b/c", List.of("chain-3"))),
                        List.of(),
                        largeDiscovered,
                        setLargeTests,
                        expectLargeTests,
                        List.of(LargeStart.largeTest("chain-3", false),
                                LargeStart.largeTest("chain-3", true))
                ),

                // All matches
                Arguments.of(
                        null,
                        List.of(new LargeAutostart("some_dir/a.yaml", "a/b/c", List.of())),
                        List.of(),
                        largeDiscovered,
                        setLargeTests,
                        expectLargeTests,
                        List.of(LargeStart.largeTest("chain-1", false),
                                LargeStart.largeTest("chain-1", true),
                                LargeStart.largeTest("chain-2", false),
                                LargeStart.largeTest("chain-2", true),
                                LargeStart.largeTest("chain-3", false),
                                LargeStart.largeTest("chain-3", true))
                ),

                // All matches with mixed settings
                Arguments.of(
                        null,
                        List.of(new LargeAutostart("some_dir/a.yaml", "a/b/c", List.of("chain-2")),
                                new LargeAutostart("some_dir2/a.yaml", "a/b/c", List.of()), // will overwrite other
                                // settings
                                new LargeAutostart("some_dir3/a.yaml", "a/b/c", List.of("chain-3"))),
                        List.of(),
                        largeDiscovered,
                        setLargeTests,
                        expectLargeTests,
                        List.of(LargeStart.largeTest("chain-1", false),
                                LargeStart.largeTest("chain-1", true),
                                LargeStart.largeTest("chain-2", false),
                                LargeStart.largeTest("chain-2", true),
                                LargeStart.largeTest("chain-3", false),
                                LargeStart.largeTest("chain-3", true))
                ),

                // Partial match
                Arguments.of(
                        null,
                        List.of(new LargeAutostart("some_dir/a.yaml", "a/b/c", List.of("chain-1", "chain-2"))),
                        List.of(),
                        TestStatuses.largeTests(Common.TestStatus.TS_INTERNAL, Common.TestStatus.TS_DISCOVERED),
                        setLargeTests,
                        expectLargeTests,
                        List.of(LargeStart.largeTest("chain-1", true),
                                LargeStart.largeTest("chain-2", true))
                ),

                // Partial full match
                Arguments.of(
                        null,
                        List.of(new LargeAutostart("some_dir/a.yaml", "a/b/c", List.of())),
                        List.of(),
                        TestStatuses.largeTests(Common.TestStatus.TS_DISCOVERED, Common.TestStatus.TS_INTERNAL),
                        setLargeTests,
                        expectLargeTests,
                        List.of(LargeStart.largeTest("chain-1", false),
                                LargeStart.largeTest("chain-2", false),
                                LargeStart.largeTest("chain-3", false))
                ),

                // None match
                Arguments.of(
                        null,
                        List.of(new LargeAutostart("some_dir/a.yaml", "a/b/c", List.of())),
                        List.of(),
                        TestStatuses.largeTests(Common.TestStatus.TS_INTERNAL, Common.TestStatus.TS_INTERNAL),
                        setLargeTests,
                        expectLargeTests,
                        List.of()
                ),

                // None match even with runLargeTestsAfterDiscovery = true
                Arguments.of(
                        true,
                        List.of(new LargeAutostart("some_dir/a.yaml", "a/b/c", List.of())),
                        List.of(),
                        TestStatuses.largeTests(Common.TestStatus.TS_INTERNAL, Common.TestStatus.TS_INTERNAL),
                        setLargeTests,
                        expectLargeTests,
                        List.of()
                ),

                // Inappropriate status COMPLETE
                Arguments.of(
                        null,
                        List.of(new LargeAutostart("some_dir/a.yaml", "a/b/c", List.of("chain-3"))),
                        List.of(),
                        largeDiscovered,
                        linkedMap(Common.CheckTaskType.CTT_LARGE_TEST, CheckTaskStatus.COMPLETE),
                        linkedMap(Common.CheckTaskType.CTT_LARGE_TEST, CheckTaskStatus.COMPLETE),
                        List.of()
                ),

                // Inappropriate status NOT_REQUIRED
                Arguments.of(
                        null,
                        List.of(new LargeAutostart("some_dir/a.yaml", "a/b/c", List.of("chain-3"))),
                        List.of(),
                        largeDiscovered,
                        linkedMap(Common.CheckTaskType.CTT_LARGE_TEST, CheckTaskStatus.NOT_REQUIRED),
                        linkedMap(Common.CheckTaskType.CTT_LARGE_TEST, CheckTaskStatus.NOT_REQUIRED),
                        List.of()
                ),

                // Inappropriate status null
                Arguments.of(
                        null,
                        List.of(new LargeAutostart("some_dir/a.yaml", "a/b/c", List.of("chain-3"))),
                        List.of(),
                        largeDiscovered,
                        linkedMap(),
                        linkedMap(),
                        List.of()
                ),

                // Glob single match
                Arguments.of(
                        null,
                        List.of(new LargeAutostart("some_dir/a.yaml", "a/b/*", List.of("chain-3"))),
                        List.of(),
                        largeDiscovered,
                        setLargeTests,
                        expectLargeTests,
                        List.of(LargeStart.largeTest("chain-3", false),
                                LargeStart.largeTest("chain-3", true))
                ),

                Arguments.of(
                        null,
                        List.of(new LargeAutostart("some_dir/a.yaml", "a/**", List.of("chain-3"))),
                        List.of(),
                        largeDiscovered,
                        setLargeTests,
                        expectLargeTests,
                        List.of(LargeStart.largeTest("chain-3", false),
                                LargeStart.largeTest("chain-3", true))
                ),

                // Testenv compatible
                Arguments.of(
                        null,
                        List.of(new LargeAutostart("some_dir/a.yaml", "a/*", List.of("chain-3"))),
                        List.of(),
                        largeDiscovered,
                        setLargeTests,
                        expectLargeTests,
                        List.of(LargeStart.largeTest("chain-3", false),
                                LargeStart.largeTest("chain-3", true))
                ),
                Arguments.of(
                        null,
                        List.of(new LargeAutostart("some_dir/a.yaml", "a/*/*", List.of("chain-3"))),
                        List.of(),
                        largeDiscovered,
                        setLargeTests,
                        expectLargeTests,
                        List.of(LargeStart.largeTest("chain-3", false),
                                LargeStart.largeTest("chain-3", true))
                ),
                Arguments.of(
                        null,
                        List.of(new LargeAutostart("some_dir/a.yaml", "*/c", List.of("chain-3"))),
                        List.of(),
                        largeDiscovered,
                        setLargeTests,
                        expectLargeTests,
                        List.of(LargeStart.largeTest("chain-3", false),
                                LargeStart.largeTest("chain-3", true))
                ),
                Arguments.of(
                        null,
                        List.of(new LargeAutostart("some_dir/a.yaml", "*/d", List.of("chain-3"))),
                        List.of(),
                        largeDiscovered,
                        setLargeTests,
                        expectLargeTests,
                        List.of()
                )
        ));


        var nativeOk = TestStatuses.nativeBuilds(Common.TestStatus.TS_OK, Common.TestStatus.TS_OK);
        var setNativeBuilds = linkedMap(Common.CheckTaskType.CTT_NATIVE_BUILD, CheckTaskStatus.SCHEDULED);
        var expectNativeBuilds = linkedMap(Common.CheckTaskType.CTT_NATIVE_BUILD, CheckTaskStatus.COMPLETE);

        // Native builds
        allArguments.addAll(List.of(
                // No matches
                Arguments.of(
                        null,
                        List.of(),
                        List.of(new NativeBuild("some_dir/a/a.yaml", "chain-1", List.of("a", "a/b/c")),
                                new NativeBuild("some_dir/a/a.yaml", "chain-2", List.of("a")),
                                new NativeBuild("some_dir/b/a.yaml", "chain-1", List.of("c/a"))),
                        nativeOk,
                        setNativeBuilds,
                        expectNativeBuilds,
                        List.of()
                ),

                // No glob allowed
                Arguments.of(
                        null,
                        List.of(),
                        List.of(new NativeBuild("some_dir/a/a.yaml", "chain-1", List.of("a/*"))),
                        nativeOk,
                        setNativeBuilds,
                        expectNativeBuilds,
                        List.of()
                ),

                // Default match
                Arguments.of(
                        null,
                        List.of(),
                        List.of(new NativeBuild("some_dir/a/a.yaml", "chain-1", List.of("a/b"))),
                        nativeOk,
                        setNativeBuilds,
                        expectNativeBuilds,
                        List.of(LargeStart.nativeBuild("a_chain-1_a-b", false),
                                LargeStart.nativeBuild("a_chain-1_a-b", true))
                ),

                Arguments.of(
                        null,
                        List.of(),
                        List.of(new NativeBuild("some_dir/a/a.yaml", "chain-1", List.of("a/b"))),
                        TestStatuses.nativeBuilds(Common.TestStatus.TS_INTERNAL, Common.TestStatus.TS_OK),
                        setNativeBuilds,
                        expectNativeBuilds,
                        List.of(LargeStart.nativeBuild("a_chain-1_a-b", true))
                ),

                Arguments.of(
                        null,
                        List.of(),
                        List.of(new NativeBuild("some_dir/a/a.yaml", "chain-1", List.of("a/b"))),
                        TestStatuses.nativeBuilds(Common.TestStatus.TS_OK, Common.TestStatus.TS_INTERNAL),
                        setNativeBuilds,
                        expectNativeBuilds,
                        List.of(LargeStart.nativeBuild("a_chain-1_a-b", false))
                ),

                Arguments.of(
                        null,
                        List.of(),
                        List.of(new NativeBuild("some_dir/a/a.yaml", "chain-1", List.of("a/b", "b/c"))),
                        nativeOk,
                        setNativeBuilds,
                        expectNativeBuilds,
                        List.of(LargeStart.nativeBuild("a_chain-1_a-b_b-c", false),
                                LargeStart.nativeBuild("a_chain-1_a-b_b-c", true))
                ),

                Arguments.of(
                        null,
                        List.of(),
                        List.of(new NativeBuild("some_dir/a/a.yaml", "chain-1", List.of("a/b", "b/c")),
                                new NativeBuild("some_dir/a/a.yaml", "chain-2", List.of("a/b", "b/c")),
                                new NativeBuild("some_dir/b/a.yaml", "chain-1", List.of("a/b"))),
                        nativeOk,
                        setNativeBuilds,
                        expectNativeBuilds,
                        List.of(LargeStart.nativeBuild("a_chain-1_a-b_b-c", false),
                                LargeStart.nativeBuild("a_chain-1_a-b_b-c", true),
                                LargeStart.nativeBuild("a_chain-2_a-b_b-c", false),
                                LargeStart.nativeBuild("a_chain-2_a-b_b-c", true),
                                LargeStart.nativeBuild("b_chain-1_a-b", false),
                                LargeStart.nativeBuild("b_chain-1_a-b", true))
                )
        ));


        // Both large and native builds
        allArguments.addAll(List.of(
                // Default match
                Arguments.of(
                        null,
                        List.of(new LargeAutostart("some_dir/a.yaml", "a/b/c", List.of("chain-1", "chain-2"))),
                        List.of(new NativeBuild("some_dir/a/a.yaml", "chain-1", List.of("a/b"))),
                        TestStatuses.all(Common.TestStatus.TS_DISCOVERED, Common.TestStatus.TS_DISCOVERED,
                                Common.TestStatus.TS_OK, Common.TestStatus.TS_OK),
                        linkedMap(
                                Common.CheckTaskType.CTT_LARGE_TEST, CheckTaskStatus.SCHEDULED,
                                Common.CheckTaskType.CTT_NATIVE_BUILD, CheckTaskStatus.SCHEDULED),
                        linkedMap(
                                Common.CheckTaskType.CTT_LARGE_TEST, CheckTaskStatus.COMPLETE,
                                Common.CheckTaskType.CTT_NATIVE_BUILD, CheckTaskStatus.COMPLETE),
                        List.of(LargeStart.largeTest("chain-1", false),
                                LargeStart.largeTest("chain-1", true),
                                LargeStart.largeTest("chain-2", false),
                                LargeStart.largeTest("chain-2", true),
                                // native builds starts in new iteration
                                LargeStart.nativeBuild("a_chain-1_a-b", false),
                                LargeStart.nativeBuild("a_chain-1_a-b", true))
                ),

                // All large tests match because runLargeTestsAfterDiscovery = true
                Arguments.of(
                        true,
                        List.of(new LargeAutostart("some_dir/a.yaml", "a/b/c", List.of("chain-1", "chain-2"))),
                        List.of(new NativeBuild("some_dir/a/a.yaml", "chain-1", List.of("a/b"))),
                        TestStatuses.all(Common.TestStatus.TS_DISCOVERED, Common.TestStatus.TS_DISCOVERED,
                                Common.TestStatus.TS_OK, Common.TestStatus.TS_OK),
                        linkedMap(
                                Common.CheckTaskType.CTT_LARGE_TEST, CheckTaskStatus.SCHEDULED,
                                Common.CheckTaskType.CTT_NATIVE_BUILD, CheckTaskStatus.SCHEDULED),
                        linkedMap(
                                Common.CheckTaskType.CTT_LARGE_TEST, CheckTaskStatus.COMPLETE,
                                Common.CheckTaskType.CTT_NATIVE_BUILD, CheckTaskStatus.COMPLETE),
                        List.of(LargeStart.largeTest("chain-1", false),
                                LargeStart.largeTest("chain-1", true),
                                LargeStart.largeTest("chain-2", false),
                                LargeStart.largeTest("chain-2", true),
                                LargeStart.largeTest("chain-3", false),
                                LargeStart.largeTest("chain-3", true),
                                LargeStart.nativeBuild("a_chain-1_a-b", false),
                                LargeStart.nativeBuild("a_chain-1_a-b", true))
                )
        ));

        return allArguments;
    }
}
