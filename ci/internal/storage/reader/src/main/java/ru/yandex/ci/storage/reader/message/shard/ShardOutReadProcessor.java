package ru.yandex.ci.storage.reader.message.shard;

import java.io.IOException;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import com.google.common.util.concurrent.Uninterruptibles;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.logbroker.LogbrokerPartitionRead;
import ru.yandex.ci.logbroker.LogbrokerReadProcessor;
import ru.yandex.ci.storage.core.CheckTaskOuterClass;
import ru.yandex.ci.storage.core.ShardOut;
import ru.yandex.ci.storage.core.db.constant.CheckStatusUtils;
import ru.yandex.ci.storage.core.logbroker.LogbrokerLogger;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.core.utils.NumberOfMessagesFormatter;
import ru.yandex.ci.storage.core.utils.TimeTraceService;
import ru.yandex.ci.storage.reader.cache.ReaderCache;
import ru.yandex.ci.storage.reader.message.main.ReaderStatistics;
import ru.yandex.ci.util.Retryable;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerLockMessage;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerReleaseMessage;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.data.MessageData;

@Slf4j
@RequiredArgsConstructor
public class ShardOutReadProcessor implements LogbrokerReadProcessor {
    @Nonnull
    private final ForwardingMessageProcessor forwardingMessageProcessor;

    @Nonnull
    private final ReaderCache readerCache;

    @Nonnull
    private final ReaderStatistics statistics;

    @Nonnull
    private final ChunkMessageProcessor chunkMessageProcessor;

    private final TimeTraceService timeTraceService;

    @Override
    public void onLock(ConsumerLockMessage lock) {
        log.info("Locking shard_out stream partition: {}, for topic: {}", lock.getPartition(), lock.getTopic());
    }

    @Override
    public void onRelease(ConsumerReleaseMessage release) {
        log.info("Unlocking shard_out stream partition: {}, for topic: {}", release.getPartition(), release.getTopic());
    }

    @Override
    public void process(List<LogbrokerPartitionRead> reads) {
        var parsedMessages = reads.stream()
                .flatMap(read -> read.getBatches().stream()
                        .flatMap(batch -> batch.getMessageData().stream())
                        .map(data -> parseWithRetries(read.getPartition(), data))
                )
                .filter(Optional::isPresent)
                .map(optional -> optional.orElseThrow(RuntimeException::new))
                .collect(Collectors.toList());

        LogbrokerLogger.logReceived(
                parsedMessages,
                ShardOut.ShardOutMessages::getMeta
        );

        var messages = parsedMessages.stream()
                .flatMap(x -> x.getMessagesList().stream())
                .filter(this::isCheckPresentAndIterationIsActive)
                .toList();

        var messagesByCase =
                messages.stream().collect(Collectors.groupingBy(ShardOut.ShardOutMessage::getMessagesCase));

        log.info(
                "Processing shard_out logbroker reads: {}, partitions: [{}], messages by type: [{}], reads: [{}]",
                reads.size(),
                reads.stream()
                        .map(LogbrokerPartitionRead::getPartition)
                        .map(Object::toString).collect(Collectors.joining(", ")),
                NumberOfMessagesFormatter.format(messagesByCase),
                reads.stream()
                        .map(read -> read.getCommitCountdown().getCookie())
                        .map(Object::toString)
                        .collect(Collectors.joining(", "))
        );

        var trace = timeTraceService.createTrace("shard_out_processor_timing");

        this.processForwardingMessages(
                messagesByCase.getOrDefault(ShardOut.ShardOutMessage.MessagesCase.FORWARDING, List.of()),
                trace
        );

        trace.step("process_forwarding");

        this.chunkMessageProcessor.processAggregateMessages(
                messagesByCase.getOrDefault(ShardOut.ShardOutMessage.MessagesCase.AGGREGATE, List.of())
        );

        trace.step("process_aggregate");

        this.chunkMessageProcessor.processChunkFinishedMessages(
                messagesByCase.getOrDefault(ShardOut.ShardOutMessage.MessagesCase.FINISHED, List.of())
        );

        trace.step("process_finished");

        for (var entry : messagesByCase.entrySet()) {
            if (!entry.getKey().equals(ShardOut.ShardOutMessage.MessagesCase.FORWARDING)) {
                this.statistics.getShard().onMessageProcessed(entry.getKey(), entry.getValue().size());
            }
        }

        log.info("Shard out timings: {}", trace.logString());

        reads.forEach(LogbrokerPartitionRead::notifyProcessed);
    }

