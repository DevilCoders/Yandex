package ru.yandex.ci.storage.core.message.shard_out;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.stream.Collectors;

import com.google.common.collect.Lists;
import io.micrometer.core.instrument.MeterRegistry;

import ru.yandex.ci.storage.core.ShardOut;
import ru.yandex.ci.storage.core.cache.ChecksCache;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactory;
import ru.yandex.ci.storage.core.logbroker.StorageMessageWriter;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.kikimr.persqueue.producer.transport.message.inbound.ProducerWriteResponse;

public class ShardOutMessageWriterImpl extends StorageMessageWriter implements ShardOutMessageWriter {
    private static final int MAX_NUMBER_OF_MESSAGES_IN_WRITE = 512;

    private final ChecksCache checksCache;

    public ShardOutMessageWriterImpl(
            ChecksCache checksCache,
            MeterRegistry meterRegistry,
            int numberOfPartitions,
            LogbrokerWriterFactory logbrokerWriterFactory
    ) {
        super(meterRegistry, numberOfPartitions, logbrokerWriterFactory);
        this.checksCache = checksCache;
    }

    @Override
    public void writeForwarding(List<ShardOut.ShardForwardingMessage> messages) {
        var byPartition = messages.stream().collect(
                Collectors.groupingBy(
                        m -> getPartition(CheckEntity.Id.of(m.getFullTaskId().getIterationId().getCheckId()))
                )
        );

        var outMessages = new ArrayList<StorageMessageWriter.Message>();
        for (var partition : byPartition.entrySet()) {
            var meta = this.createMeta();
            outMessages.add(
                    new StorageMessageWriter.Message(
                            meta,
                            partition.getKey(),
                            ShardOut.ShardOutMessages.newBuilder()
                                    .setMeta(meta)
                                    .addAllMessages(
                                            partition.getValue().stream()
                                                    .map(this::toMessage)
                                                    .collect(Collectors.toList())
                                    )
                                    .build(),
                            "Forwarding messages: " + messages.stream()
                                    .map(x -> CheckProtoMappers.toTaskId(x.getFullTaskId()) + ":" + x.getMessageCase())
                                    .collect(Collectors.joining(", "))
                    )
            );
        }

        this.writeWithRetries(outMessages);
    }

    @Override
    public void writeFinish(List<ShardOut.ChunkFinished> messages) {
        var byPartition = messages.stream().collect(
                Collectors.groupingBy(
                        m -> getPartition(CheckEntity.Id.of(m.getAggregate().getId().getIterationId().getCheckId()))
                )
        );

        var outMessages = new ArrayList<StorageMessageWriter.Message>();
        for (var partition : byPartition.entrySet()) {
            var meta = this.createMeta();
            outMessages.add(
                    new StorageMessageWriter.Message(
                            meta,
                            partition.getKey(),
                            ShardOut.ShardOutMessages.newBuilder()
                                    .setMeta(meta)
                                    .addAllMessages(
                                            partition.getValue().stream()
                                                    .map(this::toMessage)
                                                    .collect(Collectors.toList())
                                    )
                                    .build(),
                            "Chunk finished messages: " + messages.stream()
                                    .map(x -> CheckProtoMappers.toAggregateId(x.getAggregate().getId()).toString())
                                    .collect(Collectors.joining(", "))
                    )
            );
        }

        this.writeWithRetries(outMessages);
    }

    private ShardOut.ShardOutMessage toMessage(ShardOut.ChunkFinished message) {
        return ShardOut.ShardOutMessage.newBuilder()
                .setMeta(createMeta())
                .setIterationId(message.getAggregate().getId().getIterationId())
                .setFinished(message)
                .build();
    }

    private ShardOut.ShardOutMessage toMessage(ShardOut.ShardForwardingMessage message) {
        return ShardOut.ShardOutMessage.newBuilder()
                .setMeta(createMeta())
                .setIterationId(message.getFullTaskId().getIterationId())
                .setForwarding(message)
                .build();
    }

    @Override
    public List<CompletableFuture<ProducerWriteResponse>> writeAggregates(List<ChunkAggregateEntity> aggregates) {
        var byPartition = aggregates.stream().collect(
                Collectors.groupingBy(x -> getPartition(x.getId().getIterationId().getCheckId()))
        );

        var messages = new ArrayList<StorageMessageWriter.Message>();
        for (var partition : byPartition.entrySet()) {
            for (var batch : Lists.partition(partition.getValue(), MAX_NUMBER_OF_MESSAGES_IN_WRITE)) {
                var meta = this.createMeta();
                messages.add(
                        new StorageMessageWriter.Message(
                                meta,
                                partition.getKey(),
                                ShardOut.ShardOutMessages.newBuilder()
                                        .setMeta(meta)
                                        .addAllMessages(
                                                batch.stream().map(this::toMessage).collect(Collectors.toList())
                                        )
                                        .build(),
                                "Aggregates: " + aggregates.size()
                        )
                );
            }
        }

        return this.writeWithoutRetries(messages);
    }

    private ShardOut.ShardOutMessage toMessage(ChunkAggregateEntity aggregate) {
        return ShardOut.ShardOutMessage.newBuilder()
                .setMeta(createMeta())
                .setIterationId(CheckProtoMappers.toProtoIterationId(aggregate.getId().getIterationId()))
                .setAggregate(CheckProtoMappers.toProtoAggregate(aggregate))
                .build();
    }

    private int getPartition(CheckEntity.Id checkId) {
        return this.checksCache.getOrThrow(checkId).getShardOutPartition();
    }
}
