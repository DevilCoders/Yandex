package ru.yandex.ci.storage.shard.message;

import java.io.IOException;
import java.util.HashSet;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import com.google.common.util.concurrent.Uninterruptibles;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.logbroker.LbCommitCountdown;
import ru.yandex.ci.logbroker.LogbrokerPartitionRead;
import ru.yandex.ci.logbroker.LogbrokerReadProcessor;
import ru.yandex.ci.storage.core.ShardIn;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.logbroker.LogbrokerLogger;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.core.utils.NumberOfMessagesFormatter;
import ru.yandex.ci.storage.shard.ShardStatistics;
import ru.yandex.ci.storage.shard.cache.ShardCache;
import ru.yandex.ci.util.Retryable;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerLockMessage;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerReleaseMessage;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.data.MessageData;

@Slf4j
public class ShardInReadProcessor implements LogbrokerReadProcessor {

    private final ShardCache shardCache;
    private final ShardStatistics statistics;

    private final ChunkPoolProcessor chunkProcessor;

    private final Set<ChunkEntity.Id> lockedChunks = new HashSet<>();

    public ShardInReadProcessor(
            ChunkPoolProcessor chunkProcessor,
            ShardCache shardCache,
            ShardStatistics statistics
    ) {
        this.shardCache = shardCache;
        this.statistics = statistics;
        this.chunkProcessor = chunkProcessor;
    }

    @Override
    public void process(List<LogbrokerPartitionRead> reads) {
        reads.forEach(read ->
                Retryable.retryUntilInterruptedOrSucceeded(
                        () -> this.process(read),
                        t -> this.statistics.onChunkWorkerError()
                )
        );
    }

    @Override
    public void onLock(ConsumerLockMessage lock) {
        var chunks = this.shardCache.chunks().getAll().stream()
                .filter(x -> x.getPartition() == lock.getPartition())
                .map(ChunkEntity::getId)
                .collect(Collectors.toSet());

        lockedChunks.addAll(chunks);

        log.info(
                "Locking shard_in stream partition: {}, for topic: {}, chunks: [{}]",
                lock.getPartition(), lock.getTopic(),
                chunks.stream().map(ChunkEntity.Id::toString).collect(Collectors.joining(", "))
        );

        this.shardCache.modify(cache -> {
            cache.chunkAggregates().invalidate(chunks);
            cache.testDiffs().invalidate(chunks);
        });

        log.info("Clean up for locking partition {} completed", lock.getPartition());
        reportLockedChunks();
    }

    @Override
    public void onRelease(ConsumerReleaseMessage release) {
        var chunks = this.shardCache.chunks().getAll().stream()
                .filter(x -> x.getPartition() == release.getPartition())
                .map(ChunkEntity::getId)
                .collect(Collectors.toSet());

        lockedChunks.removeAll(chunks);

        log.info(
                "Unlocking shard_in stream partition: {}, for topic: {}, chunks: [{}]",
                release.getPartition(), release.getTopic(),
                chunks.stream().map(ChunkEntity.Id::toString).collect(Collectors.joining(", "))
        );

        reportLockedChunks();
    }

    public void process(LogbrokerPartitionRead read) {
        var parsedMessages = read.getBatches().stream()
                .flatMap(
                        batch -> batch.getMessageData().stream()
                                .map(messageData -> parseWithRetries(batch.getPartition(), messageData))
                )
                .filter(Optional::isPresent)
                .map(optional -> optional.orElseThrow(RuntimeException::new))
                .collect(Collectors.toList());

        LogbrokerLogger.logReceived(
                parsedMessages,
                ShardIn.ShardInMessage::getMeta
        );

        if (parsedMessages.isEmpty()) {
            log.info("All messages skipped with parse errors");
            read.notifyProcessed();
            return;
        }

        var messagesByCase =
                parsedMessages.stream().collect(Collectors.groupingBy(ShardIn.ShardInMessage::getMessageCase));

        var skipped = read.getNumberOfMessages() - parsedMessages.size() +
                messagesByCase.getOrDefault(ShardIn.ShardInMessage.MessageCase.MESSAGE_NOT_SET, List.of()).size();

        log.info(
                "Processing shard_in logbroker read, " +
                        "partition: {}, batches: {}, lb messages: {}, " +
                        "messages: {}, by type: {}, skipped: {}, read: {}, chunks: [{}]",
                read.getPartition(),
                read.getBatches().size(),
                read.getNumberOfMessages(),
                parsedMessages.size(),
                NumberOfMessagesFormatter.format(messagesByCase),
                skipped,
                read.getCommitCountdown().getCookie(),
                parsedMessages.stream()
                        .filter(x -> x.getMessageCase().equals(ShardIn.ShardInMessage.MessageCase.CHUNK))
                        .map(ShardIn.ShardInMessage::getChunk)
                        .map(x -> CheckProtoMappers.toChunkId(x.getChunkId()).toString())
                        .collect(Collectors.joining(", "))
        );

        this.processChunkMessages(
                read.getCommitCountdown(),
                messagesByCase.getOrDefault(ShardIn.ShardInMessage.MessageCase.CHUNK, List.of())
        );

        if (skipped > 0) {
            log.info("{} messages skipped with parse errors", skipped);
            read.getCommitCountdown().notifyMessageProcessed(skipped);
        }
    }

