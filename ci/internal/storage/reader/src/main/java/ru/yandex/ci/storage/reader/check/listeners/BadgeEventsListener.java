package ru.yandex.ci.storage.reader.check.listeners;

import java.util.Set;

import lombok.RequiredArgsConstructor;

import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.logbroker.badge_events.BadgeEventsProducer;
import ru.yandex.ci.storage.reader.cache.ReaderCache;
import ru.yandex.ci.storage.reader.check.CheckEventsListener;
import ru.yandex.ci.storage.reader.check.ReaderCheckService;
import ru.yandex.lang.NonNullApi;

@RequiredArgsConstructor
@NonNullApi
public class BadgeEventsListener implements CheckEventsListener {

    private final BadgeEventsProducer eventsProducer;

    @Override
    public void onCheckCompleted(
            ReaderCheckService checkService,
            ReaderCache.Modifiable cache,
            CheckEntity.Id checkId,
            Set<CheckIterationEntity.Id> checkIterations
    ) {
        eventsProducer.onCheckFinished(cache.checks().getFreshOrThrow(checkId));
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
        eventsProducer.onCheckFinished(cache.checks().getFreshOrThrow(checkId));
    }

    @Override
    public void onCheckCancelled(
            ReaderCheckService checkService,
            ReaderCache.Modifiable cache,
            CheckEntity.Id checkId,
            Set<CheckIterationEntity.Id> iterationIds
    ) {
        eventsProducer.onCheckFinished(cache.checks().getFreshOrThrow(checkId));
    }

    @Override
    public void onIterationTypeCompleted(ReaderCheckService checkService, ReaderCache.Modifiable cache,
                                         CheckIterationEntity.Id iterationId) {
        var iteration = cache.iterations().getFreshOrThrow(iterationId);
        var check = cache.checks().getFreshOrThrow(iterationId.getCheckId());
        eventsProducer.onIterationTypeFinished(
                check,
                iteration
        );
    }
}
