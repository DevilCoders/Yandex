package ru.yandex.ci.storage.core.logbroker.badge_events;

import java.util.List;

import io.micrometer.core.instrument.MeterRegistry;

import ru.yandex.ci.storage.core.CiBadgeEvents;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactory;
import ru.yandex.ci.storage.core.logbroker.StorageMessageWriter;
import ru.yandex.ci.storage.core.message.InternalPartitionGenerator;

public class BadgeEventsSenderImpl extends StorageMessageWriter implements BadgeEventsSender {
    private final InternalPartitionGenerator partitionGenerator;

    public BadgeEventsSenderImpl(
            int numberOfPartitions,
            MeterRegistry meterRegistry,
            LogbrokerWriterFactory logbrokerWriterFactory
    ) {
        super(meterRegistry, numberOfPartitions, logbrokerWriterFactory);
        partitionGenerator = new InternalPartitionGenerator(numberOfPartitions);
    }

    @Override
    public void sendEvent(CiBadgeEvents.Event event) {
        writeWithRetries(List.of(
                new Message(createMeta(), 0, event, "Event: %s".formatted(event.getTypeCase().name())))
        );
    }

    @Override
    public void sendEvent(CheckEntity.Id checkId, CiBadgeEvents.Event event) {
        var partition = partitionGenerator.generatePartition(checkId);

        writeWithRetries(List.of(
                new Message(
                        createMeta(),
                        partition,
                        event,
                        "Event for %s: %s".formatted(checkId, event.getTypeCase().name())
                ))
        );
    }
}
