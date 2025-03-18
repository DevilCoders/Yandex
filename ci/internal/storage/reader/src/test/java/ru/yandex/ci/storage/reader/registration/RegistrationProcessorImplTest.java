package ru.yandex.ci.storage.reader.registration;

import java.time.Instant;
import java.util.stream.Collectors;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.EventsStreamMessages;
import ru.yandex.ci.storage.core.check.RequirementsService;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_id_generator.CheckIdGenerator;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.message.events.EventsStreamStatistics;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.core.registration.RegistrationProcessor;
import ru.yandex.ci.storage.core.sharding.ShardingSettings;
import ru.yandex.ci.storage.reader.StorageReaderYdbTestBase;
import ru.yandex.ci.storage.reader.check.ReaderCheckService;
import ru.yandex.ci.storage.reader.check.listeners.ArcanumCheckEventsListener;
import ru.yandex.ci.storage.reader.message.main.MainStreamStatistics;
import ru.yandex.ci.storage.reader.message.main.ReaderStatistics;
import ru.yandex.ci.storage.reader.message.shard.ShardOutStreamStatistics;
import ru.yandex.ci.storage.reader.message.writer.ShardInMessageWriter;
import ru.yandex.ci.storage.reader.other.MetricAggregationService;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.mock;

class RegistrationProcessorImplTest extends StorageReaderYdbTestBase {

    @Autowired
    ArcanumCheckEventsListener arcanumCheckEventsListener;

    @Autowired
    ShardInMessageWriter shardInMessageWriter;

    ReaderCheckService checkService;

    @BeforeEach
    void startup() {
        var readerStatistics = new ReaderStatistics(
                mock(MainStreamStatistics.class),
                mock(ShardOutStreamStatistics.class),
                mock(EventsStreamStatistics.class),
                meterRegistry
        );

        var requirementsService = mock(RequirementsService.class);
        checkService = new ReaderCheckService(
                requirementsService, readerCache, readerStatistics, db, badgeEventsProducer,
                mock(MetricAggregationService.class)
        );
    }

    @Test
    void registerCheckNoSampling() {
        var shardingSettings = ShardingSettings.DEFAULT;
        RegistrationProcessor processor = new RegistrationProcessorImpl(
                db, readerCache, checkService, 1, shardingSettings
        );

        var checkIds = CheckIdGenerator.generate(1L, 100);
        checkIds.forEach(id -> {
            var registrationMessage = generateRegistrationMessage(id);
            processor.processMessage(registrationMessage, Instant.now());
        });

        db.currentOrReadOnly(() -> {
            var all = db.checks().readTable().collect(Collectors.toList());
            assertThat(all).hasSize(100);
            assertThat(all.get(0).getId().getId()).isEqualTo(checkIds.get(0));
            assertThat(all.get(1).getId().getId()).isEqualTo(checkIds.get(1));
            assertThat(all.get(2).getId().getId()).isEqualTo(checkIds.get(2));
            assertThat(all.get(3).getId().getId()).isEqualTo(checkIds.get(3));
            assertThat(all.get(4).getId().getId()).isEqualTo(checkIds.get(4));
        });
    }

    @Test
    void registerCheckSampling() {
        var shardingSettings = new ShardingSettings(1, 1, 1, 1, 1, 1, 1, 1, 0, 4);
        RegistrationProcessor processor = new RegistrationProcessorImpl(
                db, readerCache, checkService, 1, shardingSettings
        );

        var checkIds = CheckIdGenerator.generate(1L, 100);
        checkIds.forEach(id -> {
            var registrationMessage = generateRegistrationMessage(id);
            processor.processMessage(registrationMessage, Instant.now());
        });

        db.currentOrReadOnly(() -> {
            var all = db.checks().readTable().collect(Collectors.toList());
            assertThat(all).hasSize(20);
            assertThat(all.get(0).getId().getId()).isEqualTo(checkIds.get(0));
            assertThat(all.get(1).getId().getId()).isEqualTo(checkIds.get(5));
            assertThat(all.get(2).getId().getId()).isEqualTo(checkIds.get(10));
            assertThat(all.get(3).getId().getId()).isEqualTo(checkIds.get(15));
            assertThat(all.get(4).getId().getId()).isEqualTo(checkIds.get(20));
        });
    }

    private EventsStreamMessages.RegistrationMessage generateRegistrationMessage(long checkId) {
        var check = sampleCheck.toBuilder()
                .id(CheckEntity.Id.of(checkId))
                .build();
        var iteration = sampleIteration.toBuilder()
                .id(CheckIterationEntity.Id.of(check.getId(), CheckIteration.IterationType.FAST, 1))
                .build();

        return EventsStreamMessages.RegistrationMessage.newBuilder()
                .setCheck(CheckProtoMappers.toProtoCheck(check))
                .setIteration(CheckProtoMappers.toProtoIteration(iteration))
                .build();
    }
}
