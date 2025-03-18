package ru.yandex.ci.storage.api.controllers;

import java.util.Comparator;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import io.grpc.stub.StreamObserver;
import lombok.AllArgsConstructor;

import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.storage.api.StorageFrontHistoryApi;
import ru.yandex.ci.storage.api.StorageFrontHistoryApiServiceGrpc;
import ru.yandex.ci.storage.api.tests.HistoryService;
import ru.yandex.ci.storage.api.tests.LaunchesByStatus;
import ru.yandex.ci.storage.api.tests.TestHistoryPage;
import ru.yandex.ci.storage.core.db.clickhouse.metrics.TestMetricEntity;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_revision.HistoryFilters;
import ru.yandex.ci.storage.core.db.model.test_revision.HistoryPaging;
import ru.yandex.ci.storage.core.db.model.test_revision.TestRevisionEntity;
import ru.yandex.ci.storage.core.db.model.test_revision.WrappedRevisionsBoundaries;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.core.test_metrics.TestMetricsService;

@AllArgsConstructor
public class StorageFrontHistoryApiController
        extends StorageFrontHistoryApiServiceGrpc.StorageFrontHistoryApiServiceImplBase {

    private final HistoryService historyService;
    private final TestMetricsService metricsService;

    @Override
    public void getIdByOldId(
            StorageFrontHistoryApi.GetIdByOldIdRequest request,
            StreamObserver<StorageFrontHistoryApi.GetIdByOldIdResponse> responseObserver
    ) {
        if (request.getId().isBlank()) {
            throw GrpcUtils.invalidArgumentException("id");
        }

        var result = historyService.findTestByOldId(request.getId());
        if (result.isEmpty()) {
            throw GrpcUtils.notFoundException(request.getId());
        }

        responseObserver.onNext(
                StorageFrontHistoryApi.GetIdByOldIdResponse.newBuilder()
                        .setTestId(CheckProtoMappers.toProtoTestStatusId(result.get()))
                        .build()
        );
        responseObserver.onCompleted();
    }


    @Override
    public void getTestInfo(
            StorageFrontHistoryApi.GetTestInfoRequest request,
            StreamObserver<StorageFrontHistoryApi.GetTestInfoResponse> responseObserver
    ) {
        var testId = CheckProtoMappers.toTestStatusId(request.getTestId());
        var testInfo = historyService.getTestInfo(testId);

        responseObserver.onNext(
                StorageFrontHistoryApi.GetTestInfoResponse.newBuilder()
                        .addAllToolchains(
                                testInfo.stream().map(CheckProtoMappers::toToolchainStatus).collect(Collectors.toList())
                        )
                        .build()
        );
        responseObserver.onCompleted();
    }

    @Override
    public void getTestHistory(
            StorageFrontHistoryApi.GetTestHistoryRequest request,
            StreamObserver<StorageFrontHistoryApi.GetTestHistoryResponse> responseObserver
    ) {
        var statusId = CheckProtoMappers.toTestStatusId(request.getTestId());
        var history = historyService.getTestHistory(
                statusId,
                convert(request.getFilters()),
                new HistoryPaging(request.getPage().getFrom(), request.getPage().getTo())
        );

        responseObserver.onNext(
                StorageFrontHistoryApi.GetTestHistoryResponse.newBuilder()
                        .setNext(convert(history.getPaging().getNext()))
                        .setPrevious(convert(history.getPaging().getPrevious()))
                        .addAllRevisions(history.getRevisions().stream().map(this::convert).toList())
                        .build()
        );

        responseObserver.onCompleted();
    }

    private HistoryFilters convert(StorageFrontHistoryApi.HistoryFilters filters) {
        return new HistoryFilters(
                filters.getStatus(), filters.getFromRevision(), filters.getToRevision()
        );
    }

    @Override
    public void getWrappedRevisions(
            StorageFrontHistoryApi.GetWrappedRevisionsRequest request,
            StreamObserver<StorageFrontHistoryApi.GetWrappedRevisionsResponse> responseObserver
    ) {
        var statusId = CheckProtoMappers.toTestStatusId(request.getTestId());
        var boundaries = new WrappedRevisionsBoundaries(
                request.getBoundaries().getFrom(), request.getBoundaries().getTo()
        );

        var result = this.historyService.getWrappedRevisions(
                statusId,
                convert(request.getFilters()),
                boundaries
        );

        responseObserver.onNext(
                StorageFrontHistoryApi.GetWrappedRevisionsResponse.newBuilder()
                        .addAllRevisions(result.getRevisions().stream().map(this::convert).toList())
                        .build()
        );

        responseObserver.onCompleted();

    }

    @Override
    public void countRevisions(
            StorageFrontHistoryApi.CountRevisionsRequest request,
            StreamObserver<StorageFrontHistoryApi.CountRevisionsResponse> responseObserver
    ) {
        var statusId = CheckProtoMappers.toTestStatusId(request.getTestId());

        var revisions = historyService.countRevisions(
                statusId,
                convert(request.getFilters()),
                request.getBoundariesList().stream()
                        .map(x -> new WrappedRevisionsBoundaries(x.getFrom(), x.getTo()))
                        .toList()
        );

        responseObserver.onNext(
                StorageFrontHistoryApi.CountRevisionsResponse.newBuilder()
                        .addAllBoundaries(
                                revisions.entrySet().stream()
                                        .map(e -> StorageFrontHistoryApi.WrappedRevisionsNumber.newBuilder()
                                                .setBoundary(convert(e.getKey()))
                                                .setNumberOfRevisions(e.getValue())
                                                .build())
                                        .collect(Collectors.toList())
                        )
                        .build()
        );

        responseObserver.onCompleted();
    }

    @Override
    public void getLaunches(
            StorageFrontHistoryApi.GetLaunchesRequest request,
            StreamObserver<StorageFrontHistoryApi.GetLaunchesResponse> responseObserver
    ) {
        var toolchain = request.getTestId().getToolchain();
        if (toolchain.isEmpty() || toolchain.equals(TestEntity.ALL_TOOLCHAINS)) {
            throw GrpcUtils.invalidArgumentException("Toolchain not selected");
        }
        var statusId = CheckProtoMappers.toTestStatusId(request.getTestId());

        var result = this.historyService.getLaunches(statusId, request.getRevision());

        responseObserver.onNext(
                StorageFrontHistoryApi.GetLaunchesResponse.newBuilder()
                        .addAllLaunches(result.stream().map(this::convert).toList())
                        .build()
        );

        responseObserver.onCompleted();
    }

    private StorageFrontHistoryApi.LaunchesByStatus convert(LaunchesByStatus value) {
        return StorageFrontHistoryApi.LaunchesByStatus.newBuilder()
                .setStatus(value.getStatus())
                .setNumberOfLaunches((int) value.getNumberOfLaunches())
                .setLastLaunch(CheckProtoMappers.toProtoTestRun(value.getLastRun()))
                .build();
    }

    private StorageFrontHistoryApi.TestRevision convert(TestHistoryPage.TestRevision value) {
        return StorageFrontHistoryApi.TestRevision.newBuilder()
                .setRevision(CheckProtoMappers.toProtoRevision(value.getRevision()))
                .setWrappedRevisionsBoundaries(convert(value.getWrappedRevisionsBoundaries()))
                .addAllToolchains(value.getToolchains().values().stream().map(this::convert).toList())
                .build();
    }

    private StorageFrontHistoryApi.TestRevisionToolchain convert(TestRevisionEntity value) {
        return StorageFrontHistoryApi.TestRevisionToolchain.newBuilder()
                .setToolchain(value.getId().getStatusId().getToolchain())
                .setPreviousStatus(value.getPreviousStatus())
                .setStatus(value.getStatus())
                .setUid(value.getUid())
                .build();
    }

    private StorageFrontHistoryApi.WrappedRevisionsBoundaries convert(
            @Nullable WrappedRevisionsBoundaries value
    ) {
        if (value == null) {
            return StorageFrontHistoryApi.WrappedRevisionsBoundaries.getDefaultInstance();
        }

        return StorageFrontHistoryApi.WrappedRevisionsBoundaries.newBuilder()
                .setFrom(value.getFrom())
                .setTo(value.getTo())
                .build();
    }

    private StorageFrontHistoryApi.HistoryPager convert(HistoryPaging value) {
        return StorageFrontHistoryApi.HistoryPager.newBuilder()
                .setFrom(value.getFromRevision())
                .setTo(value.getToRevision())
                .build();
    }

    @Override
    public void getTestMetrics(
            StorageFrontHistoryApi.GetTestMetricsRequest request,
            StreamObserver<StorageFrontHistoryApi.GetTestMetricsResponse> responseObserver
    ) {
        var statusId = CheckProtoMappers.toTestStatusId(request.getTestId());

        responseObserver.onNext(
                StorageFrontHistoryApi.GetTestMetricsResponse.newBuilder()
                        .addAllMetrics(metricsService.getTestMetrics(statusId).stream().map(this::convert).toList())
                        .build()
        );

        responseObserver.onCompleted();
    }

    private StorageFrontHistoryApi.TestMetrics convert(String name) {
        return StorageFrontHistoryApi.TestMetrics.newBuilder()
                .setName(name)
                .build();
    }

    @Override
    public void getTestMetricHistory(
            StorageFrontHistoryApi.GetTestMetricHistoryRequest request,
            StreamObserver<StorageFrontHistoryApi.GetTestMetricHistoryResponse> responseObserver
    ) {
        var statusId = CheckProtoMappers.toTestStatusId(request.getTestId());

        var history = metricsService.getMetricHistory(
                statusId, request.getMetricName(), request.getFilters()
        );

        responseObserver.onNext(
                StorageFrontHistoryApi.GetTestMetricHistoryResponse.newBuilder()
                        .addAllPoints(
                                history.stream()
                                        .sorted(Comparator.comparing(TestMetricEntity::getRevision))
                                        .map(this::toPoint)
                                        .toList()
                        )
                        .build()
        );

        responseObserver.onCompleted();
    }

    private StorageFrontHistoryApi.TestMetricHistoryPoint toPoint(TestMetricEntity entity) {
        return StorageFrontHistoryApi.TestMetricHistoryPoint.newBuilder()
                .setRevision(entity.getRevision())
                .setValue(entity.getValue())
                .build();
    }
}
