package ru.yandex.ci.storage.core.registration;

import java.time.Instant;

import ru.yandex.ci.storage.core.EventsStreamMessages;

public interface RegistrationProcessor {
    void processMessage(EventsStreamMessages.RegistrationMessage message, Instant timestamp);
}
