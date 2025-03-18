package ru.yandex.ci.storage.shard.message;

import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

import com.google.common.annotations.VisibleForTesting;
import io.micrometer.core.instrument.Gauge;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.ayamler.StrongModeRequest;
import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.project.AutocheckProject;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.ShardIn;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.StrongModePolicy;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.db.model.check_text_search.CheckTextSearchEntity;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.task_result.TestResult;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultEntity;
import ru.yandex.ci.storage.core.db.model.task_result.TestResultYamlInfo;
import ru.yandex.ci.storage.core.db.model.test_status.TestStatusEntity;
import ru.yandex.ci.storage.core.exceptions.CheckTaskNotFoundException;
import ru.yandex.ci.storage.core.micrometr.StorageMetrics;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.core.utils.NumberOfMessagesFormatter;
import ru.yandex.ci.storage.core.utils.TimeTraceService;
import ru.yandex.ci.storage.shard.ShardStatistics;
import ru.yandex.ci.storage.shard.cache.ShardCache;
import ru.yandex.ci.storage.shard.task.TestResultConverter;
import ru.yandex.ci.util.ExceptionUtils;
import ru.yandex.ci.util.Retryable;
import ru.yandex.ci.util.queue.QueueExecutor;
import ru.yandex.ci.util.queue.QueueWorker;
import ru.yandex.ci.util.queue.SyncQueueExecutor;
import ru.yandex.ci.util.queue.ThreadPerQueueExecutor;

@Slf4j
public class ChunkPoolProcessor {
    private static final int BULK_LIMIT = 4096;

    private final CiStorageDb db;
    private final ShardCache shardCache;
    private final TimeTraceService timeTraceService;
    private final AggregatePoolProcessor aggregatePoolProcessor;
    private final ShardStatistics statistics;
    private final String queueSizeMetricName;
    private final int chunkQueueDrainLimit;
    private final QueueExecutor<ChunkPoolMessage> executor;

    public ChunkPoolProcessor(
            CiStorageDb db,
            ShardCache shardCache,
            TimeTraceService timeTraceService,
            AggregatePoolProcessor aggregatePoolProcessor,
            ShardStatistics statistics,
            int chunkQueueCapacity,
            int chunkQueueDrainLimit,
            boolean syncMode
    ) {
        this.executor = syncMode ?
                new SyncQueueExecutor<>(this::process) :
                new ThreadPerQueueExecutor<>("chunk-pool", chunkQueueCapacity, this::createQueueWorker);

        this.db = db;
        this.shardCache = shardCache;
        this.timeTraceService = timeTraceService;
        this.aggregatePoolProcessor = aggregatePoolProcessor;
        this.statistics = statistics;
        this.chunkQueueDrainLimit = chunkQueueDrainLimit;

        this.queueSizeMetricName = StorageMetrics.PREFIX + "chunk_pool_queue_size";

        statistics.register(
                Gauge.builder(queueSizeMetricName, this.executor::getQueueSize)
                        .tag(StorageMetrics.QUEUE, "all")
        );
    }

    public void enqueue(ChunkPoolMessage message) {
        this.executor.enqueue(message.getChunkId().toString(), message);
    }

    @VisibleForTesting
    void process(List<ChunkPoolMessage> messages) {
        var trace = timeTraceService.createTrace("chunk_pool_processor");

        var now = Instant.now();

        var messagesByCase = messages.stream()
                .collect(Collectors.groupingBy(m -> m.getChunkMessage().getMessageCase()));

        log.info(
                "Processing chunk messages: {}, chunk lags: {}",
                NumberOfMessagesFormatter.format(messagesByCase),
                messages.stream().collect(Collectors.groupingBy(ChunkPoolMessage::getChunkId)).entrySet().stream()
                        .map(x -> "%s - %ss".formatted(x.getKey(), getMessageLag(now, x.getValue().get(0)).toSeconds()))
                        .collect(Collectors.joining(", "))
        );

        processResult(messagesByCase.getOrDefault(ShardIn.ChunkMessage.MessageCase.RESULT, List.of()), trace);
        processFinish(messagesByCase.getOrDefault(ShardIn.ChunkMessage.MessageCase.FINISH, List.of()), trace);

        messagesByCase.getOrDefault(ShardIn.ChunkMessage.MessageCase.MESSAGE_NOT_SET, List.of())
                .forEach(m -> m.getCommitCountdown().notifyMessageProcessed());

        this.statistics.onMessageProcessed(ShardIn.ShardInMessage.MessageCase.CHUNK, messages.size());
    }

