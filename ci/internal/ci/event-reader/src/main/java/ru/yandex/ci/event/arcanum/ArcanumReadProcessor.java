package ru.yandex.ci.event.arcanum;

import java.time.Instant;
import java.util.List;

import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.TextFormat;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.arcanum.event.ArcanumEvents;
import ru.yandex.ci.logbroker.LogbrokerPartitionRead;
import ru.yandex.ci.logbroker.LogbrokerReadProcessor;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerLockMessage;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerReleaseMessage;

public class ArcanumReadProcessor implements LogbrokerReadProcessor {
    private static final Logger log = LoggerFactory.getLogger(ArcanumReadProcessor.class);

    private final ArcanumEventService arcanumEventService;

    public ArcanumReadProcessor(ArcanumEventService arcanumEventService) {
        this.arcanumEventService = arcanumEventService;
    }

    public void onRead(LogbrokerPartitionRead read) {
        var batches = read.getBatches();
        log.info("Read {} batches. Commit cookie: {}", batches.size(), read.getCommitCountdown().getCookie());

        for (var batch : batches) {
            for (var data : batch.getMessageData()) {
                try {
                    var eventCreated = Instant.ofEpochMilli(data.getMessageMeta().getCreateTimeMs());
                    var event = parseEvent(data.getDecompressedData());

                    log.info(
                            "Processing {} event from Arcanum: {}",
                            event.getTypeCase(), TextFormat.shortDebugString(event)
                    );

                    arcanumEventService.processEvent(event, eventCreated);
                } catch (Exception e) {
                    log.error("Exception during Arcanum event processing", e);
                    throw new RuntimeException(e);
                }
            }
        }
    }

    private ArcanumEvents.Event parseEvent(byte[] data) throws InvalidProtocolBufferException {
        return ArcanumEvents.Event.newBuilder().mergeFrom(data).build();
    }

    @Override
    public void process(List<LogbrokerPartitionRead> reads) {
        reads.forEach(this::onRead);
        reads.forEach(LogbrokerPartitionRead::notifyProcessed);
    }

    @Override
    public void onLock(ConsumerLockMessage lock) {
        log.info("Locking arcanum stream partition: {}, for topic: {}", lock.getPartition(), lock.getTopic());
    }

    @Override
    public void onRelease(ConsumerReleaseMessage release) {
        log.info("Unlocking arcanum stream partition: {}, for topic: {}", release.getPartition(), release.getTopic());
    }
}
