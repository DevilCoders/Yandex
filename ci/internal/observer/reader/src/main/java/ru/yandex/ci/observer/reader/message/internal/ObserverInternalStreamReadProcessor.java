package ru.yandex.ci.observer.reader.message.internal;

import java.io.IOException;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import com.google.common.util.concurrent.Uninterruptibles;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.logbroker.LogbrokerPartitionRead;
import ru.yandex.ci.logbroker.LogbrokerReadProcessor;
import ru.yandex.ci.observer.core.Internal;
import ru.yandex.ci.observer.core.cache.ObserverCache;
import ru.yandex.ci.observer.core.db.model.check.CheckEntity;
import ru.yandex.ci.observer.reader.message.internal.cache.LoadedPartitionEntitiesCache;
import ru.yandex.ci.storage.core.logbroker.LogbrokerLogger;
import ru.yandex.ci.util.Retryable;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerLockMessage;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerReleaseMessage;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.data.MessageData;

@Slf4j
public class ObserverInternalStreamReadProcessor implements LogbrokerReadProcessor {
    private final ObserverCache cache;
    private final ObserverEntitiesChecker entitiesChecker;
    private final LoadedPartitionEntitiesCache loadedEntitiesCache;
    private final InternalStreamStatistics statistics;
    private final ObserverInternalStreamQueuedReadProcessor queuedReadProcessor;
    private final Set<Internal.InternalMessage.MessagesCase> messageCasesToProcess = Set.of(
            Internal.InternalMessage.MessagesCase.REGISTERED,
            Internal.InternalMessage.MessagesCase.FATAL_ERROR,
            Internal.InternalMessage.MessagesCase.CANCEL,
            Internal.InternalMessage.MessagesCase.PESSIMIZE,
            Internal.InternalMessage.MessagesCase.TECHNICAL_STATS_MESSAGE,
            Internal.InternalMessage.MessagesCase.FINISH_PARTITION,
            Internal.InternalMessage.MessagesCase.AGGREGATE,
            Internal.InternalMessage.MessagesCase.ITERATION_FINISH
    );

    public ObserverInternalStreamReadProcessor(
            ObserverCache cache,
            ObserverEntitiesChecker entitiesChecker,
            LoadedPartitionEntitiesCache loadedEntitiesCache,
            InternalStreamStatistics statistics,
            ObserverInternalStreamQueuedReadProcessor queuedReadProcessor
    ) {
        this.cache = cache;
        this.entitiesChecker = entitiesChecker;
        this.loadedEntitiesCache = loadedEntitiesCache;
        this.statistics = statistics;
        this.queuedReadProcessor = queuedReadProcessor;
    }

    @Override
    public void process(List<LogbrokerPartitionRead> reads) {
        log.info(
                "Processing internal logbroker reads, reads: {}, cookies: [{}]",
                reads.size(),
                reads.stream()
                        .map(read -> String.valueOf(read.getCommitCountdown().getCookie()))
                        .collect(Collectors.joining(", "))
        );

        for (var read : reads) {
            Retryable.retryUntilInterruptedOrSucceeded(
                    () -> processInternal(read),
                    r -> this.statistics.onReadFailed()
            );
        }
    }

