package ru.yandex.ci.storage.reader.check;

import java.time.Instant;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.errorprone.annotations.CanIgnoreReturnValue;
import com.ibm.icu.impl.Pair;
import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.Actions;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.ShardIn;
import ru.yandex.ci.storage.core.cache.IterationsCache;
import ru.yandex.ci.storage.core.check.CheckTaskTypeUtils;
import ru.yandex.ci.storage.core.check.tasks.CancelIterationFlowTask;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.constant.CheckStatusUtils;
import ru.yandex.ci.storage.core.db.constant.StorageLimits;
import ru.yandex.ci.storage.core.db.constant.TestTypeUtils;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.FatalErrorInfo;
import ru.yandex.ci.storage.core.db.model.check_iteration.IterationInfo;
import ru.yandex.ci.storage.core.db.model.check_iteration.TestTypeStatistics;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.large.BazingaIterationId;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.reader.cache.ReaderCache;
import ru.yandex.ci.storage.reader.message.main.ReaderStatistics;
import ru.yandex.ci.storage.reader.message.writer.ShardInMessageWriter;
import ru.yandex.commune.bazinga.BazingaTaskManager;

@Slf4j
@AllArgsConstructor
public class CheckFinalizationService {
    private final ReaderCache cache;
    private final ReaderStatistics readerStatistics;
    private final CiStorageDb db;
    private final List<CheckEventsListener> eventListeners;
    private final ShardInMessageWriter shardMessageWriter;
    private final BazingaTaskManager bazingaTaskManager;
    private final CheckAnalysisService analysisService;

    private final ReaderCheckService readerCheckService;


    @CanIgnoreReturnValue
    public CheckTaskEntity processPartitionFinishedInTx(
            ReaderCache.Modifiable cache,
            CheckTaskEntity.Id taskId,
            int partition
    ) {
        var task = cache.checkTasks().getFreshOrThrow(taskId);

        if (!CheckStatusUtils.isRunning(task.getStatus())) {
            log.info("Task not running {}, status: {}", task.getId(), task.getStatus());
            return task;
        }

        if (CheckStatusUtils.isCompleted(task.getStatus())) {
            log.info("Task is not active {}, status: {}", task.getId(), task.getStatus());
            return task;
        }

        final var iteration = cache.iterations().getFreshOrThrow(taskId.getIterationId());

        if (CheckStatusUtils.isCompleted(iteration.getStatus())) {
            log.info("Iteration is not active {}, status: {}", iteration.getId(), iteration.getStatus());
            return task;
        }

        log.info(
                "Processing partition finished for task: '{}', partition: {}, current finished: [{}]",
                task.getId(), partition,
                task.getCompletedPartitions().stream().map(String::valueOf).collect(Collectors.joining(","))
        );

        var completedPartitions = new HashSet<>(task.getCompletedPartitions());
        completedPartitions.add(partition);

        var taskBuilder = task.toBuilder().completedPartitions(completedPartitions);

        if (completedPartitions.size() < task.getNumberOfPartitions()) {
            log.info(
                    "task '{}' completed partitions [{}]",
                    task.getId(),
                    task.getCompletedPartitions().stream().map(String::valueOf).collect(Collectors.joining(","))
            );

            var updatedTask = taskBuilder.build();
            cache.checkTasks().writeThrough(updatedTask);

            return updatedTask;
        }

        log.info("All partitions for task {} completed, finishing task", taskId);

        var updatedTask = taskBuilder.build().complete(Common.CheckStatus.COMPLETED_SUCCESS);
        cache.checkTasks().writeThrough(updatedTask);

        this.eventListeners.forEach(listener -> listener.onTaskCompleted(readerCheckService, cache, taskId));

        onTaskFinished(cache, taskId, iteration.getId());

        var metaIteration = cache.iterations().getFresh(taskId.getIterationId().toMetaId());
        metaIteration.ifPresent(entity -> onTaskFinished(cache, taskId, entity.getId()));

        return updatedTask;
    }

