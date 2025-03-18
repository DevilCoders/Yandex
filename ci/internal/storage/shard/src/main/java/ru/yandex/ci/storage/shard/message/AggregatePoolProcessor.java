package ru.yandex.ci.storage.shard.message;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.collect.Lists;
import com.google.common.hash.Hashing;
import com.google.common.util.concurrent.ThreadFactoryBuilder;
import io.micrometer.core.instrument.Gauge;
import lombok.extern.slf4j.Slf4j;

import yandex.cloud.repository.db.exception.UnavailableException;

import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.ShardOut;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.task_result.TestResult;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffByHashEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffImportantEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.index.TestDiffBySuiteEntity;
import ru.yandex.ci.storage.core.message.shard_out.ShardOutMessageWriter;
import ru.yandex.ci.storage.core.micrometr.StorageMetrics;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.core.utils.TimeTraceService;
import ru.yandex.ci.storage.shard.ShardStatistics;
import ru.yandex.ci.storage.shard.cache.ShardCache;
import ru.yandex.ci.storage.shard.task.AggregateProcessor;
import ru.yandex.ci.util.Retryable;
import ru.yandex.ci.util.queue.QueueWorker;

@SuppressWarnings("UnstableApiUsage")
@Slf4j
public class AggregatePoolProcessor {
    private static final int AGGREGATES_CALCULATION_BATCH_LIMIT = 128;
    private static final int BULK_LIMIT = 4096;

    private final CiStorageDb db;
    private final AggregateProcessor aggregateProcessor;
    private final ShardCache shardCache;
    private final AggregateReporter aggregateReporter;
    private final ShardOutMessageWriter writer;
    private final PostProcessorDeliveryService postProcessor;
    private final TimeTraceService timeTraceService;
    private final ShardStatistics statistics;
    private final int numberOfAggregateQueues;

    private final List<BlockingQueue<AggregateMessage>> aggregatesQueues;

    public AggregatePoolProcessor(
            CiStorageDb db,
            AggregateProcessor aggregateProcessor,
            ShardCache shardCache,
            AggregateReporter aggregateReporter,
            ShardOutMessageWriter writer,
            TimeTraceService timeTraceService,
            ShardStatistics statistics,
            PostProcessorDeliveryService postProcessor,
            int numberOfAggregateQueues,
            int queueDrainLimit
    ) {
        this.db = db;
        this.aggregateProcessor = aggregateProcessor;
        this.shardCache = shardCache;
        this.aggregateReporter = aggregateReporter;
        this.writer = writer;
        this.timeTraceService = timeTraceService;
        this.statistics = statistics;
        this.postProcessor = postProcessor;
        this.numberOfAggregateQueues = numberOfAggregateQueues;

        this.aggregatesQueues = new ArrayList<>(numberOfAggregateQueues);

        var queueSizeMetricName = StorageMetrics.PREFIX + "aggregate_pool_queue_size";

        statistics.register(
                Gauge.builder(
                                queueSizeMetricName,
                                () -> aggregatesQueues.stream().mapToInt(Collection::size).sum()
                        )
                        .tag(StorageMetrics.QUEUE, "all")
        );

        if (numberOfAggregateQueues == 0) {
            return;
        }

        ExecutorService executorService = Executors.newFixedThreadPool(
                numberOfAggregateQueues,
                new ThreadFactoryBuilder().setNameFormat("aggregate-pool-%d").build()
        );

        for (int i = 0; i < numberOfAggregateQueues; i++) {
            var queue = new LinkedBlockingQueue<AggregateMessage>();
            aggregatesQueues.add(queue);
            executorService.execute(new AggregatePoolWorker(queue, queueDrainLimit));

            statistics.register(
                    Gauge.builder(queueSizeMetricName, queue::size)
                            .tag(StorageMetrics.QUEUE, String.valueOf(i))
            );
        }
    }

    private void process(List<AggregateMessage> messages) {
        processResults(messages.stream().filter(x -> x.getResult() != null).collect(Collectors.toList()));
        processFinish(messages.stream().filter(x -> x.getFinish() != null).collect(Collectors.toList()));
    }

