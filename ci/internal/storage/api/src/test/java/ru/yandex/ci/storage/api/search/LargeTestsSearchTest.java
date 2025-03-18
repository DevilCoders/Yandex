package ru.yandex.ci.storage.api.search;

import java.time.Instant;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.EnumSource;
import org.junit.jupiter.params.provider.MethodSource;

import ru.yandex.ci.storage.api.StorageFrontApi.LargeTestResponse;
import ru.yandex.ci.storage.api.StorageFrontApi.LargeTestStatus;
import ru.yandex.ci.storage.api.StorageFrontApi.LargeTestToolchain;
import ru.yandex.ci.storage.api.StorageFrontApi.ListLargeTestsToolchainsRequest;
import ru.yandex.ci.storage.api.StorageFrontApi.SearchLargeTestsRequest;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.StorageYdbTestBase;
import ru.yandex.ci.storage.core.check.CheckTaskTypeUtils;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffByHashEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffEntity;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;

import static org.assertj.core.api.Assertions.assertThat;
import static ru.yandex.ci.storage.api.StorageFrontApi.LargeTestStatus.LTS_ALL;
import static ru.yandex.ci.storage.api.StorageFrontApi.LargeTestStatus.LTS_DISCOVERED;
import static ru.yandex.ci.storage.api.StorageFrontApi.LargeTestStatus.LTS_FAILURE;
import static ru.yandex.ci.storage.api.StorageFrontApi.LargeTestStatus.LTS_RUNNING;
import static ru.yandex.ci.storage.api.StorageFrontApi.LargeTestStatus.LTS_SUCCESS;
import static ru.yandex.ci.storage.core.Common.ResultType.RT_TEST_SUITE_LARGE;
import static ru.yandex.ci.storage.core.Common.ResultType.RT_TEST_SUITE_MEDIUM;
import static ru.yandex.ci.storage.core.Common.TestStatus.TS_DISCOVERED;
import static ru.yandex.ci.storage.core.Common.TestStatus.TS_NONE;

class LargeTestsSearchTest extends StorageYdbTestBase {

    private static final CheckEntity.Id CHECK_ID = CheckEntity.Id.of(1L);
    private static final CheckIterationEntity.Id ITERATION_ID = CheckIterationEntity.Id.of(
            CHECK_ID, CheckIteration.IterationType.FULL, 1
    );

    private static final ChunkAggregateEntity.Id AGGREGATE_ID = new ChunkAggregateEntity.Id(
            ITERATION_ID, ChunkEntity.Id.of(Common.ChunkType.CT_LARGE_TEST, 1)
    );

    private static final TestDiffByHashEntity TEST_ABC1 =
            TestDiffByHashEntity.builder()
                    .id(createId(3L, "chain-1", null))
                    .path("a/b/c")
                    .isLast(true)
                    .resultType(RT_TEST_SUITE_LARGE)
                    .left(TS_NONE)
                    .right(TS_DISCOVERED)
                    .isLaunchable(true) // Make sure it's configured
                    .build();
    private static final TestDiffByHashEntity TEST_ABC2 =
            TestDiffByHashEntity.builder()
                    .id(createId(3L, "chain-2", null)) // Same as previous but with different toolchain
                    .path("a/b/c")
                    .isLast(true)
                    .resultType(RT_TEST_SUITE_LARGE)
                    .left(TS_DISCOVERED)
                    .right(TS_NONE)
                    .isLaunchable(true)
                    .build();
    private static final TestDiffByHashEntity TEST_ABCDE1 =
            TestDiffByHashEntity.builder()
                    .id(createId(5L, "chain-1", null)) // Same as previous but with different toolchain
                    .path("a/b/c/d/e")
                    .isLast(true)
                    .resultType(RT_TEST_SUITE_LARGE)
                    .left(TS_DISCOVERED)
                    .right(TS_DISCOVERED)
                    .isLaunchable(true)
                    .build();

    private LargeTestsSearch search;

    @BeforeEach
    void init() {
        search = new LargeTestsSearch(db);
        registerCheck();
    }

    @Test
    void searchLargeTestsNoCheck() {
        var request = SearchLargeTestsRequest.newBuilder()
                .setCheckId("12345")
                .build();
        assertThat(search.searchLargeTests(request).getTestsList())
                .isEmpty();
    }

    @ParameterizedTest
    @EnumSource(LargeTestStatus.class)
    void searchLargeTestsDiscoveredOnlyWithoutData(LargeTestStatus status) {
        var request = SearchLargeTestsRequest.newBuilder()
                .setCheckId(sampleCheck.getId().toString());
        if (status != LargeTestStatus.UNRECOGNIZED) {
            request.setLargeTestsStatus(status);
        }
        var response = search.searchLargeTests(request.build());
        assertThat(response.getTestsList())
                .isEqualTo(List.of());
    }

