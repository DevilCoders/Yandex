package ru.yandex.ci.core.logbroker;

import java.util.concurrent.CompletableFuture;

import ru.yandex.kikimr.persqueue.producer.transport.message.inbound.ProducerWriteResponse;

public interface LogbrokerWriter {
    String getSourceId();

    CompletableFuture<ProducerWriteResponse> write(byte[] data);

}
