package ru.yandex.ci.storage.api.controllers;

import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.primitives.UnsignedLong;
import com.google.common.primitives.UnsignedLongs;
import io.grpc.stub.StreamObserver;
import lombok.AllArgsConstructor;

import yandex.cloud.util.Strings;

import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.storage.api.StorageFrontTestsApi;
import ru.yandex.ci.storage.api.StorageFrontTestsApiServiceGrpc;
import ru.yandex.ci.storage.api.tests.TestsService;
import ru.yandex.ci.storage.core.db.model.test_status.TestSearch;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;

@SuppressWarnings("UnstableApiUsage")
@AllArgsConstructor
public class StorageFrontTestsApiController
        extends StorageFrontTestsApiServiceGrpc.StorageFrontTestsApiServiceImplBase {

    private final TestsService testsService;

    @Override
    public void searchTests(
            StorageFrontTestsApi.SearchTestsRequest request,
            StreamObserver<StorageFrontTestsApi.SearchTestsResponse> responseObserver
    ) {
        if (request.getBranch().isEmpty()) {
            throw GrpcUtils.invalidArgumentException("Empty id");
        }

        var result = testsService.search(
                TestSearch.builder()
                        .statuses(new HashSet<>(request.getStatusesList()))
                        .branch(request.getBranch())
                        .name(request.getName())
                        .notificationFilter(request.getNotificationFilter())
                        .paging(TestSearch.Paging.builder()
                                .page(
                                        toPage(
                                                request.getNext().getPath().isEmpty() ?
                                                        request.getPrevious() :
                                                        request.getNext()
                                        )
                                )
                                .pageSize(Math.min(100, request.getPageSize() <= 0 ? 20 : request.getPageSize()))
                                .ascending(request.getPrevious().getPath().isEmpty())
                                .build()
                        )
                        .path(request.getPath())
                        .resultTypes(new HashSet<>(request.getResultTypesList()))
                        .project(request.getProject())
                        .subtestName(request.getSubtestName())
                        .build()
        );

        responseObserver.onNext(
                StorageFrontTestsApi.SearchTestsResponse.newBuilder()
                        .addAllTests(
                                result.getResults().entrySet().stream().map(this::convert).collect(Collectors.toList())
                        )
                        .setNext(toProtoPage(result.getNextPage()))
                        .setPrevious(toProtoPage(result.getPreviousPage()))
                        .build()
        );
        responseObserver.onCompleted();
    }

    private StorageFrontTestsApi.SearchTestsPage toProtoPage(@Nullable TestSearch.Page value) {
        if (value == null) {
            return StorageFrontTestsApi.SearchTestsPage.getDefaultInstance();
        }

        return StorageFrontTestsApi.SearchTestsPage.newBuilder()
                .setPath(value.getPath())
                .setTestId(UnsignedLongs.toString(value.getTestId()))
                .build();
    }

    private TestSearch.Page toPage(StorageFrontTestsApi.SearchTestsPage value) {
        return new TestSearch.Page(
                value.getPath(),
                Strings.isBlank(value.getTestId()) ? 0 : UnsignedLong.valueOf(value.getTestId()).longValue()
        );
    }

    private StorageFrontTestsApi.TestViewModel convert(Map.Entry<TestStatusEntity.Id, List<TestStatusEntity>> value) {
        return StorageFrontTestsApi.TestViewModel.newBuilder()
                .setId(CheckProtoMappers.toProtoTestStatusId(value.getKey()))
                .addAllToolchains(
                        value.getValue().stream().map(this::convertToolchain).collect(Collectors.toList())
                )
                .build();
    }

    private StorageFrontTestsApi.TestToolchainViewModel convertToolchain(TestStatusEntity value) {
        return StorageFrontTestsApi.TestToolchainViewModel.newBuilder()
                .setName(value.getName())
                .setToolchain(value.getId().getToolchain())
                .setPath(value.getPath())
                .setResultType(value.getType())
                .setRevisionNumber(value.getRevisionNumber())
                .setService(value.getService())
                .setSubtestName(value.getSubtestName())
                .addAllTags(value.getTags())
                .setTestStatus(value.getStatus())
                .setUid(value.getUid())
                .build();
    }
}
