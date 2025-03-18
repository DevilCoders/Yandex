package ru.yandex.ci.observer.reader.registration;

import java.util.List;

import javax.annotation.Nullable;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTaskEntity;
import ru.yandex.ci.observer.reader.ObserverReaderYdbTestBase;
import ru.yandex.ci.observer.reader.proto.ObserverProtoMappers;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.EventsStreamMessages;

class RegistrationProcessorImplTest extends ObserverReaderYdbTestBase {
    private ObserverRegistrationProcessor processor;

    @BeforeEach
    void setup() {
        this.processor = new RegistrationProcessorImpl(db, clock);
    }

    @Test
    void processMessages_RegisterIteration() {
        var iteration = SAMPLE_ITERATION.withStatus(Common.CheckStatus.CREATED);
        var message = createRegistrationMessage(iteration, null);
        var time = ProtoConverter.convert(TIME);

        processor.processMessages(
                List.of(
                        EventsStreamMessages.EventsStreamMessage.newBuilder()
                                .setMeta(Common.MessageMeta.newBuilder().setTimestamp(time).build())
                                .setRegistration(message)
                                .build()
                )
        );

        db.readOnly(() -> assertEqualsCheckAndIterationInTx(iteration));
    }

    @Test
    void processMessages_RegisterTask() {
        var iteration = SAMPLE_ITERATION.withStatus(Common.CheckStatus.CREATED);
        var message = createRegistrationMessage(iteration, SAMPLE_TASK);
        var time = ProtoConverter.convert(TIME);

        processor.processMessages(
                List.of(
                        EventsStreamMessages.EventsStreamMessage.newBuilder()
                                .setMeta(Common.MessageMeta.newBuilder().setTimestamp(time).build())
                                .setRegistration(message)
                                .build()
                )
        );

        db.readOnly(() -> {
            assertEqualsCheckAndIterationInTx(iteration);
            assertEqualsTaskInTx(SAMPLE_TASK);
        });
    }

    @Test
    void processMessages_RegisterMultipleTasks() {
        var iteration = SAMPLE_ITERATION.withStatus(Common.CheckStatus.CREATED);
        var message = createRegistrationMessage(iteration, SAMPLE_TASK);
        var message2 = createRegistrationMessage(iteration, SAMPLE_TASK_2);
        var time = ProtoConverter.convert(TIME);

        processor.processMessages(
                List.of(
                        EventsStreamMessages.EventsStreamMessage.newBuilder()
                                .setMeta(Common.MessageMeta.newBuilder().setTimestamp(time).build())
                                .setRegistration(message)
                                .build()
                )
        );
        processor.processMessages(
                List.of(
                        EventsStreamMessages.EventsStreamMessage.newBuilder()
                                .setMeta(Common.MessageMeta.newBuilder().setTimestamp(time).build())
                                .setRegistration(message2)
                                .build()
                )
        );

        db.readOnly(() -> {
            assertEqualsCheckAndIterationInTx(iteration);
            assertEqualsTaskInTx(SAMPLE_TASK);
            assertEqualsTaskInTx(SAMPLE_TASK_2);
        });
    }

    private void assertEqualsCheckAndIterationInTx(CheckIterationEntity iteration) {
        var actualCheck = db.checks().get(SAMPLE_CHECK_ID);
        var actualIteration = db.iterations().get(iteration.getId());

        Assertions.assertEquals(SAMPLE_CHECK, actualCheck);
        Assertions.assertEquals(iteration, actualIteration);
    }

    private void assertEqualsTaskInTx(CheckTaskEntity expectedTask) {
        var actualTask = db.tasks().get(expectedTask.getId());

        Assertions.assertEquals(expectedTask, actualTask);
    }

    private EventsStreamMessages.RegistrationMessage createRegistrationMessage(
            CheckIterationEntity iteration, @Nullable CheckTaskEntity task
    ) {
        var builder = EventsStreamMessages.RegistrationMessage.newBuilder()
                .setCheck(ObserverProtoMappers.toProtoCheck(SAMPLE_CHECK))
                .setIteration(ObserverProtoMappers.toProtoIteration(iteration));

        if (task != null) {
            builder.setTask(ObserverProtoMappers.toProtoTask(task));
        }

        return builder.build();
    }
}
