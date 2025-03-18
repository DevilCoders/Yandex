package ru.yandex.ci.storage.api.controllers;

import java.time.Instant;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.junit.jupiter.MockitoExtension;

import ru.yandex.ci.common.info.InfoPanelOuterClass;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.flow.CiActionReference;
import ru.yandex.ci.core.flow.FlowUrls;
import ru.yandex.ci.storage.api.ApiTestBase;
import ru.yandex.ci.storage.api.StorageFrontApi;
import ru.yandex.ci.storage.api.StorageFrontApi.ListLargeTestsToolchainsRequest;
import ru.yandex.ci.storage.api.StorageFrontApi.ListLargeTestsToolchainsResponse;
import ru.yandex.ci.storage.api.StorageFrontApi.SearchLargeTestsRequest;
import ru.yandex.ci.storage.api.StorageFrontApi.SearchLargeTestsResponse;
import ru.yandex.ci.storage.api.StorageFrontApi.StartLargeTestsRequest;
import ru.yandex.ci.storage.api.StorageFrontApiServiceGrpc;
import ru.yandex.ci.storage.api.check.ApiCheckService;
import ru.yandex.ci.storage.api.search.LargeTestsSearch;
import ru.yandex.ci.storage.api.search.SearchService;
import ru.yandex.ci.storage.api.util.CheckAttributesCollector;
import ru.yandex.ci.storage.api.util.TestRunCommandProvider;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckOuterClass.CheckType;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.check.RequirementsService;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check.SuspiciousAlert;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.IterationInfo;
import ru.yandex.ci.storage.core.db.model.check_iteration.IterationStatistics;
import ru.yandex.ci.storage.core.db.model.check_iteration.StrongModePolicy;
import ru.yandex.ci.storage.core.db.model.check_iteration.metrics.Metric;
import ru.yandex.ci.storage.core.db.model.check_iteration.metrics.Metrics;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffByHashEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffEntity;
import ru.yandex.ci.storage.core.large.LargeStartService;
import ru.yandex.ci.storage.core.logbroker.event_producer.StorageEventsProducer;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.core.sharding.ShardingSettings;
import ru.yandex.ci.test.clock.OverridableClock;
import ru.yandex.commune.bazinga.BazingaTaskManager;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

@ExtendWith(MockitoExtension.class)
class StorageFrontApiControllerTest extends ApiTestBase {
    private StorageFrontApiServiceGrpc.StorageFrontApiServiceBlockingStub stub;

    private final CheckEntity checkOne = CheckEntity.builder()
            .id(CheckEntity.Id.of(1L))
            .left(new StorageRevision("trunk", "left", 0, Instant.EPOCH))
            .right(new StorageRevision("trunk", "7c2f8d6acb6159d1bfac277b8e566f45f4cbde46", 0, Instant.EPOCH))
            .diffSetId(1L)
            .author("author")
            .created(Instant.now())
            .type(CheckType.TRUNK_POST_COMMIT)
            .build();

    private final CheckEntity checkTwo = CheckEntity.builder()
            .id(CheckEntity.Id.of(2L))
            .left(new StorageRevision("trunk", "leftTwo", 0, Instant.EPOCH))
            .right(new StorageRevision("trunk", "rightTwo", 0, Instant.EPOCH))
            .diffSetId(1L)
            .author("author")
            .created(Instant.now())
            .type(CheckType.TRUNK_POST_COMMIT)
            .build();

    private final CheckEntity checkSuspiciousThree = CheckEntity.builder()
            .id(CheckEntity.Id.of(3L))
            .status(Common.CheckStatus.COMPLETED_SUCCESS)
            .left(new StorageRevision("trunk", "1", 0, Instant.EPOCH))
            .right(new StorageRevision("pr:1", "1", 0, Instant.EPOCH))
            .diffSetId(1L)
            .author("author")
            .created(Instant.now())
            .type(CheckType.TRUNK_PRE_COMMIT)
            .suspiciousAlerts(Set.of(new SuspiciousAlert("test", "There are suspicious iterations")))
            .build();

    private final CheckIterationEntity iterationOne = CheckIterationEntity.builder()
            .id(CheckIterationEntity.Id.of(checkOne.getId(), CheckIteration.IterationType.FAST, 1))
            .status(Common.CheckStatus.RUNNING)
            .statistics(IterationStatistics.EMPTY.withMetrics(new Metrics(Map.of(
                    "time_spent", new Metric(47.2, Common.MetricAggregateFunction.SUM, Common.MetricSize.SECONDS),
                    "mem_max", new Metric(1200000, Common.MetricAggregateFunction.MAX, Common.MetricSize.BYTES),
                    "slot_time", new Metric(5.16 * 60 * 60 /* 5.16 hours */,
                            Common.MetricAggregateFunction.SUM, Common.MetricSize.SECONDS)
            ))))
            .info(
                    IterationInfo.EMPTY.toBuilder()
                            .ciActionReferences(
                                    List.of(
                                            new CiActionReference(
                                                    new FlowFullId("test-dir", "test-id"), 1
                                            )
                                    )
                            )

                            .build()
            )
            .testenvId("a")
            .build();