    private void processFinish(List<AggregateMessage> messages) {
        if (messages.isEmpty()) {
            return;
        }

        // We can not allow throw here, because part of results could be already commited in processResults
        Retryable.retryUntilInterruptedOrSucceeded(
                () -> processFinishRetryable(messages),
                (e) -> this.statistics.onAggregateWorkerError()
        );

        messages.forEach(x -> x.getCommitCountdown().notifyMessageProcessed());
    }

    private void processFinishRetryable(List<AggregateMessage> messages) {
        var aggregateIds = messages.stream().map(AggregateMessage::getAggregateId).collect(Collectors.toSet());

        log.info(
                "Finishing aggregates: [{}]",
                aggregateIds.stream().map(ChunkAggregateEntity.Id::toString).collect(Collectors.joining(", "))
        );

        var aggregates = this.shardCache.chunkAggregates().get(aggregateIds).stream()
                .collect(Collectors.toMap(ChunkAggregateEntity::getId, Function.identity()));

        var skipMissing = this.shardCache.settings().get().getShard().getSkip().isMissing();

        for (var message : messages) {
            var id = message.getAggregateId();
            var aggregate = aggregates.get(id);
            if (aggregate == null) {
                if (skipMissing) {
                    log.warn("Chunk aggregate not found {}", id);
                    //noinspection ConstantConditions
                    if (message.getFinish().getState() == Common.ChunkAggregateState.CAS_COMPLETED) {
                        this.statistics.onMissingError();
                    }
                } else {
                    throw new RuntimeException("Chunk aggregate not found: " + id);
                }
            } else if (aggregate.isFinished()) {
                log.info("Chunk aggregate already finished {}", id);
            } else {
                //noinspection ConstantConditions
                this.finish(aggregate, message.getFinish().getState());
            }
        }

        var finishMessages = aggregateIds.stream()
                .map(id -> shardCache.chunkAggregates().get(id))
                .filter(Optional::isPresent)
                .map(
                        aggregateOptional -> {
                            var aggregate = aggregateOptional.get();
                            if (!aggregate.isFinished()) {
                                log.error(
                                        "[Logic error] Aggregate not finished in cache: {}", aggregate.getId()
                                );
                                this.statistics.onLogicError();

                                aggregate = aggregate.toBuilder()
                                        .state(Common.ChunkAggregateState.CAS_COMPLETED)
                                        .build();
                            }

                            var builder = ShardOut.ChunkFinished.newBuilder().setAggregate(
                                    CheckProtoMappers.toProtoAggregate(aggregate)
                            );

                            var metaAggregate = shardCache.chunkAggregates().get(aggregate.getId().toIterationMetaId());
                            metaAggregate.ifPresent(chunkAggregateEntity -> builder.setMetaAggregate(
                                    CheckProtoMappers.toProtoAggregate(chunkAggregateEntity)
                            ));

                            return builder.build();
                        }
                )
                .collect(Collectors.toList());

        this.writer.writeFinish(finishMessages);
    }

    private void finish(ChunkAggregateEntity aggregate, Common.ChunkAggregateState state) {
        var diffs = this.shardCache.testDiffs().get(aggregate.getId()).getAll().stream()
                .filter(TestDiffByHashEntity::isUnknown)
                .filter(x -> !x.getId().getTestId().isAggregate())
                .toList();

        log.info("Finishing aggregate {}, state: {}, unknown diffs: {}", aggregate.getId(), state, diffs.size());

        var check = shardCache.checks().getOrThrow(aggregate.getId().getIterationId().getCheckId());
        var fakeResults = diffs.stream()
                .map(diff -> toFakeResult(diff, check))
                .collect(Collectors.toList());

        this.processResults(aggregate.getId(), fakeResults, null);

        this.shardCache.modifyWithDbTx(
                cache -> {
                    cache.chunkAggregates().writeThrough(
                            cache.chunkAggregates().getOrThrow(aggregate.getId())
                                    .finish(state)
                    );

                    cache.chunkAggregates()
                            .get(aggregate.getId().toIterationMetaId())
                            .ifPresent(chunkAggregateEntity -> cache.chunkAggregates().writeThrough(
                                    chunkAggregateEntity.finish(state)
                            ));
                }
        );

        this.shardCache.modify(cache -> cache.testDiffs().invalidate(aggregate.getId()));

        log.info("Aggregate finished: {}", aggregate.getId());
    }