    private void processFinish(List<ChunkPoolMessage> messages, TimeTraceService.Trace trace) {
        if (messages.isEmpty()) {
            return;
        }

        log.info("Processing finish messages: {}", messages.size());

        Retryable.retryUntilInterruptedOrSucceeded(
                () -> {
                    var aggregateMessages = processFinishInternal(messages);

                    for (var message : aggregateMessages) {
                        aggregatePoolProcessor.enqueue(message);
                        log.info("Finishing chunk {}", message.getAggregateId());
                    }
                }, t -> this.statistics.onChunkWorkerError()
        );

        trace.step("finish_processed");
    }

    private ArrayList<AggregateMessage> processFinishInternal(List<ChunkPoolMessage> messages) {
        var aggregateMessages = new ArrayList<AggregateMessage>();
        var tests = new HashSet<TestStatusEntity.Id>();
        for (var message : messages) {
            var aggregateId = new ChunkAggregateEntity.Id(
                    CheckProtoMappers.toIterationId(message.getChunkMessage().getFinish().getIterationId()),
                    message.getChunkId()
            );

            var aggregate = this.shardCache.chunkAggregates().getOrDefault(aggregateId);
            var diffs = aggregate.getStatistics().isEmpty() ?
                    this.shardCache.testDiffs().getForEmpty(aggregate.getId()) :
                    this.shardCache.testDiffs().get(aggregate.getId());

            tests.addAll(
                    diffs.getAll().stream()
                            .map(x -> TestStatusEntity.Id.idInTrunk(x.getId().getTestId()))
                            .toList()
            );

            aggregateMessages.add(
                    new AggregateMessage(
                            aggregateId,
                            message.getCommitCountdown(),
                            null,
                            new AggregateMessage.Finish(message.getChunkMessage().getFinish().getState())
                    )
            );
        }

        log.info("Warming tests cache");
        this.shardCache.muteStatus().get(tests);
        return aggregateMessages;
    }

    private Duration getMessageLag(Instant now, ChunkPoolMessage message) {
        return Duration.between(ProtoConverter.convert(message.getMeta().getTimestamp()), now);
    }

    private void processResult(List<ChunkPoolMessage> messages, TimeTraceService.Trace trace) {
        if (messages.isEmpty()) {
            return;
        }

        Retryable.retryUntilInterruptedOrSucceeded(
                () -> processResultsInternal(messages, trace), t -> this.statistics.onChunkWorkerError()
        );
    }

