package ru.yandex.ci.observer.reader.message.internal;

import java.util.concurrent.atomic.AtomicInteger;

import lombok.Getter;
import lombok.ToString;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.logbroker.LogbrokerPartitionRead;
import ru.yandex.ci.observer.core.Internal;
import ru.yandex.ci.observer.core.db.model.check.CheckEntity;

@Getter
@ToString
@Slf4j
public class ReadInternalStreamMessage {
    private final AtomicInteger messagesLeft;
    private final LogbrokerPartitionRead read;
    @Getter
    private final Internal.InternalMessage message;
    @Getter
    private final CheckEntity.Id checkId;

    public ReadInternalStreamMessage(AtomicInteger messagesLeft,
                                     LogbrokerPartitionRead read,
                                     Internal.InternalMessage message) {
        this.messagesLeft = messagesLeft;
        this.read = read;
        this.message = message;
        this.checkId = CheckEntity.Id.of(message.getCheckId());
    }

    public void notifyMessageProcessed(int size) {
        if (size == 0) {
            return;
        }

        int left = messagesLeft.addAndGet(-size);
        long cookie = read.getCommitCountdown().getCookie();

        if (left < 0) {
            messagesLeft.addAndGet(size);
            throw new RuntimeException(
                    ("Processed more InternalMessage then have in InternalMessages, " +
                            "read cookie: %d, left: %d, required: %d").formatted(cookie, left + size, size)
            );
        } else if (left == 0) {
            log.debug("0 messages left in InternalMessages, will notify read, cookie: {}", cookie);
            read.getCommitCountdown().notifyMessageProcessed(1);
        } else {
            log.debug("{} messages left in InternalMessages, read cookie: {}", left, cookie);
        }
    }
}