    private TestResult toFakeResult(TestDiffByHashEntity diff, CheckEntity check) {
        var isRight = diff.getRight() == Common.TestStatus.TS_UNKNOWN;
        return TestResult.builder()
                .id(
                        new TestResultEntity.Id(
                                diff.getId().getAggregateId().getIterationId(),
                                diff.getId().getTestId(),
                                "", 0, 0
                        )
                )
                .oldSuiteId(diff.getOldSuiteId())
                .oldTestId(diff.getOldTestId())
                .status(Common.TestStatus.TS_NONE)
                .chunkId(diff.getId().getAggregateId().getChunkId())
                .path(diff.getPath())
                .name(diff.getName())
                .subtestName(diff.getSubtestName())
                .resultType(diff.getResultType())
                .isRight(isRight)
                .isStrongMode(diff.isStrongMode())
                .strongModeAYaml(diff.getStrongModeAYaml())
                .autocheckChunkId(diff.getAutocheckChunkId())
                .revision(isRight ? check.getRight().getRevision() : check.getLeft().getRevision())
                .revisionNumber(isRight ? check.getRight().getRevisionNumber() : check.getLeft().getRevisionNumber())
                .branch(isRight ? check.getRight().getBranch() : check.getLeft().getBranch())
                .build();
    }

    private void retryUntilInterruptedOrSucceeded(Runnable action) {
        Retryable.retryUntilInterruptedOrSucceeded(
                action,
                (e) -> {
                    if (e instanceof UnavailableException) {
                        log.warn("Db unavailable {}", e.getMessage());
                        this.statistics.onDbUnavailableError();
                    } else {
                        log.error("Failed", e);
                        this.statistics.onAggregateWorkerError();
                    }
                },
                true
        );
    }

    private void processResults(List<AggregateMessage> messages) {
        if (messages.isEmpty()) {
            return;
        }

        var byAggregateId = messages.stream().map(AggregateMessage::getResult).collect(
                Collectors.groupingBy(ChunkMessageWithResults::getAggregateId)
        );

        log.info(
                "Processing aggregate result messages, total: {}, by aggregate: {}",
                messages.size(),
                byAggregateId.entrySet().stream()
                        .map(x -> "%s - %d".formatted(x.getKey(), x.getValue().size()))
                        .collect(Collectors.joining(", "))
        );

        for (var group : byAggregateId.entrySet()) {
            var results = group.getValue().stream().flatMap(x -> x.getResults().stream()).collect(Collectors.toList());
            this.processResults(
                    group.getKey(),
                    results,
                    () -> group.getValue().forEach(x -> x.getLbCommitCountdown().notifyMessageProcessed())
            );
        }

        log.info("Adding results to report queue");

        // Async result reporting for batching (less writes) and speed.
        // Small chance of skipping some reports on reboot.
        // Correct state remains in db, divergence will be fixed on next report / finalization / reader restart.
        aggregateReporter.enqueue(byAggregateId.keySet());
        aggregateReporter.enqueue(
                byAggregateId.keySet().stream()
                        .filter(id -> id.getIterationId().getNumber() > 1)
                        .map(ChunkAggregateEntity.Id::toIterationMetaId)
                        .collect(Collectors.toSet())
        );
    }

    private void processResults(
            ChunkAggregateEntity.Id aggregateId, List<TestResult> results, @Nullable Runnable commitCallback
    ) {
        var diffs = processResultsRetryable(aggregateId, results);

        var postMessage = new PostProcessMessage(aggregateId.getChunkId(), results, diffs, commitCallback);
        if (commitCallback != null) {
            this.postProcessor.enqueue(postMessage);
        } else {
            // Without callback, we have to process it synchronously.
            this.postProcessor.process(List.of(postMessage));
        }
    }