    private void onTaskFinished(
            ReaderCache.Modifiable cache, CheckTaskEntity.Id taskId, CheckIterationEntity.Id iterationId
    ) {
        final var iteration = cache.iterations().getFreshOrThrow(iterationId);
        var activeTasks = iteration.getNumberOfTasks() - iteration.getNumberOfCompletedTasks() - 1;
        var notRegisteredTasks = Math.max(
                iteration.getExpectedTasks().size() - iteration.getRegisteredExpectedTasks().size(),
                0
        );

        if (iterationId.isHeavy()) {
            var iterationBuilder = iteration.toBuilder();
            var task = cache.checkTasks().getFreshOrThrow(taskId);
            for (var chunkType : CheckTaskTypeUtils.toChunkType(task.getType())) {
                completeChunkTypeInTestTypeStatistics(
                        taskId,
                        iterationBuilder,
                        chunkType,
                        iteration.isAllRequiredTasksRegistered() ||
                                chunkType.equals(Common.ChunkType.CT_NATIVE_BUILD) ||
                                chunkType.equals(Common.ChunkType.CT_LARGE_TEST) // temporary for CI-3163
                );
            }

            cache.iterations().writeThrough(iterationBuilder.build());
            this.startFinalizationForCompletedChunks(cache, iterationId);
        }

        if (activeTasks < 0 && ((activeTasks + notRegisteredTasks) == 0)) {
            log.error(
                    "[Logic error] Negative active tasks, iteration {}, active tasks: {}, not registered tasks: {}",
                    iterationId,
                    activeTasks,
                    notRegisteredTasks
            );
            this.readerStatistics.onLogicError();

            onAllTasksFinished(cache, iterationId);
        } else if (activeTasks == 0 && notRegisteredTasks == 0) {
            onAllTasksFinished(cache, iterationId);
        } else {
            log.info(
                    "Tasks still running, iteration {}, active tasks: {}, not registered tasks: {}",
                    iterationId,
                    activeTasks,
                    notRegisteredTasks
            );

            onTasksStillRunning(cache, iterationId);
        }
    }

    private void onTasksStillRunning(ReaderCache.Modifiable cache, CheckIterationEntity.Id iterationId) {
        final var iteration = cache.iterations().getOrThrow(iterationId);
        var iterationBuilder = iteration.toBuilder();
        if (iteration.getStatus().equals(Common.CheckStatus.CREATED)) {
            iterationBuilder.status(Common.CheckStatus.RUNNING);
            var check = cache.checks().getFreshOrThrow(iteration.getId().getCheckId());
            if (check.getStatus().equals(Common.CheckStatus.CREATED)) {
                cache.checks().writeThrough(check.withStatus(Common.CheckStatus.RUNNING));
            }
        }

        cache.iterations().writeThrough(
                iterationBuilder
                        .numberOfCompletedTasks(iteration.getNumberOfCompletedTasks() + 1)
                        .info(updateProgress(iteration))
                        .build()
        );
    }

    private void onAllTasksFinished(ReaderCache.Modifiable cache, CheckIterationEntity.Id iterationId) {
        final var iteration = cache.iterations().getOrThrow(iterationId);

        log.info(
                "All tasks for iteration {} completed, test type statistics: {}",
                iteration.getId(), iteration.getTestTypeStatistics().printStatus()
        );

        var info = iteration.getInfo().toBuilder().progress(99).build();
        cache.iterations().writeThrough(
                iteration.toBuilder()
                        .numberOfCompletedTasks(iteration.getNumberOfCompletedTasks() + 1)
                        .status(Common.CheckStatus.COMPLETING)
                        .allTasksFinished(Instant.now())
                        .info(info)
                        .build()
        );
    }

    private IterationInfo updateProgress(CheckIterationEntity iteration) {
        return iteration.getInfo().toBuilder()
                .progress(iteration.getTestTypeStatistics().calculateProgress())
                .build();
    }

    public void finishIteration(CheckIterationEntity.Id iterationId) {
        cache.modifyWithDbTx(cache -> finishIterationInTx(cache, iterationId));
    }

