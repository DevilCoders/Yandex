package ru.yandex.ci.engine.event;

import java.util.concurrent.BlockingQueue;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.LinkedBlockingQueue;

import lombok.Value;

import ru.yandex.ci.core.logbroker.LogbrokerWriter;
import ru.yandex.kikimr.persqueue.producer.transport.message.inbound.ProducerWriteResponse;

public class NoOpAsyncProducerWithStore implements LogbrokerWriter {

    private final BlockingQueue<Content> queue = new LinkedBlockingQueue<>();

    @Override
    public String getSourceId() {
        return "some-source-id";
    }

    @Override
    public CompletableFuture<ProducerWriteResponse> write(byte[] data) {
        queue.add(new Content(data, 0, 0));
        return CompletableFuture.completedFuture(new ProducerWriteResponse(0, 0, true));
    }

    public BlockingQueue<Content> getQueue() {
        return queue;
    }

    @Value
    public static class Content {
        byte[] data;
        long seqNo;
        long timestamp;
    }
}
