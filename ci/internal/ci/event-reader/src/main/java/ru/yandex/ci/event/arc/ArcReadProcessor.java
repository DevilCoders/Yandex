package ru.yandex.ci.event.arc;

import java.util.List;

import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.TextFormat;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.arc.api.Message;
import ru.yandex.ci.logbroker.LogbrokerPartitionRead;
import ru.yandex.ci.logbroker.LogbrokerReadProcessor;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerLockMessage;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerReleaseMessage;

public class ArcReadProcessor implements LogbrokerReadProcessor {
    private static final Logger log = LoggerFactory.getLogger(ArcReadProcessor.class);

    private final ArcEventService arcEventService;

    public ArcReadProcessor(ArcEventService arcEventService) {
        this.arcEventService = arcEventService;
    }

    public void onRead(LogbrokerPartitionRead read) {
        var batches = read.getBatches();
        log.info("Read {} batches. Commit cookie: {}", batches.size(), read.getCommitCountdown().getCookie());

        for (var batch : batches) {
            for (var data : batch.getMessageData()) {
                try {
                    var reflogRecord = parseReflogEvent(data.getDecompressedData());
                    log.info("Got arc event {}", TextFormat.shortDebugString(reflogRecord));
                    try {
                        arcEventService.processEvent(reflogRecord);
                    } catch (Exception e) {
                        log.error(
                                "Got error while processing reflog record: {}.",
                                TextFormat.shortDebugString(reflogRecord),
                                e
                        );
                        throw e;
                    }
                } catch (InvalidProtocolBufferException e) {
                    log.error("Exception during Arc reflog event processing", e);
                    throw new RuntimeException(e);
                }
            }
        }
    }

    private Message.ReflogRecord parseReflogEvent(byte[] data) throws InvalidProtocolBufferException {
        return Message.ReflogRecord.parseFrom(data);
    }

    @Override
    public void process(List<LogbrokerPartitionRead> reads) {
        reads.forEach(this::onRead);
        reads.forEach(LogbrokerPartitionRead::notifyProcessed);
    }

    @Override
    public void onLock(ConsumerLockMessage lock) {
        log.info("Locking arc stream partition: {}, for topic: {}", lock.getPartition(), lock.getTopic());
    }

    @Override
    public void onRelease(ConsumerReleaseMessage release) {
        log.info("Unlocking arc stream partition: {}, for topic: {}", release.getPartition(), release.getTopic());
    }
}