    private final CheckIterationEntity iterationTwo = CheckIterationEntity.builder()
            .id(CheckIterationEntity.Id.of(checkOne.getId(), CheckIteration.IterationType.FULL, 1))
            .status(Common.CheckStatus.RUNNING)
            .statistics(IterationStatistics.EMPTY)
            .info(IterationInfo.EMPTY)
            .testenvId("a")
            .build();

    private final CheckIterationEntity iterationThree = CheckIterationEntity.builder()
            .id(CheckIterationEntity.Id.of(checkTwo.getId(), CheckIteration.IterationType.FAST, 1))
            .status(Common.CheckStatus.RUNNING)
            .statistics(IterationStatistics.EMPTY)
            .info(IterationInfo.EMPTY)
            .testenvId("b")
            .build();

    private final CheckIterationEntity iterationFour = CheckIterationEntity.builder()
            .id(CheckIterationEntity.Id.of(checkTwo.getId(), CheckIteration.IterationType.FAST, 1))
            .status(Common.CheckStatus.RUNNING)
            .statistics(IterationStatistics.EMPTY)
            .info(IterationInfo.builder()
                    .fastTargets(Set.of("fast"))
                    .disabledToolchains(Set.of())
                    .notRecheckReason("")
                    .pessimized(false)
                    .pessimizationInfo("")
                    .advisedPool("")
                    .testenvCheckId("")
                    .progress(0)
                    .strongModePolicy(StrongModePolicy.BY_A_YAML)
                    .autodetectedFastCircuit(true)
                    .build())
            .testenvId("c")
            .build();

    private final CheckIterationEntity iterationSuspiciousOne = CheckIterationEntity.builder()
            .id(CheckIterationEntity.Id.of(checkSuspiciousThree.getId(), CheckIteration.IterationType.FAST, 1))
            .status(Common.CheckStatus.COMPLETED_SUCCESS)
            .statistics(IterationStatistics.EMPTY)
            .suspiciousAlerts(Set.of(new SuspiciousAlert("test", "10 tests deleted, only 1 added")))
            .build();


    @Mock
    private LargeTestsSearch largeTestsSearch;

    @Mock
    private LargeStartService largeStartService;

    @Mock
    private ArcService arcService;

    @BeforeEach
    public void setup() {
        var requirementsService = mock(RequirementsService.class);
        var storageEventsProducer = mock(StorageEventsProducer.class);

        var attributesCollector = new CheckAttributesCollector(new FlowUrls("https://a.yandex-team.ru"));
        var storageApiController = new StorageFrontApiController(
                new ApiCheckService(
                        new OverridableClock(),
                        requirementsService, db, arcService, "test", apiCache,
                        storageEventsProducer, 1, mock(BazingaTaskManager.class),
                        ShardingSettings.DEFAULT
                ),
                new SearchService(db, 20, 20),
                largeTestsSearch,
                largeStartService,
                apiCache,
                new TestRunCommandProvider(db),
                attributesCollector,
                arcService
        );

        stub = StorageFrontApiServiceGrpc.newBlockingStub(buildChannel(storageApiController));
    }

