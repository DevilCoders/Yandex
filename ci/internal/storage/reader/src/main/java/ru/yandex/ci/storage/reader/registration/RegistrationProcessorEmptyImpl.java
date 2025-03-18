package ru.yandex.ci.storage.reader.registration;

import java.time.Instant;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.storage.core.EventsStreamMessages;
import ru.yandex.ci.storage.core.registration.RegistrationProcessor;

public class RegistrationProcessorEmptyImpl implements RegistrationProcessor {
    private static final Logger log = LoggerFactory.getLogger(RegistrationProcessorEmptyImpl.class);

    @Override
    public void processMessage(EventsStreamMessages.RegistrationMessage message, Instant timestamp) {
        if (message.hasTask()) {
            log.info(
                    "Receive registration message for iteration {}, task {}",
                    message.getIteration().getId(), message.getTask().getId()
            );
        } else {
            log.info("Receive registration message for iteration {}", message.getIteration().getId());
        }
    }
}
