package ru.yandex.ci.storage.reader.message.writer;

import java.util.List;
import java.util.stream.Collectors;

import io.micrometer.core.instrument.MeterRegistry;

import ru.yandex.ci.storage.core.ShardIn;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactory;
import ru.yandex.ci.storage.core.logbroker.StorageMessageWriter;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.reader.cache.ReaderCache;

public class ShardInMessageWriterImpl extends StorageMessageWriter implements ShardInMessageWriter {

    public ShardInMessageWriterImpl(
            int numberOfPartitions,
            MeterRegistry meterRegistry,
            LogbrokerWriterFactory logbrokerWriterFactory
    ) {
        super(meterRegistry, numberOfPartitions, logbrokerWriterFactory);
    }

    @Override
    public void writeChunkMessages(ReaderCache.Modifiable cache, List<ShardIn.ChunkMessage> messages) {
        var messagesToWrite = messages.stream().map(
                message -> {
                    var shardInMessage = ShardIn.ShardInMessage.newBuilder()
                            .setMeta(createMeta())
                            .setChunk(message)
                            .build();

                    var chunkId = CheckProtoMappers.toChunkId(message.getChunkId());
                    var description = "Chunk message: %s/%s".formatted(chunkId, shardInMessage.getMessageCase());
                    if (message.getMessageCase().equals(ShardIn.ChunkMessage.MessageCase.RESULT)) {
                        description += ", task: " + CheckProtoMappers.toTaskId(message.getResult().getFullTaskId());
                    } else if (message.getMessageCase().equals(ShardIn.ChunkMessage.MessageCase.FINISH)) {
                        description += ", iteration: " + CheckProtoMappers.toIterationId(
                                message.getFinish().getIterationId()
                        );
                    }

                    return new StorageMessageWriter.Message(
                            shardInMessage.getMeta(),
                            getPartition(cache, chunkId),
                            shardInMessage,
                            description
                    );
                }
        ).collect(Collectors.toList());

        this.writeWithRetries(messagesToWrite);
    }

    private int getPartition(ReaderCache.Modifiable cache, ChunkEntity.Id chunkId) {
        return cache.chunks().getOrThrow(chunkId).getPartition();
    }
}