    private void processForwardingMessages(List<ShardOut.ShardOutMessage> messages, TimeTraceService.Trace trace) {
        if (messages.isEmpty()) {
            return;
        }

        messages = messages.stream()
                .filter(m -> isCancelMessage(m) || isTaskPresentAndActive(m.getForwarding().getFullTaskId()))
                .collect(Collectors.toList());

        this.forwardingMessageProcessor.process(
                messages.stream()
                        .map(ShardOut.ShardOutMessage::getForwarding)
                        .collect(Collectors.toList()),
                trace
        );

        var byCase = messages.stream().collect(Collectors.groupingBy(x -> x.getForwarding().getMessageCase()));
        for (var entry : byCase.entrySet()) {
            this.statistics.getShard().onMessageProcessed(entry.getKey(), entry.getValue().size());
        }
    }

    private boolean isCancelMessage(ShardOut.ShardOutMessage message) {
        return message.getForwarding().getMessageCase() == ShardOut.ShardForwardingMessage.MessageCase.CANCEL;
    }

    private boolean isCheckPresentAndIterationIsActive(ShardOut.ShardOutMessage message) {
        var iterationId = CheckProtoMappers.toIterationId(message.getIterationId());
        var check = this.readerCache.checks().get(iterationId.getCheckId());
        if (check.isEmpty()) {
            log.warn("Skipping shard out message because check {} not found", iterationId.getCheckId());
            this.statistics.getShard().onMissingError();
            return false;
        }

        var iteration = this.readerCache.iterations().get(iterationId);
        if (iteration.isEmpty()) {
            iteration = this.readerCache.iterations().getFresh(iterationId);
            if (iteration.isEmpty()) {
                log.warn("Skipping shard out message because iteration {} not found", iterationId);
                this.statistics.getShard().onMissingError();
                return false;
            }
        }

        if (iterationId.getNumber() > 1) {
            var metaIteration = this.readerCache.iterations().get(iterationId.toMetaId());
            if (metaIteration.isEmpty()) {
                metaIteration = this.readerCache.iterations().getFresh(iterationId.toMetaId());
                if (metaIteration.isEmpty()) {
                    log.warn("Skipping shard out message because meta iteration {} not found", iterationId.toMetaId());
                    this.statistics.getShard().onMissingError();
                    return false;
                }
            }
        }

        if (CheckStatusUtils.isCompleted(iteration.orElseThrow().getStatus())) {
            log.warn(
                    "Skipping shard out message {} because iteration {} is not active, status: {}",
                    message.getMessagesCase(), iteration.get().getId(), iteration.get().getStatus()
            );
            return false;
        }

        return true;
    }

    private boolean isTaskPresentAndActive(CheckTaskOuterClass.FullTaskId id) {
        var taskId = CheckProtoMappers.toTaskId(id);
        var task = this.readerCache.checkTasks().get(taskId);
        if (task.isEmpty()) {
            log.warn("Skipping shard out message because task {} not found", taskId);
            this.statistics.getShard().onMissingError();
            return false;
        }

        if (CheckStatusUtils.isCompleted(task.get().getStatus())) {
            log.warn("Skipping shard out message because task {} is not active", task.get().getId());
            return false;
        }

        return true;
    }

    private Optional<ShardOut.ShardOutMessages> parseWithRetries(
            int partition, MessageData messageData
    ) {
        var retryDurationSeconds = Retryable.START_TIMEOUT_SECONDS;
        var retryDurationSecondsMax = Retryable.MAX_TIMEOUT_SECONDS;

        while (!Thread.currentThread().isInterrupted()) {
            try {
                return Optional.of(parseMessage(partition, messageData));
            } catch (RuntimeException | IOException e) {
                log.error("Failed to parse message", e);
                this.statistics.getShard().onParseError();

                if (readerCache.settings().get().getReader().getSkip().isParseError()) {
                    log.info("Skipping parse error");
                    return Optional.empty();
                }

                Uninterruptibles.sleepUninterruptibly(retryDurationSeconds, TimeUnit.SECONDS);
                retryDurationSeconds = Math.min(retryDurationSecondsMax, retryDurationSeconds * 2);
            }
        }

        return Optional.empty();
    }

    private ShardOut.ShardOutMessages parseMessage(int partition, MessageData data) throws IOException {
        var decompressed = data.getDecompressedData();
        this.statistics.getShard().onDataReceived(partition, 0, decompressed.length);
        return ShardOut.ShardOutMessages.parseFrom(decompressed);
    }
}
