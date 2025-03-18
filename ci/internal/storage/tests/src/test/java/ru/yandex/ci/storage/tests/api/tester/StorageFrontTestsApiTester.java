package ru.yandex.ci.storage.tests.api.tester;

import ru.yandex.ci.common.grpc.GrpcTestUtils;
import ru.yandex.ci.storage.api.StorageFrontTestsApi;
import ru.yandex.ci.storage.api.StorageFrontTestsApiServiceGrpc;
import ru.yandex.ci.storage.api.controllers.StorageFrontTestsApiController;

public class StorageFrontTestsApiTester {
    private final StorageFrontTestsApiServiceGrpc.StorageFrontTestsApiServiceBlockingStub apiController;

    public StorageFrontTestsApiTester(StorageFrontTestsApiController api) {
        this.apiController = StorageFrontTestsApiServiceGrpc.newBlockingStub(GrpcTestUtils.buildChannel(api));
    }

    public StorageFrontTestsApi.SearchTestsResponse getTestInfo(StorageFrontTestsApi.SearchTestsRequest request) {
        return apiController.searchTests(request);
    }
}