    private void finishIterationInTx(ReaderCache.Modifiable cache, CheckIterationEntity.Id iterationId) {
        final var iteration = cache.iterations().getFreshOrThrow(iterationId);
        if (CheckStatusUtils.isCompleted(iteration.getStatus())) {
            log.warn("Iteration {} is not running, skipping finish", iterationId);
            return;
        }

        this.eventListeners.forEach(listener -> listener.onBeforeIterationCompleted(readerCheckService, cache,
                iteration.getId()));

        var completedIteration = iteration.complete(iteration.getStatistics().toCompletedStatus());
        var updatedIteration = analysisService.analyzeIterationFinish(completedIteration);

        cache.iterations().writeThrough(updatedIteration);

        this.eventListeners.forEach(listener -> listener.onIterationCompleted(readerCheckService, cache,
                updatedIteration.getId()));

        var activeIterations = this.getActiveIterations(cache.iterations(), updatedIteration.getId().getCheckId());

        cache.iterations().getFresh(iterationId.toMetaId()).ifPresentOrElse(
                metaIteration -> tryFinishMetaIteration(cache, updatedIteration, activeIterations, metaIteration),
                () -> this.eventListeners.forEach(
                        listener -> listener.onIterationTypeCompleted(readerCheckService, cache,
                                updatedIteration.getId())
                )
        );

        tryFinishCheck(cache, iterationId);
    }

    private void tryFinishMetaIteration(
            ReaderCache.Modifiable cache,
            CheckIterationEntity iteration,
            List<CheckIterationEntity> activeIterations,
            CheckIterationEntity metaIteration
    ) {
        var activeIterationOfSameType = activeIterations.stream()
                .filter(x -> x.getId().getIterationType().equals(iteration.getId().getIterationType()))
                .filter(x -> !x.getId().isMetaIteration())
                .toList();

        if (activeIterationOfSameType.isEmpty()) {
            log.info(
                    "Completing meta iteration {} after iteration {} finished",
                    metaIteration.getId(), iteration.getId()
            );
            var updatedMetaIteration = analysisService.analyzeIterationFinish(metaIteration).toBuilder()
                    .info(metaIteration.getInfo().toBuilder().progress(100).build())
                    .status(metaIteration.getStatistics().toCompletedStatus())
                    .finish(Instant.now())
                    .build();

            cache.iterations().writeThrough(updatedMetaIteration);

            this.eventListeners.forEach(
                    listener -> listener.onMetaIterationCompleted(readerCheckService, cache,
                            updatedMetaIteration.getId())
            );
            this.eventListeners.forEach(
                    listener -> listener.onIterationTypeCompleted(readerCheckService, cache,
                            updatedMetaIteration.getId())
            );
        }
    }

    private void tryFinishCheck(ReaderCache.Modifiable cache, CheckIterationEntity.Id iterationId) {
        var check = cache.checks().getFreshOrThrow(iterationId.getCheckId());
        var iterations = cache.iterations().getFreshForCheck(iterationId.getCheckId());
        var activeIterations = getActiveIterations(iterations);

        if (activeIterations.isEmpty()) {
            log.info("Completing check after iteration {} finished", iterationId);

            var checkStatus = CheckStatusResolver.evaluateFinishedCheckStatus(iterationId.getCheckId(), iterations);
            var completedCheck = check.complete(checkStatus);
            var updatedCheck = analysisService.analyzeCheckFinish(completedCheck, iterations);
            cache.checks().writeThrough(updatedCheck);

            var iterationIds = mapToIterationIds(iterations);
            this.eventListeners.forEach(
                    listener -> listener.onCheckCompleted(readerCheckService, cache, check.getId(), iterationIds)
            );
        }
    }

