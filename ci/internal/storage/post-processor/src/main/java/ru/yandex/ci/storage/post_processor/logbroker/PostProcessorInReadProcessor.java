package ru.yandex.ci.storage.post_processor.logbroker;

import java.io.IOException;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import com.google.common.util.concurrent.Uninterruptibles;
import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.logbroker.LbCommitCountdown;
import ru.yandex.ci.logbroker.LogbrokerPartitionRead;
import ru.yandex.ci.logbroker.LogbrokerReadProcessor;
import ru.yandex.ci.storage.core.PostProcessor;
import ru.yandex.ci.storage.core.PostProcessor.PostProcessorInMessage.MessageCase;
import ru.yandex.ci.storage.core.logbroker.LogbrokerLogger;
import ru.yandex.ci.storage.core.utils.NumberOfMessagesFormatter;
import ru.yandex.ci.storage.post_processor.PostProcessorStatistics;
import ru.yandex.ci.storage.post_processor.cache.PostProcessorCache;
import ru.yandex.ci.storage.post_processor.processing.MessageProcessorPool;
import ru.yandex.ci.storage.post_processor.processing.ResultMessage;
import ru.yandex.ci.storage.post_processor.proto.PostProcessorProtoMappers;
import ru.yandex.ci.util.Retryable;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerLockMessage;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerReleaseMessage;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.data.MessageData;

@Slf4j
@AllArgsConstructor
public class PostProcessorInReadProcessor implements LogbrokerReadProcessor {
    private final MessageProcessorPool messageProcessor;
    private final PostProcessorStatistics statistics;
    private final PostProcessorCache cache;

    @Override
    public void process(List<LogbrokerPartitionRead> reads) {
        reads.forEach(read ->
                Retryable.retryUntilInterruptedOrSucceeded(
                        () -> this.process(read),
                        t -> this.statistics.onPostProcessorWorkerError()
                )
        );
    }

    @Override
    public void onLock(ConsumerLockMessage lock) {
        log.info(
                "Locking post_processor_in stream partition: {}, for topic: {}",
                lock.getPartition(), lock.getTopic()
        );
    }

    @Override
    public void onRelease(ConsumerReleaseMessage release) {
        log.info(
                "Unlocking post_processor_in stream partition: {}, for topic: {}",
                release.getPartition(), release.getTopic()
        );

        cache.testHistory().invalidatePartition(release.getPartition());
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

        LogbrokerLogger.logReceived(parsedMessages, PostProcessor.PostProcessorInMessage::getMeta);

        if (parsedMessages.isEmpty()) {
            log.info("All messages skipped with parse errors");
            read.notifyProcessed();
            return;
        }

        var messagesByCase = parsedMessages.stream().collect(
                Collectors.groupingBy(PostProcessor.PostProcessorInMessage::getMessageCase)
        );

        var skipped = read.getNumberOfMessages() - parsedMessages.size() +
                messagesByCase.getOrDefault(MessageCase.MESSAGE_NOT_SET, List.of()).size();

        log.info(
                "Processing post_processor_in logbroker read, " +
                        "partition: {}, batches: {}, lb messages: {}, " +
                        "messages: {}, by type: {}, skipped: {}, read: {}",
                read.getPartition(),
                read.getBatches().size(),
                read.getNumberOfMessages(),
                parsedMessages.size(),
                NumberOfMessagesFormatter.format(messagesByCase),
                skipped,
                read.getCommitCountdown().getCookie()
        );

        this.processResultsMessages(
                read.getCommitCountdown(),
                messagesByCase.getOrDefault(MessageCase.TEST_RESULTS, List.of())
        );

        if (skipped > 0) {
            log.info("{} messages skipped with parse errors", skipped);
            read.getCommitCountdown().notifyMessageProcessed("read_processor_skipped", skipped);
            read.getCommitCountdown().notifyMessageProcessed(skipped);
        }
    }

    private void processResultsMessages(
            LbCommitCountdown lbCommitCountdown, List<PostProcessor.PostProcessorInMessage> messages
    ) {
        if (messages.isEmpty()) {
            log.warn("No result messages");
            return;
        }

        var results = PostProcessorProtoMappers.convert(
                        messages.stream().flatMap(x -> x.getTestResults().getResultsList().stream()).toList()
                ).stream()
                .filter(x -> cache.checks().get(x.getId().getCheckId()).isPresent())
                .filter(x -> cache.iterations().get(x.getId().getIterationId()).isPresent())
                .toList();

        if (results.isEmpty()) {
            lbCommitCountdown.notifyMessageProcessed("read_processor", messages.size());
            lbCommitCountdown.notifyMessageProcessed(messages.size());
            return;
        }

        var messagesCountdown = lbCommitCountdown.createdNestedCountdown(messages.size(), results.size());

        enqueue(results.stream().map(r -> new ResultMessage(messagesCountdown, r)).toList());

        lbCommitCountdown.notifyMessageProcessed("read_processor", messages.size());
    }

    private void enqueue(List<ResultMessage> results) {
        results.forEach(this.messageProcessor::enqueue);
    }

    private Optional<PostProcessor.PostProcessorInMessage> parseWithRetries(
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

                if (cache.settings().get().getPostProcessor().getSkip().isParseError()) {
                    log.info("Skipping parse error");
                    return Optional.empty();
                }

                Uninterruptibles.sleepUninterruptibly(retryDurationSeconds, TimeUnit.SECONDS);
                retryDurationSeconds = Math.min(retryDurationSecondsMax, retryDurationSeconds * 2);
            }
        }

        return Optional.empty();
    }

    private PostProcessor.PostProcessorInMessage parseMessage(int partition, MessageData data) throws IOException {
        var decompressed = data.getDecompressedData();
        this.statistics.onDataReceived(partition, data.getRawData().length, decompressed.length);
        return PostProcessor.PostProcessorInMessage.parseFrom(decompressed);
    }
}