    @Test
    public void getChecks() {
        when(arcService.getCommit(eq(ArcRevision.parse("7c2f8d6acb6159d1bfac277b8e566f45f4cbde46")))).thenReturn(
                ArcCommit.builder()
                        .id(ArcCommit.Id.of("7c2f8d6acb6159d1bfac277b8e566f45f4cbde46"))
                        .build()
        );

        this.db.currentOrTx(() -> {
                    this.db.checks().save(checkOne);
                    this.db.checks().save(checkTwo);
                    this.db.checkIterations().save(iterationOne);
                    this.db.checkIterations().save(iterationTwo);
                    this.db.checkIterations().save(iterationThree);
                }
        );

        var request = StorageFrontApi.GetChecksRequest.newBuilder()
                .addAllIds(
                        List.of(checkOne.getId().getId().toString(), checkTwo.getId().getId().toString(), "missing")
                )
                .build();
        var response = stub.getChecks(request);

        var checks = response.getChecksList().stream()
                .sorted(Comparator.comparing(StorageFrontApi.CheckViewModel::getId))
                .toList();

        assertThat(checks.size()).isEqualTo(2);
        assertThat(checks.get(0).getFastIterationsCount()).isEqualTo(1);
        assertThat(checks.get(1).getFastIterationsCount()).isEqualTo(1);

        var iteration = checks.get(0).getFastIterations(0);
        assertThat(iteration.getStatistics().getMetricsList())
                .containsExactlyInAnyOrder(
                        Common.Metric.newBuilder()
                                .setName("time_spent")
                                .setValue(47.2)
                                .setSize(Common.MetricSize.SECONDS)
                                .setAggregate(Common.MetricAggregateFunction.SUM)
                                .build(),
                        Common.Metric.newBuilder()
                                .setName("mem_max")
                                .setValue(1200000.0)
                                .setSize(Common.MetricSize.BYTES)
                                .setAggregate(Common.MetricAggregateFunction.MAX)
                                .build(),
                        Common.Metric.newBuilder()
                                .setName("slot_time")
                                .setValue(5.16 * 60 * 60)
                                .setSize(Common.MetricSize.SECONDS)
                                .setAggregate(Common.MetricAggregateFunction.SUM)
                                .build()
                );

        assertThat(iteration.getInfoPanel().getEntitiesList())
                .extracting(InfoPanelOuterClass.InfoEntity::getKey)
                .describedAs("don't pass generic metrics to frontend ")
                .doesNotContain("time_spent", "mem_max");

        assertThat(iteration.getInfoPanel().getEntitiesList())
                .contains(InfoPanelOuterClass.InfoEntity.newBuilder()
                        .setKey("Consumed CPU")
                        .setValue("5.16 hours")
                        .build()
                );

        var flowUrl = iteration.getInfoPanel().getEntitiesList().stream()
                .filter(x -> x.getKey().trim().equals("Flow"))
                .findFirst()
                .orElse(null);

        assertThat(flowUrl).isNotNull();
        assertThat(flowUrl.getLink()).isEqualTo(
                "https://a.yandex-team.ru/projects/autocheck/ci/actions/flow?dir=test-dir&id=test-id&number=1"
        );


        var postCommitChecks = stub.getPostCommitChecks(
                StorageFrontApi.GetPostCommitChecksRequest.newBuilder()
                        .setRevision("7c2f8d6acb6159d1bfac277b8e566f45f4cbde46")
                        .build()
        );

        assertThat(postCommitChecks.getChecksList()).hasSize(1);
    }

    @Test
    public void getChecksSuspicious() {
        this.db.currentOrTx(() -> {
                    this.db.checks().save(checkSuspiciousThree);
                    this.db.checkIterations().save(iterationSuspiciousOne);
                }
        );

        var request = StorageFrontApi.GetChecksRequest.newBuilder()
                .addAllIds(
                        List.of(checkSuspiciousThree.getId().getId().toString())
                )
                .build();
        var response = stub.getChecks(request);

        var checks = response.getChecksList().stream()
                .sorted(Comparator.comparing(StorageFrontApi.CheckViewModel::getId))
                .toList();

        assertThat(checks.size()).isEqualTo(1);
        assertThat(checks.get(0).getFastIterationsCount()).isEqualTo(1);
        assertThat(checks.get(0).getSuspiciousAlertsCount()).isEqualTo(1);

        var iteration = checks.get(0).getFastIterationsList().get(0);
        assertThat(iteration.getSuspiciousAlertsCount()).isEqualTo(1);
    }

    @Test
    public void shouldReturnAutoFastTargetFlag() {
        this.db.currentOrTx(() -> {
            db.checks().save(checkTwo);
            db.checkIterations().save(iterationThree);
            db.checkIterations().save(iterationFour);
        });
        var request = StorageFrontApi.GetChecksRequest.newBuilder()
                .addIds("c")
                .build();
        var response = stub.getChecks(request);
        var checks = response.getChecksList();
        assertThat(checks).hasSize(1);
        assertThat(checks.get(0).getFastIterations(0).getInfo().getAutodetectedFastCircuit()).isTrue();
    }

    @Test
    @Disabled // todo temporary
    public void getChecksByTestenvIds() {
        this.db.currentOrTx(() -> {
                    this.db.checks().save(checkOne);
                    this.db.checks().save(checkTwo);
                    this.db.checkIterations().save(iterationOne);
                    this.db.checkIterations().save(iterationTwo);
                    this.db.checkIterations().save(iterationThree);
                }
        );

        var request = StorageFrontApi.GetChecksRequest.newBuilder()
                .addAllIds(List.of("a", "b"))
                .build();
        var response = stub.getChecks(request);

        var checks = response.getChecksList().stream()
                .sorted(Comparator.comparing(StorageFrontApi.CheckViewModel::getId))
                .toList();

        assertThat(checks.size()).isEqualTo(2);
        assertThat(checks.get(0).getFastIterationsCount()).isEqualTo(1);
        assertThat(checks.get(1).getFastIterationsCount()).isEqualTo(1);
    }

