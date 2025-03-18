package ru.yandex.ci.observer.reader.registration;

import java.time.Clock;
import java.time.Duration;
import java.time.Instant;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

import javax.annotation.Nonnull;

import com.google.common.collect.Maps;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.observer.core.db.CiObserverDb;
import ru.yandex.ci.observer.core.db.model.check.CheckEntity;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTaskEntity;
import ru.yandex.ci.observer.reader.proto.ObserverProtoMappers;
import ru.yandex.ci.storage.core.EventsStreamMessages;

@Slf4j
@RequiredArgsConstructor
public class RegistrationProcessorImpl implements ObserverRegistrationProcessor {

    @Nonnull
    private final CiObserverDb db;
    @Nonnull
    private final Clock clock;

    @Override
    public void processMessages(List<EventsStreamMessages.EventsStreamMessage> registrationMessages) {
        if (registrationMessages.isEmpty()) {
            return;
        }

        Map<CheckEntity.Id, CheckEntity> checks = new HashMap<>();
        Map<CheckIterationEntity.Id, CheckIterationEntity> iterations = new HashMap<>();
        Map<CheckTaskEntity.Id, CheckTaskEntity> tasks = new HashMap<>();
        Instant minTime = null;
        CheckEntity.Id minTimeCheckId = null;

        var now = clock.instant();
        for (var m : registrationMessages) {
            var registrationMessage = m.getRegistration();
            var check = ObserverProtoMappers.toCheck(registrationMessage.getCheck());
            var iteration = ObserverProtoMappers.toIteration(
                    registrationMessage.getCheck(),
                    registrationMessage.getIteration()
            );

            if (iteration.getStressTest()) {
                iteration = iteration.toBuilder()
                        .diffSetEventCreated(now)
                        .build();
            }

            checks.put(check.getId(), check);
            iterations.put(iteration.getId(), iteration);
            if (registrationMessage.hasTask()) {
                var task = ObserverProtoMappers.toTask(
                        registrationMessage.getTask(),
                        iteration.getCheckType(),
                        check.getRight()
                );
                tasks.put(task.getId(), task);
            }

            var messageTime = ProtoConverter.convert(m.getMeta().getTimestamp());
            if (minTime == null || minTime.isAfter(messageTime)) {
                minTime = messageTime;
                minTimeCheckId = check.getId();
            }
        }

        log.info(
                "Receive min time registration message for check {}, lag {}",
                minTimeCheckId, Duration.between(now, minTime).getSeconds()
        );

        log.info(
                "Registering checks {}, iterations {}, tasks {}", checks.keySet(), iterations.keySet(), tasks.keySet()
        );

        registerInTx(checks, iterations, tasks);
    }

    private void registerInTx(
            Map<CheckEntity.Id, CheckEntity> checks,
            Map<CheckIterationEntity.Id, CheckIterationEntity> iterations,
            Map<CheckTaskEntity.Id, CheckTaskEntity> tasks
    ) {
        this.db.currentOrTx(() -> {
            var existedCheckIds = Set.copyOf(db.checks().findIds(checks.keySet()));
            var checksToRegister = Maps.filterKeys(checks, id -> !existedCheckIds.contains(id));
            log.info("Checks {} already registered", existedCheckIds);

            var existedIterationIds = Set.copyOf(db.iterations().findIds(iterations.keySet()));
            var iterationsToRegister = Maps.filterKeys(iterations, id -> !existedIterationIds.contains(id));
            log.info("Iterations {} already registered", existedIterationIds);

            var existedTaskIds = Set.copyOf(db.tasks().findIds(tasks.keySet()));
            var tasksToRegister = Maps.filterKeys(tasks, id -> !existedTaskIds.contains(id));
            log.info("Tasks {} already registered", existedTaskIds);

            db.checks().save(checksToRegister.values());
            log.info("Checks {} registered", checksToRegister.keySet());

            db.iterations().save(iterationsToRegister.values());
            log.info("Iterations {} registered", iterationsToRegister.keySet());

            db.tasks().save(tasksToRegister.values());
            log.info("Tasks {} registered", tasksToRegister.keySet());
        });
    }
}
