package ru.yandex.ci.storage.shard.message;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.charset.Charset;
import java.util.List;
import java.util.Map;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.logbroker.LbCommitCountdown;
import ru.yandex.ci.logbroker.LogbrokerPartitionRead;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckTaskOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.ShardIn;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateStatistics;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;
import ru.yandex.ci.storage.core.message.shard_out.ShardOutMessageWriter;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.core.utils.TimeTraceService;
import ru.yandex.ci.storage.shard.ShardStatistics;
import ru.yandex.ci.storage.shard.StorageShardTestBase;
import ru.yandex.ci.storage.shard.task.AggregateProcessor;
import ru.yandex.ci.storage.shard.task.TaskDiffProcessor;
import ru.yandex.kikimr.persqueue.compression.CompressionCodec;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.data.MessageBatch;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.data.MessageData;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.data.MessageMeta;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

public class MessageProcessingIntegrationTest extends StorageShardTestBase {
    private AggregatePoolProcessor aggregatePoolProcessor;
    private ChunkPoolProcessor chunkPoolProcessor;
    private ShardInReadProcessor readProcessor;

    private final ChunkEntity.Id chunkId = ChunkEntity.Id.of(Common.ChunkType.CT_SMALL_TEST, 1);

    @BeforeEach
    public void setup() {
        var aggregateReporter = mock(AggregateReporter.class);
        var shardOutMessageWriter = mock(ShardOutMessageWriter.class);
        var statistics = mock(ShardStatistics.class);
        var time = mock(TimeTraceService.class);
        var trace = mock(TimeTraceService.Trace.class);
        var postProcessor = mock(PostProcessorDeliveryService.class);
        when(time.createTrace(anyString())).thenReturn(trace);

        this.aggregatePoolProcessor = new AggregatePoolProcessor(
                db, new AggregateProcessor(new TaskDiffProcessor(statistics)),
                shardCache, aggregateReporter, shardOutMessageWriter, time, statistics, postProcessor, 0, 0
        );

        this.chunkPoolProcessor = new ChunkPoolProcessor(
                db, shardCache, timeTraceService, aggregatePoolProcessor, statistics, 0, 0, true
        );

        this.readProcessor = new ShardInReadProcessor(
                chunkPoolProcessor, shardCache, statistics
        );
    }

    @Test
    public void commits() throws IOException {
        var aggregateId = new ChunkAggregateEntity.Id(sampleIterationId, chunkId);
        db.currentOrTx(() -> {
            db.chunkAggregates().save(
                    ChunkAggregateEntity.builder()
                            .id(aggregateId)
                            .statistics(ChunkAggregateStatistics.EMPTY)
                            .state(Common.ChunkAggregateState.CAS_RUNNING)
                            .build()
            );
            db.checks().save(
                    CheckEntity.builder()
                            .id(new CheckEntity.Id(1L))
                            .left(StorageRevision.EMPTY)
                            .right(StorageRevision.EMPTY)
                            .build()
            );
        });

        var lbCommitCountdown = mock(LbCommitCountdown.class);

        var resultMessage = ShardIn.ShardInMessage.newBuilder()
                .setChunk(
                        ShardIn.ChunkMessage.newBuilder()
                                .setChunkId(CheckProtoMappers.toProtoChunkId(chunkId))
                                .setResult(
                                        ShardIn.ShardResultMessage.newBuilder()
                                                .setFullTaskId(CheckTaskOuterClass.FullTaskId.newBuilder()
                                                        .setIterationId(CheckIteration.IterationId.newBuilder()
                                                                .setCheckId("1"))
                                                )
                                                .build()
                                )
                                .build()
                )
                .build();

        var finishMessage = ShardIn.ShardInMessage.newBuilder()
                .setChunk(
                        ShardIn.ChunkMessage.newBuilder()
                                .setChunkId(CheckProtoMappers.toProtoChunkId(chunkId))
                                .setFinish(
                                        ShardIn.FinishChunk.newBuilder()
                                                .setIterationId(CheckProtoMappers.toProtoIterationId(sampleIterationId))
                                                .setState(Common.ChunkAggregateState.CAS_COMPLETED)
                                                .build()
                                )
                                .build()
                )
                .build();

        var badChunkMessage = ShardIn.ShardInMessage.newBuilder()
                .setChunk(
                        ShardIn.ChunkMessage.newBuilder()
                                .setChunkId(CheckProtoMappers.toProtoChunkId(chunkId))
                                .build()
                )
                .build();

        var batch = new MessageBatch(
                "shard_in",
                1,
                List.of(
                        toMessageData(resultMessage),
                        toMessageData(finishMessage),
                        toMessageData("test".getBytes(Charset.defaultCharset())), // wrong message
                        toMessageData(new byte[0]), // message not set
                        toMessageData(badChunkMessage)

                )
        );

        when(lbCommitCountdown.getNumberOfMessages()).thenReturn(5);

        readProcessor.process(
                new LogbrokerPartitionRead(1, lbCommitCountdown, List.of(batch))
        );

        verify(lbCommitCountdown, times(3)).notifyMessageProcessed();
        verify(lbCommitCountdown, times(1)).notifyMessageProcessed(eq(2));

        db.currentOrReadOnly(() -> {
            assertThat(db.chunkAggregates().get(aggregateId).getState())
                    .isEqualTo(Common.ChunkAggregateState.CAS_COMPLETED);
        });
    }

    private MessageData toMessageData(ShardIn.ShardInMessage message) throws IOException {
        return toMessageData(serialize(message));
    }

    private MessageData toMessageData(byte[] data) {
        return new MessageData(
                data, 0, new MessageMeta(new byte[0], 0, 0, 0, "", CompressionCodec.RAW, Map.of())
        );
    }

    private byte[] serialize(ShardIn.ShardInMessage message) throws IOException {
        var output = new ByteArrayOutputStream();
        message.writeTo(output);
        return output.toByteArray();
    }
}
