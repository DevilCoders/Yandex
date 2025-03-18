package ru.yandex.ci.storage.tests.api.tester;

import ru.yandex.ci.common.grpc.GrpcTestUtils;
import ru.yandex.ci.storage.api.CommonPublicApiGrpc;
import ru.yandex.ci.storage.api.StoragePublicApi;
import ru.yandex.ci.storage.api.controllers.public_api.CommonPublicApiController;

public class CommonPublicApiTester {
    private final CommonPublicApiGrpc.CommonPublicApiBlockingStub apiController;

    public CommonPublicApiTester(CommonPublicApiController api) {
        this.apiController = CommonPublicApiGrpc.newBlockingStub(GrpcTestUtils.buildChannel(api));
    }

    public StoragePublicApi.GetCheckCircuitsRequest.Response getCheckCircuits(
            StoragePublicApi.GetCheckCircuitsRequest request
    ) {
        return apiController.getCheckCircuits(request);
    }
}