    @ParameterizedTest
    @MethodSource("searchLargeTestsScenarios")
    void searchLargeTestsScenariosLegacyJobNames(
            String id,
            boolean discovered,
            boolean registered,
            SearchLargeTestsRequest request,
            List<LargeTestResponse> expect
    ) {
        searchLargeTestsScenarios(discovered, registered, request, expect, true);
    }

    @ParameterizedTest
    @MethodSource("searchLargeTestsScenarios")
    void searchLargeTestsScenariosNewJobNames(
            String id,
            boolean discovered,
            boolean registered,
            SearchLargeTestsRequest request,
            List<LargeTestResponse> expect
    ) {
        searchLargeTestsScenarios(discovered, registered, request, expect, false);
    }

    @Test
    void listToolchainsNoCheck() {
        var request = ListLargeTestsToolchainsRequest.newBuilder()
                .setCheckId("12345")
                .build();
        assertThat(search.listToolchains(request).getToolchainsList())
                .isEmpty();
    }

    @ParameterizedTest
    @MethodSource
    void listToolchainsScenarios(String id,
                                 boolean discovered,
                                 ListLargeTestsToolchainsRequest request,
                                 List<LargeTestToolchain> expect) {
        if (discovered) {
            createDiscoveredData();
        }
        var response = search.listToolchains(request);
        assertThat(response.getToolchainsList()).isEqualTo(expect);
    }

    private void searchLargeTestsScenarios(
            boolean discovered,
            boolean registered,
            SearchLargeTestsRequest request,
            List<LargeTestResponse> expect,
            boolean legacyJobNames
    ) {
        if (discovered) {
            createDiscoveredData();
        }
        if (registered) {
            createRegisteredData(legacyJobNames);
        }
        var response = search.searchLargeTests(request);
        assertThat(response.getTestsList()).isEqualTo(expect);
    }

    private void registerCheck() {
        db.currentOrTx(() -> db.checks().save(sampleCheck));
    }

    private void createDiscoveredData() {
        var diffs = List.of(
                TestDiffByHashEntity.builder()
                        .id(createId(1L, "chain-1", null))
                        .path("a/b/c")
                        .isLast(true)
                        .resultType(RT_TEST_SUITE_MEDIUM) // must be completely ignored
                        .left(TS_DISCOVERED)
                        .right(TS_DISCOVERED)
                        .isLaunchable(true)
                        .build(),
                TestDiffByHashEntity.builder()
                        .id(createId(2L, "chain-1", null))
                        .path("a/b/c")
                        .isLast(false) // must be ignored, not last, overwritten
                        .resultType(RT_TEST_SUITE_LARGE)
                        .left(TS_DISCOVERED)
                        .right(TS_DISCOVERED)
                        .build(),
                TEST_ABC1,
                TEST_ABC2,
                TestDiffByHashEntity.builder()
                        .id(createId(4L, "chain-1", null))
                        .path("a/b/c/d")
                        .isLast(false) // must be ignored, not last and not overwritten
                        .resultType(RT_TEST_SUITE_LARGE)
                        .left(TS_DISCOVERED)
                        .right(TS_DISCOVERED)
                        .build(),
                TEST_ABCDE1,
                TestDiffByHashEntity.builder()
                        .id(createId(4L, "chain-1", null))
                        .path("a/b/c/d")
                        .isLast(true)
                        .resultType(RT_TEST_SUITE_LARGE)
                        .left(TS_DISCOVERED)
                        .right(TS_DISCOVERED)
                        .isLaunchable(false)  // must be ignored, not launchable
                        .build()
        );

        db.currentOrTx(() ->
                this.db.testDiffs().save(diffs.stream()
                        .map(TestDiffEntity::new)
                        .collect(Collectors.toList())
                ));
    }

