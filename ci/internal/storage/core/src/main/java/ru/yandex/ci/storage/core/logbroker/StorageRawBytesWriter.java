package ru.yandex.ci.storage.core.logbroker;

import java.util.concurrent.CompletableFuture;

import io.micrometer.core.instrument.MeterRegistry;

import ru.yandex.ci.core.logbroker.LogbrokerWriter;
import ru.yandex.kikimr.persqueue.producer.transport.message.inbound.ProducerWriteResponse;

public class StorageRawBytesWriter extends StorageWriterBase<byte[], StorageRawBytesWriter.Message> {
    public StorageRawBytesWriter(
            MeterRegistry meterRegistry,
            int numberOfPartitions,
            LogbrokerWriterFactory logbrokerWriterFactory
    ) {
        super(meterRegistry, numberOfPartitions, logbrokerWriterFactory);
    }

    @Override
    protected CompletableFuture<ProducerWriteResponse> write(
            int partition, LogbrokerWriter writer, byte[] messageData
    ) {
        this.statistics.onWrite(partition, messageData.length);

        try {
            return writer.write(messageData);
        } catch (RuntimeException e) {
            return CompletableFuture.failedFuture(e);
        }
    }

    public static class Message extends StorageWriterBase.Message<byte[]> {
        public Message(
                int logbrokerPartition,
                byte[] message
        ) {
            super(logbrokerPartition, message);
        }
    }
}