    private List<TestDiffByHashEntity> processResultsRetryable(ChunkAggregateEntity.Id aggregateId,
                                                               List<TestResult> results) {
        var trace = timeTraceService.createTrace("aggregate_pool_processor");

        var batches = Lists.partition(results, AGGREGATES_CALCULATION_BATCH_LIMIT);
        log.info(
                "Processing {} results for {} in {} batches",
                results.size(), aggregateId, batches.size()
        );

        var diffsByHash = new HashMap<TestDiffByHashEntity.Id, TestDiffByHashEntity>();
        for (var batch : batches) {
            retryUntilInterruptedOrSucceeded(
                    () -> shardCache.modifyAndGet(
                            cache -> calculateAggregateInCacheTx(aggregateId, batch, cache, trace)
                    ).forEach(diff -> diffsByHash.put(diff.getId(), diff))
            );
        }

        log.info("All message batches processed for {}, number of diffs: {}", aggregateId, diffsByHash.size());

        trace.step("aggregates_inserted");

        this.statistics.onResultsProcessed(aggregateId.getChunkId(), results.size());

        log.info("Aggregate pool timings: {}", trace.logString());

        return new ArrayList<>(diffsByHash.values());
    }

    private void insertToIndex(
            ChunkAggregateEntity.Id aggregateId, TimeTraceService.Trace trace,
            List<TestDiffByHashEntity> diffsByHash
    ) {
        var diffs = diffsByHash.stream()
                .map(TestDiffEntity::new)
                .collect(Collectors.toList());

        if (diffs.size() > 0) {
            log.info("Bulk inserting {} diffs for {}", aggregateId, diffs.size());
            this.db.currentOrTx(
                    () -> this.db.testDiffs().bulkUpsertWithRetries(
                            diffs, BULK_LIMIT, (e) -> this.statistics.onBulkInsertError()
                    )
            );
            trace.step("diffs_by_iteration_inserted");
        }

        var diffsBySuite = diffsByHash.stream()
                .map(TestDiffBySuiteEntity::new)
                .collect(Collectors.toList());

        if (diffsBySuite.size() > 0) {
            log.info("Bulk inserting {} diffs by suite for {}", aggregateId, diffs.size());
            this.db.currentOrTx(
                    () -> this.db.testDiffsBySuite().bulkUpsertWithRetries(
                            diffsBySuite, BULK_LIMIT, (e) -> this.statistics.onBulkInsertError()
                    )
            );
            trace.step("diffs_by_iteration_inserted");
        }

        var importantDiffs = diffsByHash.stream()
                .filter(TestDiffByHashEntity::isImportant)
                .map(TestDiffImportantEntity::new)
                .collect(Collectors.toList());

        if (importantDiffs.size() > 0) {
            log.info("Bulk inserting {} important diffs for {}", aggregateId, diffs.size());
            this.db.currentOrTx(
                    () -> this.db.importantTestDiffs().bulkUpsertWithRetries(
                            importantDiffs, BULK_LIMIT, (e) -> this.statistics.onBulkInsertError()
                    )
            );
            trace.step("important_diffs_by_iteration_inserted");
        }
    }