    public void processTaskFatalError(
            CheckTaskEntity.Id taskId, String message, String details, String sandboxTaskId
    ) {
        cache.modifyWithDbTx(cache -> {
            var check = cache.checks().getFreshOrThrow(taskId.getIterationId().getCheckId());
            var iterationId = taskId.getIterationId();
            var brokenIteration = cache.iterations().getFreshOrThrow(iterationId);
            if (CheckStatusUtils.isCompleted(brokenIteration.getStatus())) {
                log.warn(
                        "Iteration {} is not running, skipping fatal error message, message: {}, details: {}",
                        brokenIteration.getId(), message, details
                );
                return;
            }

            cache.checks().writeThrough(check.complete(Common.CheckStatus.COMPLETED_WITH_FATAL_ERROR, true));

            var iterations = cache.iterations().getFreshForCheck(iterationId.getCheckId());
            var runningIterations = iterations.stream()
                    .filter(x -> CheckStatusUtils.isRunning(x.getStatus()))
                    .toList();

            runningIterations.forEach(iteration -> cache.iterations().writeThrough(
                    iteration.toBuilder()
                            .status(Common.CheckStatus.COMPLETED_WITH_FATAL_ERROR)
                            .finish(Instant.now())
                            .info(
                                    iterationId.getIterationType() == iteration.getId().getIterationType() ?
                                            iteration.getInfo().toBuilder()
                                                    .fatalErrorInfo(
                                                            FatalErrorInfo.builder()
                                                                    .message(message)
                                                                    .details(details)
                                                                    .sandboxTaskId(sandboxTaskId)
                                                                    .build()
                                                    )
                                                    .build() :
                                            iteration.getInfo().toBuilder()
                                                    .fatalErrorInfo(
                                                            FatalErrorInfo.builder()
                                                                    .message("Other circuit failure")
                                                                    .sandboxTaskId(sandboxTaskId)
                                                                    .build()
                                                    )
                                                    .build()
                            )
                            .build()
            ));

            cache.checkTasks().writeThrough(
                    cache.checkTasks().getIterationTasks(brokenIteration.getId()).stream()
                            .map(task -> task.complete(Common.CheckStatus.COMPLETED_WITH_FATAL_ERROR))
                            .collect(Collectors.toList())
            );

            var iterationIds = mapToIterationIds(iterations);
            var runningIterationsIds = mapToIterationIds(runningIterations);

            this.eventListeners.forEach(
                    listener -> listener.onCheckFatalError(
                            readerCheckService, cache, check.getId(), iterationIds, brokenIteration.getId(),
                            runningIterationsIds
                    )
            );

            runningIterations.stream().map(CheckIterationEntity::getId)
                    .filter(x -> !x.isMetaIteration())
                    .forEach(iteration -> bazingaTaskManager.schedule(
                                    new CancelIterationFlowTask(new BazingaIterationId(iteration))
                            )
                    );
        });
    }

    private static Set<CheckIterationEntity.Id> mapToIterationIds(Collection<CheckIterationEntity> iterations) {
        return iterations.stream().map(CheckIterationEntity::getId).collect(Collectors.toSet());
    }

    public void processIterationCancelled(CheckIterationEntity.Id id) {
        cache.modifyWithDbTx(cache -> processIterationCancelledInTx(id, cache));
    }

    private void processIterationCancelledInTx(CheckIterationEntity.Id id, ReaderCache.Modifiable cache) {
        var cancellingIteration = cache.iterations().getFreshOrThrow(id);
        if (CheckStatusUtils.isCompleted(cancellingIteration.getStatus())) {
            log.warn("Iteration {} is not running, skipping cancel", cancellingIteration.getId());
            return;
        }

        log.info(
                "Cancelling iteration: {}, status: {}, number of tasks: {}",
                cancellingIteration.getId(), cancellingIteration.getStatus(), cancellingIteration.getNumberOfTasks()
        );

        var nextStatus = cancellingIteration.getStatus().equals(Common.CheckStatus.CANCELLING_BY_TIMEOUT) ?
                Common.CheckStatus.CANCELLED_BY_TIMEOUT : Common.CheckStatus.CANCELLED;

        var check = cache.checks().getFreshOrThrow(id.getCheckId());
        var checkIterations = cache.iterations().getFreshForCheck(id.getCheckId());
        var activeIterations = checkIterations.stream()
                .filter(iteration -> CheckStatusUtils.isActive(iteration.getStatus()))
                .toList();

        activeIterations.forEach(iteration -> {
            cache.iterations().writeThrough(iteration.complete(nextStatus));
            if (!iteration.getId().isMetaIteration()) {
                cache.checkTasks().writeThrough(
                        cache.checkTasks().getIterationTasks(iteration.getId()).stream()
                                .filter(x -> CheckStatusUtils.isActive(x.getStatus()))
                                .map(x -> x.complete(nextStatus))
                                .collect(Collectors.toList())
                );

                bazingaTaskManager.schedule(new CancelIterationFlowTask(new BazingaIterationId(iteration.getId())));
            }
        });

        var updatedCheck = check.complete(nextStatus, true);
        cache.checks().writeThrough(updatedCheck);

        this.eventListeners.forEach(
                listener -> listener.onCheckCancelled(
                        readerCheckService, cache, updatedCheck.getId(),
                        mapToIterationIds(checkIterations)
                )
        );
    }

