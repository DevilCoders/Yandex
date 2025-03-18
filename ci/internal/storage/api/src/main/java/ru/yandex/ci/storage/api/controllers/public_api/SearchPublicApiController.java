package ru.yandex.ci.storage.api.controllers.public_api;

import io.grpc.stub.StreamObserver;
import lombok.RequiredArgsConstructor;

import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.storage.api.SearchPublicApiGrpc;
import ru.yandex.ci.storage.api.StoragePublicApi;
import ru.yandex.ci.storage.api.check.ApiCheckService;
import ru.yandex.ci.storage.core.proto.PublicProtoMappers;

@RequiredArgsConstructor
public class SearchPublicApiController extends SearchPublicApiGrpc.SearchPublicApiImplBase {
    private final ApiCheckService checkService;

    @Override
    public void findLastCheckByPullRequest(
            StoragePublicApi.FindLastCheckByPullRequestRequest request,
            StreamObserver<StoragePublicApi.FindLastCheckByPullRequestRequest.Response> responseObserver
    ) {
        var check = checkService.findLastCheckByPullRequest(request.getPullRequestId());
        if (check.isEmpty()) {
            throw GrpcUtils.notFoundException("No check found for " + request.getPullRequestId());
        }

        responseObserver.onNext(
                StoragePublicApi.FindLastCheckByPullRequestRequest.Response.newBuilder()
                        .setCheck(PublicProtoMappers.toProtoCheck(check.get()))
                        .build()
        );

        responseObserver.onCompleted();
    }
}
