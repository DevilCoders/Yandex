package ru.yandex.ci.storage.reader.message.events;

import lombok.Value;

import ru.yandex.ci.logbroker.LbCommitCountdown;
import ru.yandex.ci.storage.core.EventsStreamMessages;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;

@Value
public class EventMessage {
    CheckEntity.Id checkId;
    LbCommitCountdown commitCountdown;
    EventsStreamMessages.EventsStreamMessage message;
}
