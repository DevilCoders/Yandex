package ru.yandex.ci.storage.tests.api.tester;

import java.util.Collection;
import java.util.List;

import ru.yandex.ci.common.grpc.GrpcTestUtils;
import ru.yandex.ci.storage.api.StorageFrontApi;
import ru.yandex.ci.storage.api.StorageFrontApiServiceGrpc;
import ru.yandex.ci.storage.api.controllers.StorageFrontApiController;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;


public class StorageFrontApiTester {
    private final StorageFrontApiServiceGrpc.StorageFrontApiServiceBlockingStub frontApiController;

    public StorageFrontApiTester(StorageFrontApiController api) {
        this.frontApiController = StorageFrontApiServiceGrpc.newBlockingStub(GrpcTestUtils.buildChannel(api));
    }

    public StorageFrontApi.IterationViewModel getIteration(CheckIteration.IterationId iterationId) {
        return frontApiController.getIteration(
                StorageFrontApi.GetIterationRequest.newBuilder()
                        .setId(iterationId)
                        .build()
        ).getIteration();
    }

    public String getRunCommand(StorageFrontApi.DiffId id) {
        return frontApiController.getRunCommand(
                StorageFrontApi.GetRunCommandRequest.newBuilder()
                        .setId(id)
                        .build()
        ).getCommand();
    }

    public String getRunCommand(Common.TestStatusId id) {
        return frontApiController.getRunCommand(
                StorageFrontApi.GetRunCommandRequest.newBuilder()
                        .setStatusId(id)
                        .build()
        ).getCommand();
    }

    public List<StorageFrontApi.DiffViewModel> listSuite(
            StorageFrontApi.SuiteSearch search, StorageFrontApi.DiffId id
    ) {
        return frontApiController.listSuite(
                StorageFrontApi.ListSuiteRequest.newBuilder()
                        .setSearch(search)
                        .setDiffId(id)
                        .build()
        ).getChildrenList();
    }


    public StorageFrontApi.SearchLargeTestsResponse listLargeTestsToolchains(
            CheckEntity.Id checkId
    ) {
        return frontApiController.searchLargeTests(
                StorageFrontApi.SearchLargeTestsRequest.newBuilder()
                        .setCheckId(checkId.getId().toString())
                        .build()
        );
    }

    public StorageFrontApi.StartLargeTestsResponse startLargeTests(
            Collection<StorageFrontApi.DiffId> diffIds
    ) {
        return frontApiController.startLargeTests(
                StorageFrontApi.StartLargeTestsRequest.newBuilder()
                        .addAllTestDiffs(diffIds)
                        .build()
        );
    }

    public List<String> getSuggest(
            CheckIterationEntity.Id iterationId, Common.CheckSearchEntityType entityType, String text
    ) {
        return frontApiController.getSuggest(
                StorageFrontApi.GetSuggestRequest.newBuilder()
                        .setIterationId(CheckProtoMappers.toProtoIterationId(iterationId))
                        .setEntityType(entityType)
                        .setValue(text)
                        .build()
        ).getResultsList();
    }


    public StorageFrontApi.SearchSuitesResponse searchSuites(
            CheckIterationEntity.Id iterationId, StorageFrontApi.SuiteSearch search
    ) {
        return frontApiController.searchSuites(
                StorageFrontApi.SearchSuitesRequest.newBuilder()
                        .setId(CheckProtoMappers.toProtoIterationId(iterationId))
                        .setSearch(search)
                        .build()
        );
    }

    public StorageFrontApi.FrontCancelCheckResponse cancelCheck(CheckEntity.Id id) {
        return frontApiController.cancelCheck(
                StorageFrontApi.FrontCancelCheckRequest.newBuilder()
                        .setId(id.getId().toString())
                        .build()
        );
    }
}
