package ru.yandex.ci.storage.tests.api.tester;


import java.util.Iterator;

import ru.yandex.ci.common.grpc.GrpcTestUtils;
import ru.yandex.ci.storage.api.RawDataPublicApiGrpc;
import ru.yandex.ci.storage.api.StoragePublicApi;
import ru.yandex.ci.storage.api.controllers.public_api.RawDataPublicApiController;

public class RawDataPublicApiTester {
    private final RawDataPublicApiGrpc.RawDataPublicApiBlockingStub apiController;

    public RawDataPublicApiTester(RawDataPublicApiController api) {
        this.apiController = RawDataPublicApiGrpc.newBlockingStub(GrpcTestUtils.buildChannel(api));
    }

    public Iterator<StoragePublicApi.TestResultPublicViewModel> streamCheckIterationResults(
            StoragePublicApi.StreamCheckResultsRequest request
    ) {
        return apiController.streamCheckIterationResults(request);
    }
}