    private void createRegisteredData(boolean legacyJobName) {
        var heavyIterationId = CheckIterationEntity.Id.of(CHECK_ID, CheckIteration.IterationType.HEAVY, 1);

        var tasks = List.of(
                CheckTaskEntity.builder()
                        .id(new CheckTaskEntity.Id(heavyIterationId, "1"))
                        .right(true)
                        .jobName(
                                legacyJobName
                                        ? "a/b/c:chain-1"
                                        :
                                        CheckTaskTypeUtils.toCiProcessId(
                                                Common.CheckTaskType.CTT_LARGE_TEST,
                                                "a/b/c",
                                                null,
                                                "chain-1",
                                                "java").toString()
                        )
                        .completedPartitions(Set.of())
                        .numberOfPartitions(1)
                        .status(Common.CheckStatus.CREATED)
                        .created(Instant.now())
                        .type(Common.CheckTaskType.CTT_LARGE_TEST)
                        .build(),
                CheckTaskEntity.builder()
                        .id(new CheckTaskEntity.Id(heavyIterationId, "2"))
                        .right(false)
                        .jobName(
                                legacyJobName
                                        ? "a/b/c/d/e:chain-1"
                                        :
                                        CheckTaskTypeUtils.toCiProcessId(
                                                Common.CheckTaskType.CTT_LARGE_TEST,
                                                "a/b/c/d/e",
                                                null,
                                                "chain-1",
                                                "java").toString()
                        )
                        .completedPartitions(Set.of())
                        .numberOfPartitions(1)
                        .status(Common.CheckStatus.COMPLETED_SUCCESS)
                        .created(Instant.now())
                        .type(Common.CheckTaskType.CTT_LARGE_TEST)
                        .build(),
                CheckTaskEntity.builder()
                        .id(new CheckTaskEntity.Id(heavyIterationId, "3"))
                        .right(true)
                        .jobName(
                                legacyJobName
                                        ? "a/b/c/d/e:chain-1"
                                        :
                                        CheckTaskTypeUtils.toCiProcessId(
                                                Common.CheckTaskType.CTT_LARGE_TEST,
                                                "a/b/c/d/e",
                                                null,
                                                "chain-1",
                                                "java").toString()
                        )
                        .completedPartitions(Set.of())
                        .numberOfPartitions(1)
                        .status(Common.CheckStatus.COMPLETED_FAILED)
                        .created(Instant.now())
                        .type(Common.CheckTaskType.CTT_LARGE_TEST)
                        .build()
        );

        db.currentOrTx(() ->
                this.db.checkTasks().save(tasks)
        );
    }

    private static TestDiffByHashEntity.Id createId(long suiteId, String toolchain, @Nullable Long testId) {
        return TestDiffByHashEntity.Id.of(
                AGGREGATE_ID, new TestEntity.Id(suiteId, toolchain, testId == null ? suiteId : testId)
        );
    }

    private static LargeTestResponse response(@Nonnull TestDiffByHashEntity hashEntity,
                                              @Nullable LargeTestStatus left,
                                              @Nullable LargeTestStatus right) {
        var id = new TestDiffEntity(hashEntity).getId();
        var ret = LargeTestResponse.newBuilder()
                .setTestDiff(CheckProtoMappers.toProtoDiffId(id));
        if (left != null) {
            ret.setLeftStatus(left);
        }
        if (right != null) {
            ret.setRightStatus(right);
        }
        return ret.build();
    }

    private static List<LargeTestToolchain> toolchains(String... toolchains) {
        return Stream.of(toolchains)
                .map(toolchain -> LargeTestToolchain.newBuilder().setToolchain(toolchain).build())
                .toList();
    }

