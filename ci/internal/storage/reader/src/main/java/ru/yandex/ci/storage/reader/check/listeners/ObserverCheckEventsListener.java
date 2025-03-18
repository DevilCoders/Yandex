package ru.yandex.ci.storage.reader.check.listeners;

import java.util.Set;

import javax.annotation.Nonnull;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.logbroker.event_producer.StorageEventsProducer;
import ru.yandex.ci.storage.reader.cache.ReaderCache;
import ru.yandex.ci.storage.reader.check.CheckEventsListener;
import ru.yandex.ci.storage.reader.check.ReaderCheckService;

@Slf4j
public class ObserverCheckEventsListener implements CheckEventsListener {
    @Nonnull
    private final StorageEventsProducer eventsProducer;

    public ObserverCheckEventsListener(@Nonnull StorageEventsProducer eventsProducer) {
        this.eventsProducer = eventsProducer;
    }

    @Override
    public void onIterationCompleted(
            ReaderCheckService checkService, ReaderCache.Modifiable cache, CheckIterationEntity.Id iterationId
    ) {
        log.info("onIterationCompleted event for iteration {}", iterationId);
        eventsProducer.onIterationFinished(
                iterationId,
                cache.iterations().get(iterationId).orElseThrow().getStatus()
        );
    }

    @Override
    public void onTaskCompleted(
            ReaderCheckService checkService, ReaderCache.Modifiable cache, CheckTaskEntity.Id taskId
    ) {
        log.info("onTaskCompleted event for task {}", taskId);
        eventsProducer.onTaskFinished(taskId);
    }

    @Override
    public void onCheckFatalError(
            ReaderCheckService checkService,
            ReaderCache.Modifiable cache,
            CheckEntity.Id checkId,
            Set<CheckIterationEntity.Id> iterationIds,
            CheckIterationEntity.Id brokenIterationId,
            Set<CheckIterationEntity.Id> runningIterationsIds
    ) {
        var runningIterations = runningIterationsIds.stream()
                .filter(id -> !id.equals(brokenIterationId))
                .map(id -> cache.iterations().get(id).orElseThrow())
                .toList();

        var brokenIteration = cache.iterations().get(brokenIterationId).orElseThrow();

        eventsProducer.onCheckFatalError(checkId, runningIterations, brokenIteration);
    }
}