    public Set<ChunkEntity.Id> getAffectedChunkIds(
            ReaderCache.Modifiable cache, CheckIterationEntity.Id iterationId
    ) {
        final var iteration = cache.iterations().getOrThrow(iterationId);

        if (iteration.getNumberOfTasks() < 512) {
            return this.db.currentOrReadOnly(() -> getAffectedChunkIdsInTx(iterationId));
        } else {
            return this.db.scan().withMaxSize(StorageLimits.TASKS_LIMIT).run(
                    () -> getAffectedChunkIdsInTx(iterationId)
            );
        }
    }

    private Set<ChunkEntity.Id> getAffectedChunkIdsInTx(CheckIterationEntity.Id iterationId) {
        var taskStatistics = this.db.checkTaskStatistics().getByIteration(iterationId);

        var chunkIds = new HashSet<ChunkEntity.Id>();
        for (var statistics : taskStatistics) {
            var chunks = statistics.getStatistics().getAffectedChunks();
            chunks.getChunks().forEach(
                    (key, value) -> value.forEach(chunk -> chunkIds.add(ChunkEntity.Id.of(key, chunk)))
            );
        }

        return chunkIds;
    }

    public void startFinalizationForCompletedChunks(CheckIterationEntity.Id iterationId) {
        cache.modifyWithDbTx(cache -> startFinalizationForCompletedChunks(cache, iterationId));
    }

    public void startFinalizationForCompletedChunks(
            ReaderCache.Modifiable cache, CheckIterationEntity.Id iterationId
    ) {
        final var iteration = cache.iterations().getOrThrow(iterationId);
        var waitingForConfigure = iteration.getTestTypeStatistics().getWaitingForConfigure();

        if (!waitingForConfigure.isEmpty()) {
            var chunkTypeNames = waitingForConfigure.stream().map(Enum::name).collect(Collectors.joining(", "));

            var configure = iteration.getTestTypeStatistics().getConfigure();

            log.info(
                    "Chunk types [{}] completed in all {} tasks for {}, configure status: {}",
                    chunkTypeNames,
                    iteration.getNumberOfTasks(),
                    iterationId,
                    configure.getStatus()
            );

            if (
                    iteration.isHeavy() || // heavy iteration does not wait for configure
                            configure.isCompleted() ||
                            // We are finishing chunks without waiting for configure to complete,
                            // possibly this can cause problems
                            configure.isWaitingForChunks() ||
                            waitingForConfigure.contains(Common.ChunkType.CT_CONFIGURE)
            ) {
                this.finalizeChunkTypes(cache, iterationId, waitingForConfigure);
            }
        }
    }

