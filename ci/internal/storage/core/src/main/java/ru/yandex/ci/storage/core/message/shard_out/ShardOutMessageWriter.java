package ru.yandex.ci.storage.core.message.shard_out;

import java.util.List;
import java.util.concurrent.CompletableFuture;

import ru.yandex.ci.storage.core.ShardOut;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.kikimr.persqueue.producer.transport.message.inbound.ProducerWriteResponse;

public interface ShardOutMessageWriter {
    void writeFinish(List<ShardOut.ChunkFinished> messages);

    void writeForwarding(List<ShardOut.ShardForwardingMessage> messages);

    List<CompletableFuture<ProducerWriteResponse>> writeAggregates(List<ChunkAggregateEntity> aggregates);
}
