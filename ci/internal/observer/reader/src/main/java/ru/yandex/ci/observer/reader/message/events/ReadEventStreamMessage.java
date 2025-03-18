package ru.yandex.ci.observer.reader.message.events;

import lombok.Value;

import ru.yandex.ci.logbroker.LogbrokerPartitionRead;
import ru.yandex.ci.storage.core.EventsStreamMessages;

@Value
public class ReadEventStreamMessage {
    LogbrokerPartitionRead read;
    EventsStreamMessages.EventsStreamMessage message;
}
