package ru.yandex.ci.storage.core.logbroker;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.time.Instant;
import java.util.UUID;
import java.util.concurrent.CompletableFuture;

import com.google.protobuf.AbstractMessageLite;
import io.micrometer.core.instrument.MeterRegistry;
import lombok.Getter;

import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.core.logbroker.LogbrokerWriter;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.kikimr.persqueue.producer.transport.message.inbound.ProducerWriteResponse;

public abstract class StorageMessageWriter
        extends StorageWriterBase<AbstractMessageLite<?, ?>, StorageMessageWriter.Message> {
    public StorageMessageWriter(
            MeterRegistry meterRegistry,
            int numberOfPartitions,
            LogbrokerWriterFactory logbrokerWriterFactory
    ) {
        super(meterRegistry, numberOfPartitions, logbrokerWriterFactory);
    }

    protected Common.MessageMeta createMeta() {
        return Common.MessageMeta.newBuilder()
                .setId(UUID.randomUUID().toString())
                .setTimestamp(ProtoConverter.convert(Instant.now()))
                .build();
    }

    @Override
    protected CompletableFuture<ProducerWriteResponse> write(
            int partition, LogbrokerWriter writer, AbstractMessageLite<?, ?> message
    ) {
        var output = new ByteArrayOutputStream();

        try {
            message.writeTo(output);
        } catch (IOException e) {
            return CompletableFuture.failedFuture(e);
        }

        var outputArray = output.toByteArray();
        this.statistics.onWrite(partition, outputArray.length);

        try {
            return writer.write(outputArray);
        } catch (RuntimeException e) {
            return CompletableFuture.failedFuture(e);
        }
    }

    @Getter
    protected static class Message extends StorageWriterBase.Message<AbstractMessageLite<?, ?>> {
        private final Common.MessageMeta meta;
        private final String description;

        public Message(
                Common.MessageMeta meta,
                int logbrokerPartition,
                AbstractMessageLite<?, ?> message,
                String description
        ) {
            super(logbrokerPartition, message);

            this.meta = meta;
            this.description = description;
        }

        @Override
        public String toLogString() {
            return "id: %s, class: %s, partition: %d, description: %s".formatted(
                    getMeta().getId(),
                    getMessage().getClass().getSimpleName(),
                    getLogbrokerPartition(),
                    getDescription()
            );
        }
    }
}
