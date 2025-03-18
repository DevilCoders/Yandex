package ru.yandex.ci.core.logbroker;

import java.util.concurrent.CompletableFuture;

import lombok.RequiredArgsConstructor;

import ru.yandex.kikimr.persqueue.producer.transport.message.inbound.ProducerWriteResponse;

@RequiredArgsConstructor
public class LogbrokerWriterEmptyImpl implements LogbrokerWriter {

    @Override
    public String getSourceId() {
        return "some-soruce-id";
    }

    @Override
    public CompletableFuture<ProducerWriteResponse> write(byte[] data) {
        return CompletableFuture.completedFuture(new ProducerWriteResponse(0, 0, true));
    }

}
