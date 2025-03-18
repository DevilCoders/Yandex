package ru.yandex.ci.client.ayamler;

import java.util.Set;
import java.util.concurrent.CompletableFuture;
import java.util.function.Supplier;
import java.util.stream.Collectors;

import ru.yandex.ci.ayamler.AYamlerServiceGrpc;
import ru.yandex.ci.ayamler.AYamlerServiceGrpc.AYamlerServiceFutureStub;
import ru.yandex.ci.ayamler.Ayamler;
import ru.yandex.ci.ayamler.Ayamler.GetStrongModeBatchRequest;
import ru.yandex.ci.common.grpc.GrpcClient;
import ru.yandex.ci.common.grpc.GrpcClientImpl;
import ru.yandex.ci.common.grpc.GrpcClientProperties;
import ru.yandex.ci.common.grpc.ListenableFutures;

public class AYamlerClientImpl implements AYamlerClient, AutoCloseable {

    private final GrpcClient grpcClient;
    private final Supplier<AYamlerServiceFutureStub> stub;

    private AYamlerClientImpl(GrpcClientProperties properties) {
        this.grpcClient = GrpcClientImpl.builder(properties, getClass()).build();
        this.stub = grpcClient.buildStub(AYamlerServiceGrpc::newFutureStub);
    }

    public static AYamlerClientImpl create(GrpcClientProperties properties) {
        return new AYamlerClientImpl(properties);
    }

    @Override
    public CompletableFuture<Ayamler.GetStrongModeBatchResponse> getStrongMode(
            Set<StrongModeRequest> strongModeRequests
    ) {
        var batchRequest = GetStrongModeBatchRequest.newBuilder()
                .addAllRequest(
                        strongModeRequests.stream()
                                .map(AYamlerClientImpl::convert)
                                .collect(Collectors.toList()))
                .build();

        var futureResponse = stub.get().getStrongModeBatch(batchRequest);
        return ListenableFutures.toCompletableFuture(futureResponse);
    }

    @Override
    public void close() throws Exception {
        grpcClient.close();
    }

    private static Ayamler.GetStrongModeRequest convert(StrongModeRequest request) {
        return Ayamler.GetStrongModeRequest.newBuilder()
                .setPath(request.getPath())
                .setRevision(request.getRevision())
                .setLogin(request.getLogin())
                .build();
    }
}
