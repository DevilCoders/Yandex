package ru.yandex.ci.common.grpc;

import java.util.concurrent.CompletableFuture;

import javax.annotation.Nonnull;

import com.google.common.util.concurrent.FutureCallback;
import com.google.common.util.concurrent.Futures;
import com.google.common.util.concurrent.ListenableFuture;
import com.google.common.util.concurrent.MoreExecutors;

public class ListenableFutures {

    private ListenableFutures() {
        //
    }

    public static <T> CompletableFuture<T> toCompletableFuture(ListenableFuture<T> future) {
        var futureResponse = new CompletableFuture<T>();
        Futures.addCallback(
                future,
                new FutureCallback<>() {
                    @Override
                    public void onSuccess(T response) {
                        futureResponse.complete(response);
                    }

                    @Override
                    public void onFailure(@Nonnull Throwable t) {
                        futureResponse.completeExceptionally(t);
                    }
                },
                MoreExecutors.directExecutor()
        );
        return futureResponse;
    }
}
