package ru.yandex.ci.core.proto;

import java.util.concurrent.CompletableFuture;
import java.util.function.BiConsumer;

import io.grpc.stub.StreamObserver;

public class StreamObserverUtils {

    private StreamObserverUtils() {
        //
    }

    public static <TRequest, TResponse> CompletableFuture<TResponse> invokeAndGetSingleValueFuture(
            BiConsumer<TRequest, StreamObserver<TResponse>> call, TRequest request
    ) {
        var futureResult = new CompletableFuture<TResponse>();
        var streamObserver = new StreamObserverToSingleValueFuture<>(futureResult);
        call.accept(request, streamObserver);
        return futureResult;
    }
}
