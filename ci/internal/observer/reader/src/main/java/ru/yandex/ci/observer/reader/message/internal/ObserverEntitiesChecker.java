package ru.yandex.ci.observer.reader.message.internal;

import java.time.Duration;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.NoSuchElementException;
import java.util.Optional;
import java.util.function.BiConsumer;
import java.util.function.Supplier;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.observer.core.cache.ObserverCache;
import ru.yandex.ci.observer.core.db.CiObserverDb;
import ru.yandex.ci.observer.core.db.model.check.CheckEntity;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTaskEntity;
import ru.yandex.ci.observer.reader.message.ObserverEntitiesIdChecker;
import ru.yandex.ci.observer.reader.message.internal.cache.LoadedPartitionEntitiesCache;
import ru.yandex.ci.observer.reader.proto.ObserverProtoMappers;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.CheckTaskOuterClass;
import ru.yandex.ci.storage.core.db.constant.CheckStatusUtils;

import static com.google.protobuf.TextFormat.shortDebugString;

@Slf4j
public class ObserverEntitiesChecker {
    private final InternalStreamStatistics statistics;
    private final ObserverCache cache;
    private final LoadedPartitionEntitiesCache loadedEntitiesCache;
    private final CiObserverDb db;
    private final Duration retryDelay;

    public ObserverEntitiesChecker(
            InternalStreamStatistics statistics,
            ObserverCache cache,
            LoadedPartitionEntitiesCache loadedEntitiesCache,
            CiObserverDb db, Duration retryDelay
    ) {
        this.statistics = statistics;
        this.cache = cache;
        this.loadedEntitiesCache = loadedEntitiesCache;
        this.db = db;
        this.retryDelay = retryDelay;
    }

    public Optional<CheckEntity.Id> checkExistingCheckWithRegistrationLag(Long rawCheckId, boolean skipFinished) {
        var checkId = CheckEntity.Id.of(rawCheckId);
        var checkOptional = getEntityWithRegistrationLag(
                () -> cache.checks().get(checkId),
                () -> db.readOnly(() -> db.checks().find(checkId)),
                (c, e) -> c.checks().put(e),
                checkId.toString()
        );
        if (checkOptional.isEmpty()) {
            this.statistics.onMissingError();
            if (this.cache.settings().get().isSkipMissingEntities()) {
                log.warn("Skipping missing check: {}", checkId);
                return Optional.empty();
            }

            throw new NoSuchElementException("Missing check " + checkId);
        }

        loadedEntitiesCache.onCheckLoaded(checkId);

        if (skipFinished) {
            var check = checkOptional.get();
            if (CheckStatusUtils.isCompleted(check.getStatus())) {
                log.warn("Check {} is finished in status: {}, skipping...", check.getId(), check.getStatus());
                return Optional.empty();
            }
        }

        return Optional.of(checkId);
    }

    public Optional<CheckIterationEntity.Id> checkExistingIterationWithRegistrationLag(
            CheckIteration.IterationId protoId,
            boolean skipFinished
    ) {
        if (ObserverEntitiesIdChecker.isMetaIteration(protoId)) {
            log.info("Skipping meta iteration {} message in internal stream", shortDebugString(protoId));
            return Optional.empty();
        }

        if (!ObserverEntitiesIdChecker.isProcessableIteration(
                protoId, cache.settings().get().getProcessableIterationTypes()
        )) {
            log.info("Skipping not processable iteration {} message in internal stream", shortDebugString(protoId));
            return Optional.empty();
        }

        var iterationId = ObserverProtoMappers.toIterationId(protoId);
        var iterOptional = getEntityWithRegistrationLag(
                () -> cache.iterationsGrouped().get(iterationId),
                () -> db.readOnly(() -> db.iterations().find(iterationId)),
                (c, e) -> c.iterationsGrouped().get(iterationId.getCheckId()).put(e),
                iterationId.toString()
        );
        if (iterOptional.isEmpty()) {
            this.statistics.onMissingError();
            if (this.cache.settings().get().isSkipMissingEntities()) {
                log.warn("Skipping missing iteration: {}", iterationId);
                return Optional.empty();
            }

            throw new NoSuchElementException("Missing iteration " + iterationId);
        }

        loadedEntitiesCache.onCheckLoaded(iterationId.getCheckId());

        if (skipFinished) {
            var iteration = iterOptional.get();
            if (CheckStatusUtils.isCompleted(iteration.getStatus())) {
                log.warn("iteration {} is finished in status: {}, skipping...", iteration.getId(),
                        iteration.getStatus());
                return Optional.empty();
            }
        }

        return Optional.of(iterationId);
    }

