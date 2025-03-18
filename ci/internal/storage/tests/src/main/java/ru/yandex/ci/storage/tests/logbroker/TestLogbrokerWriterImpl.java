package ru.yandex.ci.storage.tests.logbroker;

import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.concurrent.atomic.AtomicLong;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;

import ru.yandex.ci.core.logbroker.LogbrokerWriter;
import ru.yandex.kikimr.persqueue.producer.transport.message.inbound.ProducerWriteResponse;

@RequiredArgsConstructor
public class TestLogbrokerWriterImpl implements LogbrokerWriter {

    private static final ConcurrentMap<String, AtomicLong> TOPIC_TO_SEQ_NO = new ConcurrentHashMap<>();

    @Nonnull
    private final String topic;

    @Nonnull
    private final String sourceId;

    private final TestLogbrokerService logbrokerService;

    @Override
    public String getSourceId() {
        return sourceId;
    }

    @Override
    public CompletableFuture<ProducerWriteResponse> write(byte[] data) {
        var seqNo = TOPIC_TO_SEQ_NO.computeIfAbsent(topic, k -> new AtomicLong())
                .incrementAndGet();
        logbrokerService.write(topic, List.of(data));
        return CompletableFuture.completedFuture(
                new ProducerWriteResponse(seqNo, 0, true)
        );
    }

}