    public void processTestTypeFinishedInTx(
            ReaderCache.Modifiable cache,
            CheckTaskEntity.Id taskId,
            int partition,
            Actions.TestType testType,
            @Nullable Actions.TestTypeSizeFinished.Size size
    ) {
        if (taskId.getIterationId().isHeavy()) {
            return; // Heavy tasks has no partitions and counted only in task finished
        }

        var task = cache.checkTasks().getFreshOrThrow(taskId);

        var chunkTypes = TestTypeUtils.toChunkType(testType, size);

        var oldPartitionsFinishStatistics = task.getPartitionsStatistics();
        var partitionsFinishStatistics = oldPartitionsFinishStatistics.onChunkFinished(partition, chunkTypes);
        cache.checkTasks().writeThrough(
                task.toBuilder()
                        .partitionsStatistics(partitionsFinishStatistics)
                        .build()
        );

        var iteration = cache.iterations().getFreshOrThrow(taskId.getIterationId());

        var builder = iteration.toBuilder();
        var metaId = taskId.getIterationId().toMetaId();

        var metaIterationOptional = cache.iterations().getFresh(metaId);
        var metaIterationBuilder = metaIterationOptional.map(CheckIterationEntity::toBuilder).orElse(null);

        for (var chunkType : chunkTypes) {
            if (oldPartitionsFinishStatistics.isChunkCompleted(task.getNumberOfPartitions(), chunkType)) {
                continue;
            }

            if (partitionsFinishStatistics.isChunkCompleted(task.getNumberOfPartitions(), chunkType)) {
                completeChunkTypeInTestTypeStatistics(
                        taskId, builder, chunkType, iteration.isAllRequiredTasksRegistered()
                );
                if (metaIterationOptional.isPresent()) {
                    var metaIteration = metaIterationOptional.get();
                    completeChunkTypeInTestTypeStatistics(
                            taskId, metaIterationBuilder, chunkType, metaIteration.isAllRequiredTasksRegistered()
                    );
                }
            }
        }

        builder.info(updateProgress(builder.build()));
        cache.iterations().writeThrough(builder.build());

        if (metaIterationBuilder != null) {
            metaIterationBuilder.info(updateProgress(metaIterationBuilder.build()));
            cache.iterations().writeThrough(metaIterationBuilder.build());
        }
    }

    private void completeChunkTypeInTestTypeStatistics(
            CheckTaskEntity.Id taskId,
            CheckIterationEntity.Builder builder,
            Common.ChunkType chunkType,
            boolean canFinish
    ) {
        log.info("Completing chunk type {} for {}", chunkType, taskId);
        if (builder.getTestTypeStatistics().get(chunkType).isCompleted()) {
            log.error("[Logic error] Chunk type {} for {} already finished", chunkType, taskId);
            this.readerStatistics.onLogicError();
        } else {
            builder.testTypeStatistics(builder.getTestTypeStatistics().onCompleted(chunkType, canFinish));
        }
    }

    private void processChunkFinalizationStarted(
            ReaderCache.Modifiable cache,
            Common.ChunkType chunkType,
            CheckIterationEntity.Id iterationId,
            Set<ChunkEntity.Id> chunks
    ) {
        final var iteration = cache.iterations().getFreshOrThrow(iterationId);
        var metaIterationOptional = cache.iterations().getFresh(iterationId.toMetaId());

        cache.iterations().writeThrough(
                iteration.toBuilder()
                        .testTypeStatistics(
                                iteration.getTestTypeStatistics().onChunkFinalizationStarted(chunkType, chunks)
                        )
                        .build()
        );

        if (metaIterationOptional.isPresent()) {
            var metaIteration = metaIterationOptional.get();
            cache.iterations().writeThrough(
                    metaIteration.toBuilder()
                            .testTypeStatistics(
                                    metaIteration.getTestTypeStatistics().onChunkFinalizationStarted(chunkType, chunks)
                            )
                            .build()
            );
        }

        if (chunks.isEmpty()) {
            this.processChunkTypeFinalized(cache, iteration.getId(), chunkType);
        }
    }

    private void processChunkTypeFinalized(
            ReaderCache.Modifiable cache, CheckIterationEntity.Id iterationId, Common.ChunkType chunkType
    ) {
        log.info("Chunk type {} of iteration {} finalized", iterationId, chunkType);
        this.eventListeners.forEach(listener -> listener.onChunkTypeFinalized(readerCheckService, cache, iterationId,
                chunkType));

        if (cache.iterations().getFreshOrThrow(iterationId).getStatus().equals(Common.CheckStatus.COMPLETING)) {
            tryFinishIteration(cache, iterationId);
        }
    }

