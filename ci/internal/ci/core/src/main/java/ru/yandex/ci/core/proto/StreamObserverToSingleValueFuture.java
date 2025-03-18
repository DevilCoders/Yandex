package ru.yandex.ci.core.proto;

import java.util.concurrent.CompletableFuture;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import io.grpc.Status;
import io.grpc.stub.StreamObserver;

import ru.yandex.lang.NonNullApi;

@NonNullApi
public class StreamObserverToSingleValueFuture<T> implements StreamObserver<T> {

    @Nonnull
    private final CompletableFuture<T> resultFuture;
    @Nullable
    private T value;

    public StreamObserverToSingleValueFuture(CompletableFuture<T> resultFuture) {
        this.resultFuture = resultFuture;
    }

    @Override
    public void onNext(T value) {
        if (this.value != null) {
            throw Status.INTERNAL.withDescription("More than one value received for unary call")
                    .asRuntimeException();
        }
        this.value = value;
    }


    @Override
    public void onError(Throwable t) {
        resultFuture.completeExceptionally(t);
    }

    @Override
    public void onCompleted() {
        if (this.value != null) {
            resultFuture.complete(this.value);
        } else {
            resultFuture.completeExceptionally(
                    Status.INTERNAL.withDescription("No value received for unary call")
                            .asRuntimeException()
            );
        }
    }

}
