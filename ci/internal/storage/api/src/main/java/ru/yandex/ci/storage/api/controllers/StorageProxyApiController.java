package ru.yandex.ci.storage.api.controllers;

import java.util.Map;

import javax.annotation.Nonnull;

import io.grpc.Context;
import io.grpc.stub.StreamObserver;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.api.StorageProxyApi;
import ru.yandex.ci.storage.api.StorageProxyApiServiceGrpc;
import ru.yandex.ci.storage.api.proxy.StorageMessageProxyWriter;
import ru.yandex.ci.storage.core.CheckOuterClass;

@Slf4j
@RequiredArgsConstructor
public class StorageProxyApiController extends StorageProxyApiServiceGrpc.StorageProxyApiServiceImplBase {

    @Nonnull
    private final Map<CheckOuterClass.CheckType, StorageMessageProxyWriter> writers;

    @Override
    public void writeMessages(
            StorageProxyApi.WriteMessagesRequest request,
            StreamObserver<StorageProxyApi.WriteMessagesResponse> responseObserver
    ) {
        Context.current().fork().run(() -> writeMessagesInternal(request, responseObserver));
    }

    void writeMessages(StorageProxyApi.WriteMessagesRequest request) {
        if (request.getMessagesCount() > 0) {
            log.info("Processing {} message(s), check type = {}",
                    request.getMessagesCount(), request.getCheckType());
            this.writeMessagesImpl(request);
        } else {
            log.warn("No messages to process, empty request for check type = {}?",
                    request.getCheckType());
        }
    }

    private void writeMessagesInternal(
            StorageProxyApi.WriteMessagesRequest request,
            StreamObserver<StorageProxyApi.WriteMessagesResponse> responseObserver
    ) {
        writeMessages(request);
        responseObserver.onNext(StorageProxyApi.WriteMessagesResponse.getDefaultInstance());
        responseObserver.onCompleted();
    }

    private void writeMessagesImpl(StorageProxyApi.WriteMessagesRequest request) {
        var checkType = request.getCheckType();
        var writer = writers.get(checkType);
        if (writer == null) {
            throw new IllegalArgumentException("Check type " + checkType + " is not supported, accept only " +
                    writers.keySet());
        }
        writer.writeTasks(request.getMessagesList());
    }

}