    private void processInternal(LogbrokerPartitionRead read) {
        var messages = read.getBatches().stream()
                .flatMap(x -> x.getMessageData().stream())
                .map(data -> parseWithRetries(read.getPartition(), data))
                .filter(Optional::isPresent)
                .map(optional -> optional.orElseThrow(RuntimeException::new))
                .flatMap(x -> {
                    var messageList = x.getMessagesList().stream()
                            .filter(this::isCheckPresent)
                            .collect(Collectors.toList());

                    var messagesLeft = new AtomicInteger(messageList.size());
                    if (messagesLeft.get() == 0) {
                        // If InternalMessages is empty, create MESSAGE_NOT_SET just to notify read
                        return Stream.of(new ReadInternalStreamMessage(
                                new AtomicInteger(1), read, Internal.InternalMessage.getDefaultInstance()
                        ));
                    }

                    return messageList.stream()
                            .map(m -> new ReadInternalStreamMessage(messagesLeft, read, m));
                })
                .collect(Collectors.toList());

        if (messages.isEmpty()) {
            log.info("All messages skipped with parse errors");
            read.notifyProcessed();
            return;
        }

        LogbrokerLogger.logReceived(messages, m -> m.getMessage().getMeta());

        log.info(
                "Processing internal logbroker read, " +
                        "cookie: {}, partition: {}, lb messages: {}, messages: {}",
                read.getCommitCountdown().getCookie(),
                read.getPartition(),
                read.getNumberOfMessages(),
                messages.size()
        );

        for (var readMessage : messages) {
            if (messageCasesToProcess.contains(readMessage.getMessage().getMessagesCase())) {
                queuedReadProcessor.enqueue(getQueueNumber(readMessage.getCheckId()), readMessage);
            } else {
                readMessage.notifyMessageProcessed(1);
            }
        }

        log.info("All messages enqueued to processing");
    }

    @Override
    public void onLock(ConsumerLockMessage lock) {
        log.info("Locking internal stream partition: {}, for topic: {}", lock.getPartition(), lock.getTopic());
        cleanPartitionEntitiesCache(lock.getPartition());
    }

    @Override
    public void onRelease(ConsumerReleaseMessage release) {
        log.info("Unlocking internal stream partition: {}, for topic: {}", release.getPartition(), release.getTopic());
        cleanPartitionEntitiesCache(release.getPartition());
    }

    private void cleanPartitionEntitiesCache(int partition) {
        var loadedChecks = loadedEntitiesCache.getChecks().getIfPresent(partition);
        var loadedIterations = loadedEntitiesCache.getIterations().getIfPresent(partition);

        cache.modify(c -> {
            if (loadedChecks != null) {
                loadedChecks.forEach(id -> {
                    c.iterationsGrouped().invalidate(id);
                    c.checks().invalidate(id);
                });
            }

            if (loadedIterations != null) {
                loadedIterations.forEach(id -> c.tasksGrouped().invalidate(id));
            }

            c.commit();
        });

        loadedEntitiesCache.getChecks().invalidate(partition);
        loadedEntitiesCache.getIterations().invalidate(partition);
    }

    private boolean isCheckPresent(Internal.InternalMessage message) {
        var check = entitiesChecker.checkExistingCheckWithRegistrationLag(message.getCheckId(), false);

        if (check.isEmpty()) {
            log.warn("Check {} from internal stream skipped in read processor", message.getCheckId());
            return false;
        }

        return true;
    }

    private Optional<Internal.InternalMessages> parseWithRetries(int partition, MessageData messageData) {
        var retryDurationSeconds = Retryable.START_TIMEOUT_SECONDS;
        var retryDurationSecondsMax = Retryable.MAX_TIMEOUT_SECONDS;

        while (!Thread.currentThread().isInterrupted()) {
            try {
                return Optional.of(parseMessage(partition, messageData));
            } catch (RuntimeException | IOException e) {
                log.error("Failed to process message", e);
                this.statistics.onParseError();

                //TODO Add option for skipping parse errors

                Uninterruptibles.sleepUninterruptibly(retryDurationSeconds, TimeUnit.SECONDS);
                retryDurationSeconds = Math.min(retryDurationSecondsMax, retryDurationSeconds * 2);
            }
        }

        return Optional.empty();
    }

    private int getQueueNumber(CheckEntity.Id checkId) {
        return (checkId.hashCode() & 0xfffffff) % queuedReadProcessor.getQueueMaxNumber();
    }

    private Internal.InternalMessages parseMessage(int partition, MessageData data) throws IOException {
        var decompressed = data.getDecompressedData();
        this.statistics.onDataReceived(partition, 0, decompressed.length);
        return Internal.InternalMessages.parseFrom(decompressed);
    }
}
