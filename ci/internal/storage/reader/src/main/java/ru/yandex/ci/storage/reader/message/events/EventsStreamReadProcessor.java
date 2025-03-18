package ru.yandex.ci.storage.reader.message.events;

import java.io.IOException;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import com.google.common.util.concurrent.Uninterruptibles;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.logbroker.LbCommitCountdown;
import ru.yandex.ci.logbroker.LogbrokerPartitionRead;
import ru.yandex.ci.logbroker.LogbrokerReadProcessor;
import ru.yandex.ci.storage.core.EventsStreamMessages;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.logbroker.LogbrokerLogger;
import ru.yandex.ci.storage.core.utils.NumberOfMessagesFormatter;
import ru.yandex.ci.storage.reader.cache.ReaderCache;
import ru.yandex.ci.storage.reader.message.main.ReaderStatistics;
import ru.yandex.ci.util.Retryable;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerLockMessage;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerReleaseMessage;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.data.MessageData;

import static ru.yandex.ci.storage.core.EventsStreamMessages.EventsStreamMessage.MessagesCase.MESSAGES_NOT_SET;

@Slf4j
public class EventsStreamReadProcessor implements LogbrokerReadProcessor {
    private final ReaderCache readerCache;
    private final ReaderStatistics statistics;
    private final EventsPoolProcessor eventsPoolProcessor;

    public EventsStreamReadProcessor(
            ReaderCache readerCache,
            ReaderStatistics statistics,
            EventsPoolProcessor eventsPoolProcessor
    ) {
        this.readerCache = readerCache;
        this.statistics = statistics;
        this.eventsPoolProcessor = eventsPoolProcessor;
    }

    @Override
    public void process(List<LogbrokerPartitionRead> reads) {
        reads.forEach(read ->
                Retryable.retryUntilInterruptedOrSucceeded(
                        () -> this.process(read),
                        t -> this.statistics.getEvents().onEventWorkerError()
                )
        );
    }

    public void process(LogbrokerPartitionRead read) {
        var parsedMessages = read.getBatches().stream()
                .flatMap(
                        x -> x.getMessageData().stream()
                                .map(messageData -> parseWithRetries(x.getPartition(), messageData))
                )
                .filter(Optional::isPresent)
                .map(optional -> optional.orElseThrow(RuntimeException::new))
                .map(message -> convert(message, read.getCommitCountdown()))
                .collect(Collectors.toList());

        if (parsedMessages.isEmpty()) {
            log.info("All messages skipped with parse errors");
            read.notifyProcessed();
            return;
        }

        LogbrokerLogger.logReceived(parsedMessages, m -> m.getMessage().getMeta());

        var messagesByCase = parsedMessages.stream().collect(
                Collectors.groupingBy(x -> x.getMessage().getMessagesCase())
        );

        var skipped = read.getNumberOfMessages() - parsedMessages.size() +
                messagesByCase.getOrDefault(MESSAGES_NOT_SET, List.of()).size();

        log.info(
                "Processing events logbroker read:, " +
                        "partition: {}, lb messages: {}, messages: {}, skipped: {}, by type: [{}], cookie: {}",
                read.getPartition(),
                read.getNumberOfMessages(),
                parsedMessages.size(),
                skipped,
                NumberOfMessagesFormatter.format(messagesByCase),
                read.getCommitCountdown().getCookie()
        );

        parsedMessages.stream()
                .filter(x -> x.getMessage().getMessagesCase() != MESSAGES_NOT_SET)
                .forEach(eventsPoolProcessor::enqueue);

        if (skipped > 0) {
            log.info("{} messages skipped with parse errors", skipped);
            read.getCommitCountdown().notifyMessageProcessed(skipped);
        }
    }

    private EventMessage convert(EventsStreamMessages.EventsStreamMessage message, LbCommitCountdown commitCountdown) {
        return new EventMessage(
                getCheckId(message),
                commitCountdown,
                message
        );
    }

    private CheckEntity.Id getCheckId(EventsStreamMessages.EventsStreamMessage message) {
        var id = switch (message.getMessagesCase()) {
            case REGISTRATION -> message.getRegistration().getCheck().getId();
            case CANCEL -> message.getCancel().getIterationId().getCheckId();
            case TRACE -> message.getTrace().getFullTaskId().getIterationId().getCheckId();
            case ITERATION_FINISH -> message.getIterationFinish().getIterationId().getCheckId();
            case MESSAGES_NOT_SET -> "0";
        };

        return CheckEntity.Id.of(id);
    }

    @Override
    public void onLock(ConsumerLockMessage lock) {
        log.info("Locking events stream partition: {}, for topic: {}", lock.getPartition(), lock.getTopic());
    }

    @Override
    public void onRelease(ConsumerReleaseMessage release) {
        log.info("Unlocking events stream partition: {}, for topic: {}", release.getPartition(), release.getTopic());
    }

    private Optional<EventsStreamMessages.EventsStreamMessage> parseWithRetries(
            int partition, MessageData messageData
    ) {
        var retryDurationSeconds = Retryable.START_TIMEOUT_SECONDS;
        var retryDurationSecondsMax = Retryable.MAX_TIMEOUT_SECONDS;

        while (!Thread.currentThread().isInterrupted()) {
            try {
                return Optional.of(parseMessage(partition, messageData));
            } catch (RuntimeException | IOException e) {
                log.error("Failed to process message", e);
                this.statistics.getEvents().onParseError();

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

    private EventsStreamMessages.EventsStreamMessage parseMessage(int partition, MessageData data) throws IOException {
        var decompressed = data.getDecompressedData();
        this.statistics.getEvents().onDataReceived(partition, 0, decompressed.length);
        return EventsStreamMessages.EventsStreamMessage.parseFrom(decompressed);
    }
}