    public void processChunkFinished(
            CheckIterationEntity.Id iterationId,
            List<Pair<ChunkAggregateEntity, Optional<ChunkAggregateEntity>>> aggregatesWithMetaAggregate
    ) {
        log.info(
                "Processing finish, iteration: {}, chunks: {}",
                iterationId,
                aggregatesWithMetaAggregate.stream()
                        .map(x -> x.first.getId().getChunkId().toString())
                        .collect(Collectors.joining(", "))
        );

        this.cache.modifyWithDbTx(cache -> processChunkFinishedInTx(iterationId, aggregatesWithMetaAggregate, cache));
    }

    private void processChunkFinishedInTx(
            CheckIterationEntity.Id iterationId,
            List<Pair<ChunkAggregateEntity, Optional<ChunkAggregateEntity>>> aggregatesWithMetaAggregate,
            ReaderCache.Modifiable cache
    ) {
        final var iteration = cache.iterations().getFreshOrThrow(iterationId);
        final var metaIteration = cache.iterations().getFresh(iterationId.toMetaId());

        if (CheckStatusUtils.isCompleted(iteration.getStatus())) {
            log.warn(
                    "Skipping chunk finished because iteration {} is not active, status: {}",
                    iteration.getId(), iteration.getStatus()
            );
            cache.iterations().put(iteration);
            return;
        }

        var testTypeStatistics = iteration.getTestTypeStatistics();
        var metaTestTypeStatistics = metaIteration.isEmpty() ? null : metaIteration.get().getTestTypeStatistics();

        var finishedChunkTypes = new ArrayList<Common.ChunkType>();
        for (var aggregate : aggregatesWithMetaAggregate) {
            var aggregateId = aggregate.first.getId();
            log.info(
                    "Chunk completed {}, state {}",
                    aggregateId, aggregate.first.getState()
            );

            testTypeStatistics = testTypeStatistics.onChunkCompleted(aggregateId.getChunkId());
            if (testTypeStatistics.get(aggregateId.getChunkId().getChunkType()).isCompleted()) {
                finishedChunkTypes.add(aggregateId.getChunkId().getChunkType());
            }

            cache.chunkAggregatesGroupedByIteration().put(aggregate.first);
            if (aggregate.second.isPresent()) {
                var metaAggregateId = aggregate.second.get().getId();
                log.info(
                        "Meta chunk completed {}, state {}",
                        metaAggregateId, aggregate.second.get().getState()
                );

                metaTestTypeStatistics = (
                        metaTestTypeStatistics == null ? TestTypeStatistics.EMPTY : metaTestTypeStatistics
                ).onChunkCompleted(metaAggregateId.getChunkId());

                cache.chunkAggregatesGroupedByIteration().put(aggregate.second.get());
            }
        }

        var updatedIteration = CheckIterationEntity.sumOf(
                iteration.toBuilder()
                        .testTypeStatistics(testTypeStatistics)
                        .build(),
                cache.chunkAggregatesGroupedByIteration().getByIterationId(iterationId)
        );

        readerCheckService.sendCheckFailedInAdvance(cache, updatedIteration);

        cache.iterations().writeThrough(updatedIteration);

        if (metaIteration.isPresent()) {
            cache.iterations().writeThrough(
                    CheckIterationEntity.sumOf(
                            metaIteration.get().toBuilder()
                                    .testTypeStatistics(metaTestTypeStatistics)
                                    .build(),
                            getMetaIterationAggregates(cache, metaIteration.get().getId())
                    )
            );
        }

        finishedChunkTypes.forEach(chunkType -> this.processChunkTypeFinalized(cache, iterationId, chunkType));
    }