    @Test
    public void searchesDiffs() {
        this.db.currentOrTx(() -> {
                    this.db.checks().save(checkOne);
                    this.db.checkIterations().save(iterationOne);

                    this.db.testDiffs().save(
                            new TestDiffEntity(
                                    TestDiffByHashEntity.builder()
                                            .id(
                                                    TestDiffByHashEntity.Id.of(
                                                            new ChunkAggregateEntity.Id(
                                                                    iterationOne.getId(),
                                                                    ChunkEntity.Id.of(Common.ChunkType.CT_SMALL_TEST, 1)
                                                            ),
                                                            new TestEntity.Id(1L, "a", 1L)
                                                    )
                                            )
                                            .resultType(Common.ResultType.RT_BUILD)
                                            .build()
                            )
                    );
                }
        );

        var diffs = stub.searchDiffs(
                StorageFrontApi.SearchDiffsRequest.newBuilder()
                        .setIterationId(CheckProtoMappers.toProtoIterationId(iterationOne.getId()))
                        .setSearch(
                                StorageFrontApi.DiffsSearch.newBuilder()
                                        .build()
                        )
                        .build()
        );

        assertThat(diffs.getDiffsCount()).isEqualTo(1);

    }

    @Test
    void listLargeTestsToolchains() {
        // No logic in API
        var request = ListLargeTestsToolchainsRequest.newBuilder()
                .setCheckId("my-check")
                .build();

        var response = ListLargeTestsToolchainsResponse.newBuilder()
                .addToolchains(StorageFrontApi.LargeTestToolchain.newBuilder().setToolchain("chain-1"))
                .build();

        when(largeTestsSearch.listToolchains(Mockito.eq(request)))
                .thenReturn(response);

        var ret = stub.listLargeTestsToolchains(request);
        assertThat(ret).isEqualTo(response);

        Mockito.verifyNoMoreInteractions(largeTestsSearch);
    }

    @Test
    void searchLargeTests() {
        // No logic in API
        var request = SearchLargeTestsRequest.newBuilder()
                .setCheckId("my-check")
                .build();

        var response = SearchLargeTestsResponse.newBuilder()
                .addTests(StorageFrontApi.LargeTestResponse.newBuilder()
                        .setLeftStatus(StorageFrontApi.LargeTestStatus.LTS_DISCOVERED))
                .build();

        when(largeTestsSearch.searchLargeTests(Mockito.eq(request)))
                .thenReturn(response);

        var ret = stub.searchLargeTests(request);
        assertThat(ret).isEqualTo(response);

        Mockito.verifyNoMoreInteractions(largeTestsSearch);
        Mockito.verifyNoMoreInteractions(largeStartService);
    }

    @Test
    void startLargeTests() {
        // No logic in API
        var request = StartLargeTestsRequest.newBuilder()
                .addTestDiffs(StorageFrontApi.DiffId.newBuilder()
                        .setIterationId(CheckIteration.IterationId.newBuilder()
                                .setCheckId("123")
                                .build())
                        .setTestId(Common.TestId.newBuilder()
                                .setTestId("1")
                                .setSuiteId("1")
                                .setToolchain("chain-1"))
                        .setPath("a/b/c"))
                .build();

        var ret = stub.startLargeTests(request);
        assertThat(ret).isEqualTo(StorageFrontApi.StartLargeTestsResponse.getDefaultInstance());

        var tests = List.of(CheckProtoMappers.toDiffId(request.getTestDiffs(0)));
        Mockito.verify(largeStartService).startLargeTestsManual(Mockito.eq(tests), eq("user42"));

        Mockito.verifyNoMoreInteractions(largeTestsSearch);
        Mockito.verifyNoMoreInteractions(largeStartService);
    }

    @Test
    void scheduleLargeTests() {
        // No logic in API

        var request = StorageFrontApi.ScheduleLargeTestsRequest.newBuilder()
                .setCheckId("123")
                .setRunLargeTestsAfterDiscovery(true)
                .build();
        var ret = stub.scheduleLargeTests(request);
        assertThat(ret).isEqualTo(StorageFrontApi.ScheduleLargeTestsResponse.getDefaultInstance());

        Mockito.verify(largeStartService)
                .scheduleLargeTestsManual(Mockito.eq(CheckEntity.Id.of(123L)), Mockito.eq(true), eq("user42"));

        Mockito.verifyNoMoreInteractions(largeTestsSearch);
        Mockito.verifyNoMoreInteractions(largeStartService);

    }
}
