package ru.yandex.ci.observer.reader.registration;

import java.util.List;

import ru.yandex.ci.storage.core.EventsStreamMessages;

public interface ObserverRegistrationProcessor {
    void processMessages(List<EventsStreamMessages.EventsStreamMessage> registrationMessages);
}