    private void processResultsInternal(List<ChunkPoolMessage> messages, TimeTraceService.Trace trace) {
        log.info("Processing result messages: {}", messages.size());

        var messagesWithIds = messages.stream().map(ChunkMessageWithIds::from).collect(Collectors.toList());

        trace.step("parsed");

        warmMetaCache(messagesWithIds);

        trace.step("meta_cache_warmed");

        var withResults = fromResults(messagesWithIds);

        if (withResults.messagesWithResults.isEmpty()) {
            log.info("All results skipped");
            withResults.skippedMessages.forEach(ChunkPoolMessage::notifyMessageProcessed);
            return;
        }

        var allResults = withResults.messagesWithResults.stream()
                .flatMap(x -> x.getResults().stream())
                .toList();

        this.shardCache.muteStatus().syncMuteActionsIfNeeded();

        this.shardCache.muteStatus().get(
                allResults.stream()
                        .map(r -> TestStatusEntity.Id.idInTrunk(r.getId().getFullTestId()))
                        .collect(Collectors.toSet())
        );

        trace.step("tests_cache_warmed");

        var iterationToStrongModePolicy = this.shardCache.iterations()
                .get(
                        withResults.messagesWithResults.stream()
                                .map(ChunkMessageWithResults::getTask)
                                .map(CheckTaskEntity::getId)
                                .map(CheckTaskEntity.Id::getIterationId)
                                .collect(Collectors.toSet())
                )
                .stream()
                .collect(Collectors.toMap(
                        CheckIterationEntity::getId,
                        it -> it.getInfo().getStrongModePolicy()
                ));
        trace.step("strong_mode_source_fetched");

        var strongModeFutureResponse = sendStrongModeRequest(withResults.messagesWithResults);

        trace.step("strong_mode_requested");

        log.info("Bulk inserting {} task results", allResults.size());
        this.db.currentOrTx(
                () -> this.db.testResults().bulkUpsertWithRetries(
                        allResults.stream()
                                .map(TestResultEntity::new)
                                .collect(Collectors.toList()),
                        BULK_LIMIT,
                        (e) -> this.statistics.onBulkInsertError()
                )
        );
        trace.step("results_inserted");

        this.statistics.onResultsInserted(allResults.size());

        var textSearch = allResults.stream()
                .flatMap(x -> CheckTextSearchEntity.index(x).stream())
                .filter(x -> !shardCache.checkTextSearch().contains(x.getId()))
                .toList();

        this.db.currentOrTx(
                () -> this.db.checkTextSearch().bulkUpsertWithRetries(
                        textSearch, BULK_LIMIT, (e) -> this.statistics.onBulkInsertError()
                )
        );

        shardCache.checkTextSearch().put(textSearch);

        trace.step("text_search_inserted");

        var strongModeResponse = waitStrongModeFutures(strongModeFutureResponse);
        trace.step("strong_mode_received");

        var messagesWithStrongModes = withResults.messagesWithResults.stream().map(
                message -> chunkMessageWithStrongMode(message, strongModeResponse, iterationToStrongModePolicy)
        ).toList();

        for (var message : messagesWithStrongModes) {
            // Each message must be enqueued once, so we must not fail and retry this method later.
            aggregatePoolProcessor.enqueue(
                    new AggregateMessage(
                            message.getAggregateId(),
                            message.getLbCommitCountdown(),
                            message,
                            null
                    )
            );
        }

        withResults.skippedMessages.forEach(ChunkPoolMessage::notifyMessageProcessed);

        trace.step("result_processed");

        log.info("Chunk pool timings: {}", trace.logString());
    }

    private StrongModeBatchResponse sendStrongModeRequest(List<ChunkMessageWithResults> messagesWithResults) {
        var requests = messagesWithResults.stream()
                .flatMap(it -> it.getResults().stream().map(r -> toStrongModeRequest(r, it)))
                .collect(Collectors.toSet());

        log.info("Strong mode requests {}", requests.size());

        var responseFuture = sendStrongModeRequest(requests);
        return new StrongModeBatchResponse(requests, responseFuture);
    }

    private CompletableFuture<Map<StrongModeRequest, TestResultYamlInfo>> sendStrongModeRequest(
            Set<StrongModeRequest> requests
    ) {
        try {
            log.info("Request {} strong modes", requests.size());
            return this.shardCache.strongModeCache().getStrongMode(requests);
        } catch (Exception e) {
            statistics.onAYamlerClientError();
            boolean skipAYamlerClientErrors = this.shardCache.settings().get().getShard()
                    .getSkip()
                    .isAYamlerClientErrors();
            if (!skipAYamlerClientErrors) {
                throw e;
            }
            return CompletableFuture.failedFuture(e);
        }
    }

    private Map<StrongModeRequest, TestResultYamlInfo> waitStrongModeFutures(StrongModeBatchResponse responseFuture) {
        try {
            /* We don't call `.get(timeout, TimeUnit)`, cause timeout is set inside AYamlClient (`withDeadlineAfter`).
               When timeout happens, we get `ExecutionException` with cause
               StatusRuntimeException(status=Status.DEADLINE_EXCEEDED)`. */
            return responseFuture.getResponse().get();
        } catch (InterruptedException e) {
            statistics.onAYamlerClientError();
            throw new RuntimeException("Interrupted", e);
        } catch (Exception e) {
            statistics.onAYamlerClientError();
            log.error("Failed getting strong mode: {}", responseFuture.getRequestedStrongModes(), e);

            var skipAYamlerClientErrors = this.shardCache.settings().get().getShard()
                    .getSkip()
                    .isAYamlerClientErrors();

            if (!skipAYamlerClientErrors) {
                throw ExceptionUtils.unwrap(e);
            }

            return responseFuture.getRequestedStrongModes().stream().collect(
                    Collectors.toMap(it -> it, it -> TestResultYamlInfo.disabled(AutocheckProject.NAME))
            );
        }
    }

