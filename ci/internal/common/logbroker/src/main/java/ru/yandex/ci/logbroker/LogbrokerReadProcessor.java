package ru.yandex.ci.logbroker;

import java.util.List;

import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerLockMessage;
import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.ConsumerReleaseMessage;

public interface LogbrokerReadProcessor {
    void process(List<LogbrokerPartitionRead> reads);

    void onLock(ConsumerLockMessage lock);

    void onRelease(ConsumerReleaseMessage release);
}
