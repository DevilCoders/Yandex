package ru.yandex.ci.logbroker;

import java.time.Clock;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import com.google.common.collect.Multimaps;
import com.google.common.util.concurrent.ThreadFactoryBuilder;
import com.google.common.util.concurrent.Uninterruptibles;
import io.micrometer.core.instrument.Gauge;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.util.Retryable;
import ru.yandex.kikimr.persqueue.consumer.StreamListener;
import ru.yandex.kikimr.persqueue.consumer.transport.message.CommitMessage;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerInitResponse;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerLockMessage;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerReadResponse;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerReleaseMessage;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.data.MessageBatch;

@Slf4j
public class LogbrokerStreamListener implements StreamListener {
    private final LogbrokerStatistics statistics;
    private final ExecutorService executorService;
    private final LogbrokerReadProcessor readProcessor;
    private final Clock clock;

    private final List<BlockingQueue<LogbrokerPartitionRead>> readQueues;
    private final Map<Long, LbCommitCountdown> notCommitedReads = new ConcurrentHashMap<>();

    public LogbrokerStreamListener(
            String metricsPrefix,
            String stream,
            LogbrokerReadProcessor readProcessor,
            LogbrokerStatistics statistics,
            Clock clock,
            int readQueues,
            int readQueueLimit,
            int readDrainLimit
    ) {
        this.statistics = statistics;
        this.readProcessor = readProcessor;
        this.clock = clock;

        this.executorService = statistics.monitor(
                Executors.newFixedThreadPool(
                        readQueues,
                        new ThreadFactoryBuilder().setNameFormat(stream + "_lb_stream_%d").build()
                ),
                stream + "_lb_stream_executor"
        );

        var queueSizeMetricName = metricsPrefix + stream + "_lb_stream_queue_size";

        this.readQueues = new ArrayList<>(readQueues);
        statistics.register(
                Gauge.builder(
                                queueSizeMetricName,
                                () -> this.readQueues.stream().mapToInt(Collection::size).sum()
                        )
                        .tag("queue_number", "all")
        );

        for (var i = 0; i < readQueues; i++) {
            var queue = new LinkedBlockingQueue<LogbrokerPartitionRead>(readQueueLimit);
            this.readQueues.add(queue);
            executorService.execute(
                    new LogbrokerReadWorker(
                            readProcessor, queue, readDrainLimit, this.statistics
                    )
            );

            statistics.register(
                    Gauge.builder(queueSizeMetricName, queue::size)
                            .tag("queue_number", String.valueOf(i))
            );
        }
    }

    @Override
    public void onInit(ConsumerInitResponse init) {
        log.info("On init, sessionId: {}", init.getSessionId());
    }

    @Override
    public void onCommit(CommitMessage commit) {
        var cookies = commit.getCookies();
        log.info("On commit, cookies: {}", cookies);
        this.statistics.onReadsCommited(cookies.size());
    }

    @Override
    public void onClose() {
        log.info("Closing");
        executorService.shutdown();
    }

    @Override
    public void onError(Throwable e) {
        log.error("On error", e);
    }

    @Override
    public void onRead(ConsumerReadResponse read, ReadResponder responder) {
        this.statistics.onReadReceived();

        while (!Thread.currentThread().isInterrupted()) {
            try {
                addToQueue(new LogbrokerRead(read, responder));
                return;
            } catch (Exception e) {
                log.error("Internal error while processing message from LB.", e);
                this.statistics.onReadFailed();
                Uninterruptibles.sleepUninterruptibly(1, TimeUnit.SECONDS);
            }
        }
    }

    @Override
    public void onLock(ConsumerLockMessage lock, LockResponder lockResponder) {
        log.info("Locking Logbroker partition: {}, for topic: {}", lock.getPartition(), lock.getTopic());
        readProcessor.onLock(lock);

        log.info("Logbroker partition locked: {}, for topic: {}", lock.getPartition(), lock.getTopic());
        statistics.onPartitionLocked(lock.getTopic());

        lockResponder.locked(lock.getReadOffset(), false);
    }

    @Override
    public void onRelease(ConsumerReleaseMessage release) {
        log.info("Unlocking Logbroker partition: {}, for topic: {}", release.getPartition(), release.getTopic());
        readProcessor.onRelease(release);

        log.info("Logbroker partition unlocked: {}, for topic: {}", release.getPartition(), release.getTopic());
        statistics.onPartitionUnlocked(release.getTopic());
    }

    public Collection<LbCommitCountdown> getNotCommitedReads() {
        return notCommitedReads.values();
    }

    private void addToQueue(LogbrokerRead read) {
        var batchesByPartition = Multimaps.index(read.getBatches(), MessageBatch::getPartition);

        int numberOfMessages = read.getBatches()
                .stream()
                .map(MessageBatch::getMessageData)
                .mapToInt(List::size)
                .sum();

        log.info(
                "Received messages from logbroker, messages: {}, batches: {}, partitions: [{}], read: {}",
                numberOfMessages,
                batchesByPartition.keySet().size(),
                batchesByPartition.keySet().stream().map(Object::toString).collect(Collectors.joining(", ")),
                read.getCookie()
        );

        var commitCountdown = new LbCommitCountdown(
                read.getCookie(),
                this.clock,
                read.getResponder(),
                numberOfMessages,
                this::onReadProcessed
        );

        notCommitedReads.put(read.getCookie(), commitCountdown);

        batchesByPartition.asMap().forEach(
                (key, value) -> addToQueue(new LogbrokerPartitionRead(key, commitCountdown, new ArrayList<>(value)))
        );
    }

    private void addToQueue(LogbrokerPartitionRead read) {
        try {
            readQueues.get(read.getPartition() % readQueues.size()).put(read);
        } catch (InterruptedException e) {
            throw new RuntimeException("Interrupted", e);
        }
    }

    private void onReadProcessed(ReadResponder responder, Long cookie) {
        notCommitedReads.remove(cookie);

        Retryable.retryUntilInterruptedOrSucceeded(
                () -> {
                    log.info("Trying to commit read, cookie: {}", cookie);
                    responder.commit();
                },
                e -> {
                    this.statistics.onCommitFailed();
                    log.error("Commit of cookie {} failed, will reboot, reason: {}", cookie, e.getMessage());
                    System.exit(1); // Probably disconnected from Logbroker, grep 'StreamConsumer was closed due'
                }
        );

        this.statistics.onReadProcessed();
    }
}
