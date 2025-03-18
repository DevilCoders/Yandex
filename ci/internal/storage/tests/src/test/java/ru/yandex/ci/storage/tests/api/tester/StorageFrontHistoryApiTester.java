package ru.yandex.ci.storage.tests.api.tester;

import java.util.Collection;

import ru.yandex.ci.common.grpc.GrpcTestUtils;
import ru.yandex.ci.storage.api.StorageFrontHistoryApi;
import ru.yandex.ci.storage.api.StorageFrontHistoryApiServiceGrpc;
import ru.yandex.ci.storage.api.controllers.StorageFrontHistoryApiController;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.constant.Trunk;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;

public class StorageFrontHistoryApiTester {
    private final StorageFrontHistoryApiServiceGrpc.StorageFrontHistoryApiServiceBlockingStub apiController;

    public StorageFrontHistoryApiTester(StorageFrontHistoryApiController api) {
        this.apiController = StorageFrontHistoryApiServiceGrpc.newBlockingStub(GrpcTestUtils.buildChannel(api));
    }

    public StorageFrontHistoryApi.GetTestInfoResponse getTestInfo(TestEntity.Id testId) {
        return apiController.getTestInfo(
                StorageFrontHistoryApi.GetTestInfoRequest.newBuilder()
                        .setTestId(CheckProtoMappers.toProtoTestStatusId(Trunk.name(), testId))
                        .build()
        );
    }

    public StorageFrontHistoryApi.GetTestHistoryResponse getTestHistory(TestEntity.Id testId) {
        return getTestHistory(testId, StorageFrontHistoryApi.HistoryPager.newBuilder().build());
    }

    public StorageFrontHistoryApi.GetTestHistoryResponse getTestHistory(
            TestEntity.Id testId, StorageFrontHistoryApi.HistoryPager page
    ) {
        return apiController.getTestHistory(
                StorageFrontHistoryApi.GetTestHistoryRequest.newBuilder()
                        .setTestId(CheckProtoMappers.toProtoTestStatusId(Trunk.name(), testId))
                        .setFilters(StorageFrontHistoryApi.HistoryFilters.newBuilder().build())
                        .setPage(page)
                        .build()
        );
    }

    public StorageFrontHistoryApi.CountRevisionsResponse countRevisions(
            TestEntity.Id testId, Collection<StorageFrontHistoryApi.WrappedRevisionsBoundaries> boundaries
    ) {
        return apiController.countRevisions(
                StorageFrontHistoryApi.CountRevisionsRequest.newBuilder()
                        .setTestId(CheckProtoMappers.toProtoTestStatusId(Trunk.name(), testId))
                        .addAllBoundaries(boundaries)
                        .build()
        );
    }

    public StorageFrontHistoryApi.GetLaunchesResponse getLaunches(TestEntity.Id testId, long revision) {
        return apiController.getLaunches(
                StorageFrontHistoryApi.GetLaunchesRequest.newBuilder()
                        .setTestId(CheckProtoMappers.toProtoTestStatusId(Trunk.name(), testId))
                        .setRevision(revision)
                        .build()
        );
    }

    public StorageFrontHistoryApi.GetTestMetricsResponse getMetrics(TestEntity.Id testId) {
        return apiController.getTestMetrics(
                StorageFrontHistoryApi.GetTestMetricsRequest.newBuilder()
                        .setTestId(CheckProtoMappers.toProtoTestStatusId(Trunk.name(), testId))
                        .build()
        );
    }

    public StorageFrontHistoryApi.GetTestMetricHistoryResponse getTestMetricHistory(
            TestEntity.Id testId, String metricName
    ) {
        return apiController.getTestMetricHistory(
                StorageFrontHistoryApi.GetTestMetricHistoryRequest.newBuilder()
                        .setTestId(CheckProtoMappers.toProtoTestStatusId(Trunk.name(), testId))
                        .setMetricName(metricName)
                        .build()
        );
    }

    public StorageFrontHistoryApi.GetWrappedRevisionsResponse getWrapped(
            TestEntity.Id testId, StorageFrontHistoryApi.WrappedRevisionsBoundaries boundaries
    ) {
        return apiController.getWrappedRevisions(
                StorageFrontHistoryApi.GetWrappedRevisionsRequest.newBuilder()
                        .setTestId(CheckProtoMappers.toProtoTestStatusId(Trunk.name(), testId))
                        .setBoundaries(boundaries)
                        .build()
        );
    }

    public Common.TestStatusId getTestIdByOldId(String oldId) {
        return apiController.getIdByOldId(
                StorageFrontHistoryApi.GetIdByOldIdRequest.newBuilder()
                        .setId(oldId)
                        .build()
        ).getTestId();
    }
}