    private void tryFinishIteration(ReaderCache.Modifiable cache, CheckIterationEntity.Id iterationId) {
        final var iteration = cache.iterations().getFreshOrThrow(iterationId);
        var notCompletedTestTypes = iteration.getTestTypeStatistics().getNotCompleted();

        if (notCompletedTestTypes.isEmpty()) {
            log.info("All test types for {} completed", iteration.getId());
            finishIterationInTx(cache, iteration.getId());
        } else {
            log.info(
                    "Iteration {} waiting for test type completed: {}, status: {}",
                    iteration.getId(),
                    notCompletedTestTypes.stream().map(Enum::name).collect(Collectors.joining(", ")),
                    iteration.getTestTypeStatistics().printStatus()
            );
        }
    }

    public Collection<ChunkAggregateEntity> getMetaIterationAggregates(
            ReaderCache.Modifiable cache,
            CheckIterationEntity.Id iterationId
    ) {
        var aggregates = cache.chunkAggregatesGroupedByIteration()
                .getByIterationId(iterationId.toIterationId(1)).stream()
                .collect(Collectors.toMap(x -> x.getId().getChunkId(), Function.identity()));

        aggregates.putAll(
                cache.chunkAggregatesGroupedByIteration().getByIterationId(iterationId).stream()
                        .collect(Collectors.toMap(x -> x.getId().getChunkId(), Function.identity()))
        );

        return aggregates.values();
    }

    public void finalizeChunkTypes(
            ReaderCache.Modifiable cache,
            CheckIterationEntity.Id iterationId,
            Set<Common.ChunkType> completedChunkTypes
    ) {
        var affectedChunksByType = this.getAffectedChunkIds(cache, iterationId).stream()
                .collect(Collectors.groupingBy(ChunkEntity.Id::getChunkType, Collectors.toSet()));

        for (var chunkType : completedChunkTypes) {
            var affectedChunks = affectedChunksByType.getOrDefault(chunkType, Set.of());

            if (!iterationId.isMetaIteration()) {
                // Sending finalization for meta iteration has no sense, because we are not sending results for it.
                // Meta chunks will finish when all according chunks from iterations finish.
                sendChunkFinish(cache, iterationId, affectedChunks, Common.ChunkAggregateState.CAS_COMPLETED);
            }

            this.processChunkFinalizationStarted(cache, chunkType, iterationId, affectedChunks);
        }
    }

    public void sendChunkFinish(
            ReaderCache.Modifiable cache,
            CheckIterationEntity.Id iterationId,
            Set<ChunkEntity.Id> chunks,
            Common.ChunkAggregateState state
    ) {
        if (chunks.isEmpty()) {
            return;
        }

        log.info(
                "Sending chunk completion for {}, chunks: [{}]",
                iterationId,
                chunks.stream().map(ChunkEntity.Id::toString).collect(Collectors.joining(", "))
        );

        shardMessageWriter.writeChunkMessages(cache, toChunkFinishedMessages(iterationId, chunks, state));
    }

    private List<ShardIn.ChunkMessage> toChunkFinishedMessages(
            CheckIterationEntity.Id iterationId,
            Set<ChunkEntity.Id> affectedChunks,
            Common.ChunkAggregateState state
    ) {
        return affectedChunks.stream()
                .map(
                        chunk -> ShardIn.ChunkMessage.newBuilder()
                                .setChunkId(CheckProtoMappers.toProtoChunkId(chunk))
                                .setFinish(
                                        ShardIn.FinishChunk.newBuilder()
                                                .setIterationId(
                                                        CheckProtoMappers.toProtoIterationId(iterationId)
                                                )
                                                .setState(state)
                                                .build()
                                )
                                .build()
                )
                .collect(Collectors.toList());
    }

    public List<CheckIterationEntity> getActiveIterations(
            IterationsCache iterationsCache, CheckEntity.Id checkId
    ) {
        return getActiveIterations(iterationsCache.getFreshForCheck(checkId));
    }

    protected List<CheckIterationEntity> getActiveIterations(Collection<CheckIterationEntity> iterations) {
        return iterations.stream()
                .filter(x -> CheckStatusUtils.isActive(x.getStatus()))
                .collect(Collectors.toList());
    }
}
