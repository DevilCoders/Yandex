package ru.yandex.ci.observer.tests.logbroker;

import java.time.Clock;

import ru.yandex.ci.observer.reader.message.internal.ObserverInternalStreamReadProcessor;
import ru.yandex.ci.storage.tests.logbroker.TestLogbrokerService;

public class TestCommonLogbrokerService extends TestLogbrokerService {

    public TestCommonLogbrokerService(Clock clock) {
        super(clock);
    }

    public void registerObserverInternalConsumer(ObserverInternalStreamReadProcessor readProcessor) {
        registerConsumer(ObserverTopic.INTERNAL, readProcessor);
    }

    @SuppressWarnings("ShortCircuitBoolean")
    @Override
    protected boolean deliverAllMessagesInternal() {
        var hasNewMessages = super.deliverAllMessagesInternal();

        /* use not lazy '|' instead of lazy `||` to reduce error possibility
            of swapping `hasNewMessages || deliverShardInMessages()` */
        hasNewMessages = deliverObserverInternalStreamMessages() | hasNewMessages;

        return hasNewMessages;
    }

    public boolean deliverObserverInternalStreamMessages() {
        return deliverStreamMessages(ObserverTopic.INTERNAL);
    }

}