    private void processChunkMessages(LbCommitCountdown lbCommitCountdown, List<ShardIn.ShardInMessage> messages) {
        if (messages.isEmpty()) {
            return;
        }

        var messagesByCase =
                messages.stream().collect(Collectors.groupingBy(x -> x.getChunk().getMessageCase()));

        var notSetMessages = messagesByCase.getOrDefault(
                ShardIn.ChunkMessage.MessageCase.MESSAGE_NOT_SET, List.of()
        );
        var resultMessages = messagesByCase.getOrDefault(ShardIn.ChunkMessage.MessageCase.RESULT, List.of());
        var finishMessages = messagesByCase.getOrDefault(ShardIn.ChunkMessage.MessageCase.FINISH, List.of());
        var chunks = new HashSet<String>();
        chunks.addAll(
                resultMessages.stream()
                        .map(ShardIn.ShardInMessage::getChunk)
                        .map(m -> "[%s/%s]".formatted(
                                        CheckProtoMappers.toIterationId(m.getResult().getFullTaskId().getIterationId()),
                                        CheckProtoMappers.toChunkId(m.getChunkId())
                                )
                        ).toList()
        );

        chunks.addAll(
                finishMessages.stream()
                        .map(ShardIn.ShardInMessage::getChunk)
                        .map(m -> "[%s/%s]".formatted(
                                        CheckProtoMappers.toIterationId(m.getFinish().getIterationId()),
                                        CheckProtoMappers.toChunkId(m.getChunkId())
                                )
                        ).toList()
        );

        log.info(
                "Processing {} chunk messages, read: {}, messages: {}, chunks: [{}]",
                messages.size(),
                lbCommitCountdown.getCookie(),
                NumberOfMessagesFormatter.format(messagesByCase),
                String.join(", ", chunks)
        );

        enqueueToChunkProcessor(lbCommitCountdown, notSetMessages);
        enqueueToChunkProcessor(lbCommitCountdown, resultMessages);
        enqueueToChunkProcessor(lbCommitCountdown, finishMessages);
    }

    private void enqueueToChunkProcessor(LbCommitCountdown lbCommitCountdown, List<ShardIn.ShardInMessage> messages) {
        for (var message : messages) {
            var chunkId = CheckProtoMappers.toChunkId(message.getChunk().getChunkId());

            chunkProcessor.enqueue(
                    new ChunkPoolMessage(message.getMeta(), chunkId, lbCommitCountdown, message.getChunk())
            );
        }
    }

    private Optional<ShardIn.ShardInMessage> parseWithRetries(
            int partition,
            MessageData messageData
    ) {
        var retryDurationSeconds = Retryable.START_TIMEOUT_SECONDS;
        var retryDurationSecondsMax = Retryable.MAX_TIMEOUT_SECONDS;

        while (!Thread.currentThread().isInterrupted()) {
            try {
                return Optional.of(parseMessage(partition, messageData));
            } catch (RuntimeException | IOException e) {
                log.error("Failed to process message", e);
                this.statistics.onParseError();

                if (shardCache.settings().get().getShard().getSkip().isParseError()) {
                    log.info("Skipping parse error");
                    return Optional.empty();
                }

                Uninterruptibles.sleepUninterruptibly(retryDurationSeconds, TimeUnit.SECONDS);
                retryDurationSeconds = Math.min(retryDurationSecondsMax, retryDurationSeconds * 2);
            }
        }

        return Optional.empty();
    }

    private ShardIn.ShardInMessage parseMessage(int partition, MessageData data) throws IOException {
        var decompressed = data.getDecompressedData();
        this.statistics.onDataReceived(partition, data.getRawData().length, decompressed.length);
        return ShardIn.ShardInMessage.parseFrom(decompressed);
    }

    private void reportLockedChunks() {
        log.info(
                "Locked chunks: [{}]",
                lockedChunks.stream().map(ChunkEntity.Id::toString).collect(Collectors.joining(", "))
        );
    }
}