    private void warmMetaCache(List<ChunkMessageWithIds> messagesWithIds) {
        this.shardCache.checks().get(
                messagesWithIds.stream().map(ChunkMessageWithIds::getCheckId).collect(Collectors.toSet())
        );

        this.shardCache.checkTasks().get(
                messagesWithIds.stream().map(ChunkMessageWithIds::getTaskId).collect(Collectors.toSet())
        );

        var iterationAggregateIds = messagesWithIds.stream()
                .map(ChunkMessageWithIds::getAggregateId)
                .flatMap(x -> IntStream.range(0, x.getIterationId().getNumber() + 1).mapToObj(x::toIterationId))
                .collect(Collectors.toSet());

        var aggregates = this.shardCache.chunkAggregates().get(iterationAggregateIds).stream()
                .collect(Collectors.toMap(ChunkAggregateEntity::getId, Function.identity()));

        for (var id : iterationAggregateIds) {
            var aggregate = aggregates.get(id);
            if (aggregate == null || aggregate.getStatistics().isEmpty()) {
                this.shardCache.testDiffs().getForEmpty(id);
            } else {
                this.shardCache.testDiffs().get(id);
            }
        }
    }

    private WithResultsOrSkipped fromResults(List<ChunkMessageWithIds> messagesWithIds) {
        var settings = this.shardCache.settings().get().getShard();

        var result = new WithResultsOrSkipped(messagesWithIds.size());

        for (var messageWithIds : messagesWithIds) {
            var checkOptional = this.shardCache.checks().get(messageWithIds.checkId);
            if (checkOptional.isEmpty()) {
                this.statistics.onMissingError();

                if (settings.getSkip().isMissing()) {
                    log.warn("Skipping check not found: {}", messageWithIds.getTaskId());
                    result.skippedMessages.add(messageWithIds.getMessage());
                    continue;
                } else {
                    throw new CheckTaskNotFoundException(messageWithIds.getTaskId().toString());
                }
            }

            var check = checkOptional.get();

            var checkTaskOptional = this.shardCache.checkTasks().get(messageWithIds.taskId);
            if (checkTaskOptional.isEmpty()) {
                this.statistics.onMissingError();

                if (settings.getSkip().isMissing()) {
                    log.warn("Skipping task not found: {}", messageWithIds.getTaskId());
                    result.skippedMessages.add(messageWithIds.getMessage());
                    continue;
                } else {
                    throw new CheckTaskNotFoundException(messageWithIds.getTaskId().toString());
                }
            }

            var checkTask = checkTaskOptional.get();

            var branch = ArcBranch.ofString(
                    checkTask.isRight() ? check.getRight().getBranch() : check.getLeft().getBranch()
            );

            var revision = checkTask.isRight() ? check.getRight().getRevision() :
                    check.getLeft().getRevision();

            var revisionNumber = checkTask.isRight() ? check.getRight().getRevisionNumber() :
                    check.getLeft().getRevisionNumber();

            var taskResults = TestResultConverter.convert(
                    new TestResultConverter.Context(
                            checkTaskOptional.get().getId().getTaskId(),
                            branch.asString(),
                            revision,
                            revisionNumber,
                            messageWithIds.getMessage().getChunkMessage().getResult().getPartition(),
                            checkTaskOptional.get().isRight(),
                            messageWithIds.getAggregateId()
                    ),
                    messageWithIds.getMessage().getChunkMessage().getResult().getAutocheckTestResults()
            );

            result.messagesWithResults.add(
                    new ChunkMessageWithResults(
                            check,
                            checkTaskOptional.get(),
                            branch,
                            revisionNumber,
                            messageWithIds.aggregateId,
                            messageWithIds.message.getCommitCountdown(),
                            taskResults
                    )
            );
        }

        return result;
    }

    private QueueWorker<ChunkPoolMessage> createQueueWorker(
            String queueNumber,
            LinkedBlockingQueue<ChunkPoolMessage> queue
    ) {
        statistics.register(Gauge.builder(queueSizeMetricName, queue::size).tag(StorageMetrics.QUEUE, queueNumber));
        return new ChunkPoolWorker(queue, this.statistics, chunkQueueDrainLimit);
    }

