package ru.yandex.ci.storage.core.message.main;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.Base64;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;
import java.util.zip.InflaterInputStream;

import com.google.common.util.concurrent.Uninterruptibles;
import lombok.extern.slf4j.Slf4j;
import org.springframework.util.FastByteArrayOutputStream;
import org.springframework.util.StreamUtils;

import ru.yandex.ci.logbroker.LogbrokerPartitionRead;
import ru.yandex.ci.logbroker.LogbrokerReadProcessor;
import ru.yandex.ci.storage.core.MainStreamMessages;
import ru.yandex.ci.storage.core.logbroker.LogbrokerLogger;
import ru.yandex.ci.storage.core.utils.NumberOfMessagesFormatter;
import ru.yandex.ci.storage.core.utils.TimeTraceService;
import ru.yandex.ci.util.Retryable;
import ru.yandex.kikimr.persqueue.compression.CompressionCodec;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerLockMessage;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerReleaseMessage;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.data.MessageData;

@Slf4j
public abstract class MainStreamReadProcessor implements LogbrokerReadProcessor {
    private final TimeTraceService timeTraceService;
    private final MainStreamMessageProcessor messageProcessor;
    private final MainStreamStatistics statistics;

    protected MainStreamReadProcessor(
            TimeTraceService timeTraceService,
            MainStreamMessageProcessor messageProcessor,
            MainStreamStatistics statistics
    ) {
        this.timeTraceService = timeTraceService;
        this.messageProcessor = messageProcessor;
        this.statistics = statistics;
    }

    protected abstract boolean skipParseErrors();

    @Override
    public void process(List<LogbrokerPartitionRead> reads) {
        var trace = timeTraceService.createTrace("reader_timings");

        var parsedMessages = reads.stream().flatMap(read -> read.getBatches().stream())
                .flatMap(
                        x -> x.getMessageData().stream()
                                .map(messageData -> parseWithRetries(x.getPartition(), messageData))
                )
                .filter(Optional::isPresent)
                .map(optional -> optional.orElseThrow(RuntimeException::new))
                .collect(Collectors.toList());

        LogbrokerLogger.logReceived(
                parsedMessages,
                MainStreamMessages.MainStreamMessage::getMeta
        );

        trace.step("parsed");

        if (parsedMessages.isEmpty()) {
            log.info("All messages skipped with parse errors");
            reads.forEach(LogbrokerPartitionRead::notifyProcessed);
            return;
        }

        var messagesByCase = parsedMessages.stream().collect(
                Collectors.groupingBy(MainStreamMessages.MainStreamMessage::getMessagesCase)
        );

        log.info(
                "Processing logbroker reads:, " +
                        "reads: {} partitions: {}, lb messages: {}, messages: {}, by type: [{}], cookies: [{}], " +
                        "source ids: [{}]",
                reads.size(),
                reads.stream().map(read -> String.valueOf(read.getPartition())).collect(Collectors.joining(", ")),
                reads.stream().mapToInt(LogbrokerPartitionRead::getNumberOfMessages).sum(),
                parsedMessages.size(),
                NumberOfMessagesFormatter.format(messagesByCase),
                reads.stream()
                        .map(read -> String.valueOf(read.getCommitCountdown().getCookie()))
                        .collect(Collectors.joining(", ")),
                reads.stream()
                        .flatMap(read -> read.getBatches().stream().flatMap(x -> x.getMessageData().stream()))
                        .map(x -> new String(x.getMessageMeta().getSourceId(), StandardCharsets.UTF_8))
                        .distinct()
                        .collect(Collectors.joining(", "))
        );

        process(messagesByCase, trace);

        reads.forEach(LogbrokerPartitionRead::notifyProcessed);
    }

    @Override
    public void onLock(ConsumerLockMessage lock) {
        log.info("Locking main stream partition: {}, for topic: {}", lock.getPartition(), lock.getTopic());
    }

    @Override
    public void onRelease(ConsumerReleaseMessage release) {
        log.info("Unlocking main stream partition: {}, for topic: {}", release.getPartition(), release.getTopic());
    }

    private void process(
            Map<MainStreamMessages.MainStreamMessage.MessagesCase, List<MainStreamMessages.MainStreamMessage>> messages,
            TimeTraceService.Trace trace
    ) {
        var taskMessages = messages.getOrDefault(
                MainStreamMessages.MainStreamMessage.MessagesCase.TASK_MESSAGE, List.of()
        );

        messageProcessor.process(
                taskMessages.stream()
                        .map(MainStreamMessages.MainStreamMessage::getTaskMessage)
                        .collect(Collectors.toList()),
                trace
        );

        trace.step("processed");
    }

    private Optional<MainStreamMessages.MainStreamMessage> parseWithRetries(
            int partition,
            MessageData messageData
    ) {
        byte[] decompressed;

        try {
            decompressed = decompressMessage(messageData);
        } catch (Exception e) {
            throw new RuntimeException("Failed to decompress", e);
        }

        this.statistics.onDataReceived(partition, messageData.getRawData().length, decompressed.length);

        while (!Thread.currentThread().isInterrupted()) {
            try {
                return Optional.of(MainStreamMessages.MainStreamMessage.parseFrom(decompressed));
            } catch (RuntimeException | IOException e) {
                log.error(
                        "Failed to parse message, base64:\n{}", Base64.getEncoder().encodeToString(decompressed), e
                );

                this.statistics.onParseError();

                if (skipParseErrors()) {
                    log.info("Skipping parse error");
                    return Optional.empty();
                }

                // Long sleep as retry probably won't help.
                Uninterruptibles.sleepUninterruptibly(Retryable.MAX_TIMEOUT_SECONDS, TimeUnit.SECONDS);
            }
        }

        return Optional.empty();
    }

    private byte[] decompressMessage(MessageData data) throws IOException {
        byte[] decompressed;
        if (data.getMessageMeta().getCodec() == CompressionCodec.GZIP) {
            decompressed = decompressGzip(data.getRawData());
        } else {
            decompressed = data.getDecompressedData();
        }
        return decompressed;
    }

    private byte[] decompressGzip(byte[] input) throws IOException {
        try (var inflaterInputStream = new InflaterInputStream(new ByteArrayInputStream(input))) {
            var output = new FastByteArrayOutputStream();
            StreamUtils.copy(inflaterInputStream, output);
            return output.getInputStream().readAllBytes();
        }
    }
}
