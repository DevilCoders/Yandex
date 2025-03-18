package ru.yandex.ci.storage.api.controllers.public_api;

import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import io.grpc.stub.StreamObserver;
import lombok.RequiredArgsConstructor;

import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.storage.api.CommonPublicApiGrpc;
import ru.yandex.ci.storage.api.StoragePublicApi;
import ru.yandex.ci.storage.api.check.ApiCheckService;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.proto.PublicProtoMappers;

@RequiredArgsConstructor
public class CommonPublicApiController extends CommonPublicApiGrpc.CommonPublicApiImplBase {
    private final ApiCheckService checkService;

    @Override
    public void getCheckCircuits(
            StoragePublicApi.GetCheckCircuitsRequest request,
            StreamObserver<StoragePublicApi.GetCheckCircuitsRequest.Response> responseObserver
    ) {
        var iterations = checkService.getIterationsByCheckIds(Set.of(CheckEntity.Id.of(request.getCheckId())));
        if (iterations.isEmpty()) {
            throw GrpcUtils.notFoundException("No iterations found for " + request.getCheckId());
        }

        var circuits = iterations.stream()
                .collect(Collectors.groupingBy(x -> x.getId().getIterationType()))
                .entrySet().stream().map(this::toCircuit).toList();

        responseObserver.onNext(
                StoragePublicApi.GetCheckCircuitsRequest.Response.newBuilder()
                        .addAllCircuits(circuits)
                        .build()
        );

        responseObserver.onCompleted();
    }

    @Nonnull
    private StoragePublicApi.GetCheckCircuitsRequest.Response.Circuit toCircuit(
            Map.Entry<CheckIteration.IterationType, List<CheckIterationEntity>> entry
    ) {
        var it = entry.getValue().stream().map(PublicProtoMappers::toIteration).toList();
        return StoragePublicApi.GetCheckCircuitsRequest.Response.Circuit.newBuilder()
                .setCircuitType(entry.getKey())
                .setAggregateIteration(it.get(0))
                .addAllIterations(it)
                .build();
    }
}