    private static ChunkMessageWithResults chunkMessageWithStrongMode(
            ChunkMessageWithResults message,
            Map<StrongModeRequest, TestResultYamlInfo> strongModeResponse,
            Map<CheckIterationEntity.Id, StrongModePolicy> iterationToStrongModePolicy
    ) {
        var resultsWithStrongMode = message.getResults().stream()
                .map(taskResult -> {
                    var strongModeRequest = toStrongModeRequest(taskResult, message);
                    var strongMode = strongModeResponse.get(strongModeRequest);
                    if (strongMode == null) {
                        throw new RuntimeException("Not found for " + strongModeRequest);
                    }

                    var policy = iterationToStrongModePolicy.get(taskResult.getId().getIterationId());
                    var forceOn = policy == StrongModePolicy.FORCE_ON;
                    var forceOff = policy == StrongModePolicy.FORCE_OFF;

                    var builder = taskResult.toBuilder()
                            .service(strongMode.getService())
                            .isOwner(strongMode.isOwner());

                    if (!forceOff && (strongMode.getStatus() == TestResultYamlInfo.StrongModeStatus.ON || forceOn)) {
                        builder = builder
                                .isStrongMode(true)
                                .strongModeAYaml(strongMode.getAYamlPath());
                    }

                    return builder.build();
                })
                .collect(Collectors.toList());

        return message.withResults(resultsWithStrongMode);
    }

    private static StrongModeRequest toStrongModeRequest(TestResult testResult, ChunkMessageWithResults message) {
        return new StrongModeRequest(
                testResult.getPath(),
                message.getCheck().getRight().getRevision(),
                message.getCheck().getAuthor()
        );
    }

    @Value
    private static class ChunkMessageWithIds {
        CheckEntity.Id checkId;
        ChunkAggregateEntity.Id aggregateId;
        CheckTaskEntity.Id taskId;
        ChunkPoolMessage message;

        public static ChunkMessageWithIds from(ChunkPoolMessage message) {
            var shardMessage = message.getChunkMessage().getResult();
            var requestIterationId = shardMessage.getFullTaskId().getIterationId();
            var checkId = CheckEntity.Id.of(requestIterationId.getCheckId());
            var iterationId = CheckIterationEntity.Id.of(
                    checkId,
                    CheckIteration.IterationType.valueOf(requestIterationId.getCheckType().name()),
                    requestIterationId.getNumber()
            );

            var aggregateId = new ChunkAggregateEntity.Id(iterationId, message.getChunkId());

            var checkTaskId = new CheckTaskEntity.Id(
                    aggregateId.getIterationId(), shardMessage.getFullTaskId().getTaskId()
            );

            return new ChunkMessageWithIds(checkId, aggregateId, checkTaskId, message);
        }
    }


    @Value
    private static class WithResultsOrSkipped {
        ArrayList<ChunkMessageWithResults> messagesWithResults;
        ArrayList<ChunkPoolMessage> skippedMessages;

        WithResultsOrSkipped(int size) {
            this.messagesWithResults = new ArrayList<>(size);
            this.skippedMessages = new ArrayList<>();
        }
    }

    class ChunkPoolWorker extends QueueWorker<ChunkPoolMessage> {
        private final ShardStatistics statistics;

        ChunkPoolWorker(
                BlockingQueue<ChunkPoolMessage> queue,
                ShardStatistics statistics,
                int drainLimit
        ) {
            super(queue, drainLimit);
            this.statistics = statistics;
        }

        @Override
        public void onDrain(int amount) {
            this.statistics.onChunkQueueDrain(amount);
        }

        @Override
        public void process(List<ChunkPoolMessage> items) {
            ChunkPoolProcessor.this.process(items);
        }

        @Override
        public void onFailed() {
            this.statistics.onChunkWorkerError();
        }
    }

    @Value
    private static class StrongModeBatchResponse {
        Set<StrongModeRequest> requestedStrongModes;
        CompletableFuture<Map<StrongModeRequest, TestResultYamlInfo>> response;

        public static StrongModeBatchResponse empty() {
            return new StrongModeBatchResponse(Set.of(), CompletableFuture.completedFuture(Map.of()));
        }
    }
}