    @SuppressWarnings("MethodLength")
    static List<Arguments> searchLargeTestsScenarios() {
        var base = SearchLargeTestsRequest.newBuilder()
                .setCheckId("1")  // sampleCheckId
                .build();

        var withPathExact = base.toBuilder()
                .setPath("a/b/c")
                .build();

        var withPathLike = base.toBuilder()
                .setPath("a/b/c*")
                .build();

        var withToolchain = base.toBuilder()
                .setToolchain("chain-1")
                .build();

        var withPathAndToolchain = base.toBuilder()
                .setPath("a/b/c")
                .setToolchain("chain-1")
                .build();

        var withLargeTestAll = base.toBuilder()
                .setLargeTestsStatus(LTS_ALL)
                .build();
        var withDiscovered = base.toBuilder()
                .setLargeTestsStatus(LTS_DISCOVERED)
                .build();
        var withRunning = base.toBuilder()
                .setLargeTestsStatus(LTS_RUNNING)
                .build();
        var withSuccess = base.toBuilder()
                .setLargeTestsStatus(LTS_SUCCESS)
                .build();
        var withFailure = base.toBuilder()
                .setLargeTestsStatus(LTS_FAILURE)
                .build();

        return List.of(
                // discovered, created, request, expected rows
                Arguments.of("1", false, false, base, List.of()), // No data
                Arguments.of("2", true, false, base,
                        List.of(
                                response(TEST_ABC1, null, LTS_DISCOVERED),
                                response(TEST_ABC2, LTS_DISCOVERED, null),
                                response(TEST_ABCDE1, LTS_DISCOVERED, LTS_DISCOVERED)
                        )),
                Arguments.of("3", true, false, withLargeTestAll,
                        List.of(
                                response(TEST_ABC1, null, LTS_DISCOVERED),
                                response(TEST_ABC2, LTS_DISCOVERED, null),
                                response(TEST_ABCDE1, LTS_DISCOVERED, LTS_DISCOVERED)
                        )),
                Arguments.of("4", true, false, withDiscovered,
                        List.of(
                                response(TEST_ABC1, null, LTS_DISCOVERED),
                                response(TEST_ABC2, LTS_DISCOVERED, null),
                                response(TEST_ABCDE1, LTS_DISCOVERED, LTS_DISCOVERED)
                        )),
                Arguments.of("5", true, false, withRunning,
                        List.of()),
                Arguments.of("6", true, false, withSuccess,
                        List.of()),
                Arguments.of("7", true, false, withFailure,
                        List.of()),

                Arguments.of("8", false, true, base,
                        List.of()), // No data will be selected if no discovered tests found

                Arguments.of("14", true, true, base,
                        List.of(
                                response(TEST_ABC1, null, LTS_RUNNING),
                                response(TEST_ABC2, LTS_DISCOVERED, null),
                                response(TEST_ABCDE1, LTS_SUCCESS, LTS_FAILURE)
                        )),
                Arguments.of("15", true, true, withLargeTestAll,
                        List.of(
                                response(TEST_ABC1, null, LTS_RUNNING),
                                response(TEST_ABC2, LTS_DISCOVERED, null),
                                response(TEST_ABCDE1, LTS_SUCCESS, LTS_FAILURE)
                        )),
                Arguments.of("16", true, true, withDiscovered,
                        List.of(
                                response(TEST_ABC2, LTS_DISCOVERED, null)
                        )),
                Arguments.of("17", true, true, withRunning,
                        List.of(
                                response(TEST_ABC1, null, LTS_RUNNING)
                        )),
                Arguments.of("18", true, true, withSuccess,
                        List.of(
                                response(TEST_ABCDE1, LTS_SUCCESS, LTS_FAILURE)
                        )),
                Arguments.of("19", true, true, withFailure,
                        List.of(
                                response(TEST_ABCDE1, LTS_SUCCESS, LTS_FAILURE)
                        )),

                Arguments.of("20", true, false, withPathExact,
                        List.of(
                                response(TEST_ABC1, null, LTS_DISCOVERED),
                                response(TEST_ABC2, LTS_DISCOVERED, null)
                        )),
                Arguments.of("22", true, true, withPathExact,
                        List.of(
                                response(TEST_ABC1, null, LTS_RUNNING),
                                response(TEST_ABC2, LTS_DISCOVERED, null)
                        )),

                Arguments.of("23", true, false, withPathLike,
                        List.of(
                                response(TEST_ABC1, null, LTS_DISCOVERED),
                                response(TEST_ABC2, LTS_DISCOVERED, null),
                                response(TEST_ABCDE1, LTS_DISCOVERED, LTS_DISCOVERED)
                        )),
                Arguments.of("25", true, true, withPathLike,
                        List.of(
                                response(TEST_ABC1, null, LTS_RUNNING),
                                response(TEST_ABC2, LTS_DISCOVERED, null),
                                response(TEST_ABCDE1, LTS_SUCCESS, LTS_FAILURE)
                        )),

                Arguments.of("26", true, false, withToolchain,
                        List.of(
                                response(TEST_ABC1, null, LTS_DISCOVERED),
                                response(TEST_ABCDE1, LTS_DISCOVERED, LTS_DISCOVERED)
                        )),
                Arguments.of("28", true, true, withToolchain,
                        List.of(
                                response(TEST_ABC1, null, LTS_RUNNING),
                                response(TEST_ABCDE1, LTS_SUCCESS, LTS_FAILURE)
                        )),

                Arguments.of("29", true, false, withPathAndToolchain,
                        List.of(
                                response(TEST_ABC1, null, LTS_DISCOVERED)
                        )),
                Arguments.of("31", true, true, withPathAndToolchain,
                        List.of(
                                response(TEST_ABC1, null, LTS_RUNNING)
                        ))

        );

    }

    static List<Arguments> listToolchainsScenarios() {
        var base = ListLargeTestsToolchainsRequest.newBuilder()
                .setCheckId("1")  // sampleCheckId
                .build();

        return List.of(
                Arguments.of("1", false, base, toolchains()), // No data
                Arguments.of("2", true, base, toolchains("chain-1", "chain-2")) // Default data
        );
    }
}