    private List<TestDiffByHashEntity> calculateAggregateInCacheTx(
            ChunkAggregateEntity.Id aggregateId,
            List<TestResult> results,
            ShardCache.Modifiable cache,
            TimeTraceService.Trace trace
    ) {
        var originalAggregate = cache.chunkAggregates().getOrDefault(aggregateId);
        if (originalAggregate.getState().equals(Common.ChunkAggregateState.CAS_COMPLETED)) {
            log.warn(
                    "Ignoring results for finished aggregate: {}, finished {}, results: {}",
                    aggregateId,
                    originalAggregate.getFinished(),
                    results.stream()
                            .map(x -> "%s of type %s, status %s, from task %s".formatted(
                                            x.getId().getFullTestId(), x.getResultType(), x.getStatus(),
                                            x.getId().getTaskId()
                                    )
                            )
                            .collect(Collectors.joining(", "))
            );
            this.statistics.onFinishedStateError();
            return List.of();
        }

        if (originalAggregate.getState().equals(Common.ChunkAggregateState.CAS_CANCELLED)) {
            log.info("Ignoring results for cancelled aggregate: {}", aggregateId);
            return List.of();
        }

        var metaAggregateOptional = cache.chunkAggregates().get(aggregateId.toIterationMetaId());
        var metaAggregateMutable = (ChunkAggregateEntity.ChunkAggregateEntityMutable) null;

        if (metaAggregateOptional.isEmpty() && originalAggregate.getId().getIterationId().getNumber() > 1) {
            var aggregateIdToCopy = aggregateId.toFirstIterationId();
            log.info("Cloning aggregate {} to meta iteration", aggregateIdToCopy);

            metaAggregateMutable = cache.chunkAggregates().getOrDefault(aggregateIdToCopy).toMutable();
            metaAggregateMutable.setId(metaAggregateMutable.getId().toIterationMetaId());
        } else {
            metaAggregateMutable = metaAggregateOptional.map(ChunkAggregateEntity::toMutable).orElse(null);
            if (metaAggregateMutable != null && metaAggregateMutable.isFinished()) {
                metaAggregateMutable.setFinished(null);
            }
        }

        var aggregateMutable = originalAggregate.toMutable();
        for (var result : results) {
            aggregateProcessor.process(cache, aggregateMutable, metaAggregateMutable, result);
        }

        trace.step("aggregates_calculated");

        var modified = new ArrayList<>(cache.testDiffs().getAffected());
        if (modified.isEmpty()) {
            log.info("No diffs for {}", aggregateId);
            return modified;
        }

        insertToIndex(aggregateId, trace, modified);

        log.info("Saving {} diffs for {}", modified.size(), aggregateId);
        var updatedAggregate = aggregateMutable.toImmutable();
        var updatedMetaAggregate = metaAggregateMutable == null ? null : metaAggregateMutable.toImmutable();

        // We can use short transaction here, separated from calculation, as we use aggregates from cache anyway.
        // We can't bulk insert here because we need consistency between aggregates and diffs.
        retryUntilInterruptedOrSucceeded(
                () -> this.db.currentOrTx(() -> {
                            if (updatedMetaAggregate != null) {
                                this.db.chunkAggregates().save(updatedMetaAggregate);
                            }
                            this.db.chunkAggregates().save(updatedAggregate);
                            this.db.testDiffsByHash().save(modified);
                        }
                )
        );

        log.info("Saved {} diffs for {}", modified.size(), aggregateId);

        if (updatedMetaAggregate != null) {
            cache.chunkAggregates().put(updatedMetaAggregate);
        }
        cache.chunkAggregates().put(updatedAggregate);

        return modified;
    }

    public void enqueue(AggregateMessage message) {
        if (numberOfAggregateQueues > 0) {
            var queueNumber = getQueueNumber(
                    message.getAggregateId().getIterationId().getCheckId(),
                    message.getAggregateId().getIterationId().getIterationType(),
                    message.getAggregateId().getChunkId()
            );

            aggregatesQueues.get(queueNumber).add(message);
        } else {
            process(List.of(message));
        }
    }

    class AggregatePoolWorker extends QueueWorker<AggregateMessage> {

        AggregatePoolWorker(BlockingQueue<AggregateMessage> queue, int drainLimit) {
            super(queue, drainLimit);
        }

        @Override
        public void onDrain(int amount) {
            statistics.onAggregateQueueDrain(amount);
        }

        @Override
        public void process(List<AggregateMessage> items) {
            AggregatePoolProcessor.this.process(items);
        }

        @Override
        public void onFailed() {
            statistics.onAggregateWorkerError();
        }
    }

    private int getQueueNumber(
            CheckEntity.Id checkId,
            CheckIteration.IterationType iterationType,
            ChunkEntity.Id chunkId
    ) {
        var hash = Hashing.sipHash24().newHasher()
                .putLong(checkId.getId())
                .putInt(iterationType.getNumber())
                .putInt(chunkId.getChunkType().getNumber())
                .putInt(chunkId.getNumber())
                .hash()
                .asInt();

        return Math.abs(hash % aggregatesQueues.size());
    }
}