    public Optional<CheckTaskEntity.Id> checkExistingTaskWithRegistrationLag(
            CheckTaskOuterClass.FullTaskId fullTaskId,
            boolean skipFinished
    ) {
        try {
            ObserverEntitiesIdChecker.checkFullTaskId(fullTaskId);
        } catch (NullPointerException | IllegalStateException e) {
            this.statistics.onValidationError();
            log.warn("Skipping validation error: {}, fullTaskId: {}", e.getMessage(), shortDebugString(fullTaskId));
            return Optional.empty();
        }

        if (!ObserverEntitiesIdChecker.isProcessableIteration(
                fullTaskId.getIterationId(), cache.settings().get().getProcessableIterationTypes()
        )) {
            log.info("Skipping not processable iteration task {} message in internal stream",
                    shortDebugString(fullTaskId));
            return Optional.empty();
        }

        var taskId = ObserverProtoMappers.toTaskId(fullTaskId);
        var taskOptional = getEntityWithRegistrationLag(
                () -> cache.tasksGrouped().get(taskId),
                () -> db.readOnly(() -> db.tasks().find(taskId)),
                (c, e) -> c.tasksGrouped().get(taskId.getIterationId()).put(e),
                taskId.toString()
        );
        if (taskOptional.isEmpty()) {
            this.statistics.onMissingError();
            if (this.cache.settings().get().isSkipMissingEntities()) {
                log.warn("Skipping missing task: {}", taskId);
                return Optional.empty();
            }

            throw new NoSuchElementException("Missing task " + taskId);
        }

        loadedEntitiesCache.onIterationLoaded(taskId.getIterationId());

        if (skipFinished) {
            var task = taskOptional.get();
            if (CheckStatusUtils.isCompleted(task.getStatus())) {
                log.info("task {} is finished in status: {}, skipping...", task.getId(), task.getStatus());
                return Optional.empty();
            }
        }

        return Optional.of(taskId);
    }

    public List<CheckTaskEntity.Id> filterExistingTasksWithRegistrationLag(
            Collection<CheckTaskOuterClass.FullTaskId> protoIds, boolean skipFinished
    ) {
        var taskIds = new ArrayList<CheckTaskEntity.Id>(protoIds.size());

        for (var fullTaskId : protoIds) {
            var taskIdOpt = checkExistingTaskWithRegistrationLag(fullTaskId, skipFinished);
            if (taskIdOpt.isEmpty()) {
                continue;
            }

            taskIds.add(taskIdOpt.get());
        }

        return taskIds;
    }

    private <T> Optional<T> getEntityWithRegistrationLag(
            Supplier<Optional<T>> cacheFetcher, Supplier<Optional<T>> dbFetcher,
            BiConsumer<ObserverCache.Modifiable, T> cacheModifier,
            String entityId
    ) {
        var retryNumber = this.cache.settings().get().getFetchEntityRetryNumber();
        var fromCache = cacheFetcher.get();
        if (fromCache.isPresent()) {
            return fromCache;
        }

        try {
            while (true) {
                var fromDb = dbFetcher.get();
                if (fromDb.isPresent()) {
                    cache.modify(c -> cacheModifier.accept(c, fromDb.get()));
                    return fromDb;
                }

                if (retryNumber <= 0) {
                    return Optional.empty();
                }

                --retryNumber;
                log.info("Retry fetch entity with id: {}; retry countdown: {}", entityId, retryNumber);
                Thread.sleep(retryDelay.toMillis());
            }
        } catch (InterruptedException e) {
            throw new RuntimeException("Interrupted", e);
        }
    }
}
