package ru.yandex.ci.storage.tests.api.tester;

import ru.yandex.ci.common.grpc.GrpcTestUtils;
import ru.yandex.ci.storage.api.SearchPublicApiGrpc;
import ru.yandex.ci.storage.api.StoragePublicApi;
import ru.yandex.ci.storage.api.controllers.public_api.SearchPublicApiController;

public class SearchPublicApiTester {
    private final SearchPublicApiGrpc.SearchPublicApiBlockingStub apiController;

    public SearchPublicApiTester(SearchPublicApiController api) {
        this.apiController = SearchPublicApiGrpc.newBlockingStub(GrpcTestUtils.buildChannel(api));
    }

    public StoragePublicApi.FindLastCheckByPullRequestRequest.Response findLastCheckByPullRequest(
            StoragePublicApi.FindLastCheckByPullRequestRequest request
    ) {
        return apiController.findLastCheckByPullRequest(request);
    }
}
