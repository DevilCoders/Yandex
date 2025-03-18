package ru.yandex.ci.storage.reader.message.main;

import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Collectors;

import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.ShardOut;
import ru.yandex.ci.storage.core.TaskMessages;
import ru.yandex.ci.storage.core.TaskMessages.TaskMessage.MessagesCase;
import ru.yandex.ci.storage.core.db.constant.CheckStatusUtils;
import ru.yandex.ci.storage.core.db.constant.TestTypeUtils;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.db.model.check_task_statistics.CheckTaskStatistics;
import ru.yandex.ci.storage.core.db.model.check_task_statistics.CheckTaskStatisticsEntity;
import ru.yandex.ci.storage.core.db.model.chunk.ChunkEntity;
import ru.yandex.ci.storage.core.db.model.settings.ReaderSettings;
import ru.yandex.ci.storage.core.db.model.skipped_check.SkippedCheckEntity;
import ru.yandex.ci.storage.core.exceptions.CheckNotFoundException;
import ru.yandex.ci.storage.core.exceptions.CheckTaskNotFoundException;
import ru.yandex.ci.storage.core.exceptions.IterationNotFoundException;
import ru.yandex.ci.storage.core.message.shard_out.ShardOutMessageWriter;
import ru.yandex.ci.storage.core.message.utils.ForwardMessageUtils;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.core.sharding.ShardingSettings;
import ru.yandex.ci.storage.core.utils.OptionalWaiter;
import ru.yandex.ci.storage.core.utils.TimeTraceService;
import ru.yandex.ci.storage.reader.cache.ReaderCache;
import ru.yandex.ci.storage.reader.check.CheckFinalizationService;
import ru.yandex.ci.storage.reader.check.ReaderCheckService;
import ru.yandex.ci.storage.reader.exceptions.ReaderValidationException;
import ru.yandex.ci.storage.reader.export.TestsExporter;
import ru.yandex.ci.storage.reader.message.writer.ShardInMessageWriter;
import ru.yandex.ci.util.HostnameUtils;

@Slf4j
public class MainStreamMessageProcessor extends ru.yandex.ci.storage.core.message.main.MainStreamMessageProcessor {
    private static final Set<String> FORWARD_TRACES = Set.of(
            "distbuild/started"
    );

    private final ReaderCheckService checkService;
    private final CheckFinalizationService checkFinalizationService;
    private final TaskResultDistributor taskResultDistributor;
    private final ShardInMessageWriter shardInMessageWriter;
    private final ShardOutMessageWriter shardOutMessageWriter;
    private final TestsExporter exporter;
    private final ReaderStatistics statistics;
    private final ShardingSettings defaultShardingSettings;

    // We can not fully rely on cache as api and other readers can make modifications to db.
    // We can rely on cache for task statistics, as it divided by hosts in db.
    private final ReaderCache readerCache;

    private final int waitForRegistrationSeconds;
    private final boolean mustWaitForRegistration;

    public MainStreamMessageProcessor(
            ReaderCheckService checkService,
            CheckFinalizationService checkFinalizationService,
            TaskResultDistributor taskResultDistributor,
            ShardInMessageWriter shardInMessageWriter,
            ShardOutMessageWriter shardOutMessageWriter,
            TestsExporter exporter,
            ReaderStatistics statistics,
            ReaderCache readerCache,
            ShardingSettings defaultShardingSettings,
            Duration waitForRegistrationSeconds
    ) {
        this.checkService = checkService;
        this.checkFinalizationService = checkFinalizationService;
        this.taskResultDistributor = taskResultDistributor;
        this.shardInMessageWriter = shardInMessageWriter;
        this.shardOutMessageWriter = shardOutMessageWriter;
        this.exporter = exporter;
        this.statistics = statistics;
        this.readerCache = readerCache;
        this.waitForRegistrationSeconds = (int) waitForRegistrationSeconds.toSeconds();
        this.defaultShardingSettings = defaultShardingSettings;
        this.mustWaitForRegistration = this.waitForRegistrationSeconds > 0;
    }

    @Override
    public void process(List<TaskMessages.TaskMessage> protoTaskMessages, TimeTraceService.Trace trace) {
        var settings = this.readerCache.settings().get().getReader();
        var skipSettings = settings.getSkip();

        var taskMessages = convert(protoTaskMessages, skipSettings);
        if (taskMessages.isEmpty()) {
            log.info("All messages skipped");
            return;
        }

        var messagesByCase = taskMessages.stream()
                .collect(Collectors.groupingBy(m -> m.getMessage().getMessagesCase()));

        log.info(
                "Task messages by type: {}",
                messagesByCase.entrySet().stream()
                        .map(x -> "%s - %d".formatted(x.getKey(), x.getValue().size()))
                        .collect(Collectors.joining(", "))
        );

        processFatalError(
                messagesByCase.getOrDefault(MessagesCase.AUTOCHECK_FATAL_ERROR, List.of())
        );

        var forwarding = new ArrayList<CheckTaskMessage>();
        forwarding.addAll(messagesByCase.getOrDefault(MessagesCase.PESSIMIZE, List.of()));
        forwarding.addAll(messagesByCase.getOrDefault(MessagesCase.TEST_TYPE_FINISHED, List.of()));
        forwarding.addAll(messagesByCase.getOrDefault(MessagesCase.TEST_TYPE_SIZE_FINISHED, List.of()));
        forwarding.addAll(messagesByCase.getOrDefault(MessagesCase.METRIC, List.of()));
        forwarding.addAll(
                messagesByCase.getOrDefault(MessagesCase.TRACE_STAGE, List.of()).stream()
                        .filter(x -> FORWARD_TRACES.contains(x.getMessage().getTraceStage().getType()))
                        .toList()

        );

        processForwarding(forwarding);

        trace.step("forwarding_processed");

        var distribution = processAutocheckResult(
                messagesByCase.getOrDefault(MessagesCase.AUTOCHECK_TEST_RESULTS, List.of()),
                trace
        );

        trace.step("results_processed");

        saveStatistics(taskMessages, distribution);

        // Finish must be processed after all others messages
        processForwarding(messagesByCase.getOrDefault(MessagesCase.FINISHED, List.of()));

        processForwarding(forwarding);

        trace.step("finished_processed");

        for (var entry : messagesByCase.entrySet()) {
            this.statistics.getMain().onMessageProcessed(entry.getKey(), entry.getValue().size());
        }
    }

    private void saveStatistics(
            List<CheckTaskMessage> taskMessages, TaskResultDistributor.Result distribution
    ) {
        log.info("Processing tasks statistics");

        var messagesByStatisticsId = taskMessages.stream().collect(
                Collectors.groupingBy(this::toCheckTaskStatisticsId)
        );

        var tasksStatistics = this.readerCache.taskStatistics().getOrDefault(messagesByStatisticsId.keySet())
                .stream().collect(Collectors.toMap(CheckTaskStatisticsEntity::getId, Function.identity()));

        var result = new ArrayList<CheckTaskStatisticsEntity>();
        for (var entry : messagesByStatisticsId.entrySet()) {
            var taskStatistics = tasksStatistics.get(entry.getKey());
            var statisticsBuilder = taskStatistics.getStatistics().toBuilder();

            if (taskStatistics.getStatistics().getFirstWrite() == null) {
                statisticsBuilder.firstWrite(Instant.now());
            }
            statisticsBuilder.lastWrite(Instant.now());

            var affectedChunks = distribution.getAffectedChunks().get(taskStatistics.getId().getTaskId());
            if (affectedChunks != null) {
                statisticsBuilder.affectedChunks(
                        getAffectedChunks(taskStatistics.getStatistics().getAffectedChunks(), affectedChunks)
                );
            }

            var affectedToolchains = distribution.getAffectedToolchains().get(taskStatistics.getId().getTaskId());
            if (affectedToolchains != null) {
                var updatedAffectedToolchains = new HashSet<>(taskStatistics.getStatistics().getAffectedToolchains());
                updatedAffectedToolchains.addAll(affectedToolchains);
                statisticsBuilder.affectedToolchains(updatedAffectedToolchains);
            }

            var updatedNumberOfMessagesByType = new HashMap<>(
                    taskStatistics.getStatistics().getNumberOfMessagesByType()
            );

            HashSet<Common.ChunkType> finishedChunkTypes = null;
            for (var message : entry.getValue()) {
                updatedNumberOfMessagesByType.compute(
                        message.getMessage().getMessagesCase().name(), (key, number) -> number == null ? 1 : number + 1
                );

                if (message.getMessage().getMessagesCase().equals(MessagesCase.TEST_TYPE_FINISHED)) {
                    if (finishedChunkTypes == null) {
                        finishedChunkTypes = new HashSet<>(taskStatistics.getStatistics().getFinishedChunkTypes());
                    }

                    finishedChunkTypes.addAll(
                            TestTypeUtils.toChunkType(
                                    message.getMessage().getTestTypeFinished().getTestType(), null
                            )
                    );
                } else if (message.getMessage().getMessagesCase().equals(MessagesCase.TEST_TYPE_SIZE_FINISHED)) {
                    if (finishedChunkTypes == null) {
                        finishedChunkTypes = new HashSet<>(taskStatistics.getStatistics().getFinishedChunkTypes());
                    }

                    finishedChunkTypes.addAll(
                            TestTypeUtils.toChunkType(
                                    message.getMessage().getTestTypeSizeFinished().getTestType(),
                                    message.getMessage().getTestTypeSizeFinished().getSize()
                            )
                    );
                }
            }

            if (finishedChunkTypes != null) {
                statisticsBuilder.finishedChunkTypes(finishedChunkTypes);
            }

            statisticsBuilder.numberOfMessagesByType(updatedNumberOfMessagesByType);

            result.add(taskStatistics.toBuilder().statistics(statisticsBuilder.build()).build());
        }

        checkService.updateTaskStatistics(result);
        log.info("Bulk inserted {} task statistics", result.size());
    }


    private CheckTaskStatistics.AffectedChunks getAffectedChunks(
            CheckTaskStatistics.AffectedChunks statistics,
            Set<ChunkEntity.Id> affected
    ) {
        var affectedChunks = statistics.toMutable();

        for (var chunkId : affected) {
            affectedChunks.getChunks()
                    .computeIfAbsent(chunkId.getChunkType(), t -> new HashSet<>())
                    .add(chunkId.getNumber());
        }

        return affectedChunks.toImmutable();
    }

    private CheckTaskStatisticsEntity.Id toCheckTaskStatisticsId(CheckTaskMessage message) {
        var task = this.readerCache.checkTasks().getOrThrow(message.getCheckTaskId());
        return new CheckTaskStatisticsEntity.Id(
                task.getId(), message.getMessage().getPartition(), HostnameUtils.getShortHostname()
        );
    }

    private void processFatalError(List<CheckTaskMessage> messages) {
        for (var taskMessage : messages) {
            var message = taskMessage.getMessage().getAutocheckFatalError();
            this.checkFinalizationService.processTaskFatalError(
                    taskMessage.getCheckTaskId(),
                    message.getMessage(),
                    message.getDetails(),
                    message.getSandboxTaskId()
            );
        }
    }

    private void processForwarding(List<CheckTaskMessage> messages) {
        if (messages.isEmpty()) {
            return;
        }

        var forwardingMessages = messages.stream()
                .map(CheckTaskMessage::getMessage)
                .map(this::toForwarding)
                .collect(Collectors.toList());

        log.info(
                "Sending {} forwarding messages to shards, message: [{}]",
                forwardingMessages.size(),
                forwardingMessages.stream()
                        .map(ForwardMessageUtils::formatForwardMessage)
                        .collect(Collectors.joining(", "))
        );

        this.shardOutMessageWriter.writeForwarding(forwardingMessages);
    }

    private ShardOut.ShardForwardingMessage toForwarding(TaskMessages.TaskMessage message) {
        var builder = ShardOut.ShardForwardingMessage.newBuilder()
                .setFullTaskId(message.getFullTaskId())
                .setPartition(message.getPartition());

        switch (message.getMessagesCase()) {
            case TRACE_STAGE -> builder.setTrace(message.getTraceStage());
            case TEST_TYPE_FINISHED -> builder.setTestTypeFinished(message.getTestTypeFinished());
            case TEST_TYPE_SIZE_FINISHED -> builder.setTestTypeSizeFinished(message.getTestTypeSizeFinished());
            case FINISHED -> builder.setFinished(message.getFinished());
            case PESSIMIZE -> builder.setPessimize(message.getPessimize());
            case METRIC -> builder.setMetric(message.getMetric());
            default -> throw new RuntimeException("Not forwarding message: " + message.getMessagesCase());
        }
        return builder.build();
    }

    private TaskResultDistributor.Result processAutocheckResult(
            List<CheckTaskMessage> messages,
            TimeTraceService.Trace trace
    ) {
        if (messages.isEmpty()) {
            return TaskResultDistributor.Result.EMPTY;
        }

        var distribution = taskResultDistributor.distribute(messages);

        if (distribution.getNumberOfResults() == 0) {
            log.info("All results have been removed by sampling");
            return distribution;
        }

        log.info(
                "Writing results for shards, writes: {}, messages: {}, results: {}, affected chunks: {}, tasks: [{}]",
                distribution.getMessagesToWrite().size(),
                messages.size(),
                distribution.getNumberOfResults(),
                distribution.getNumberOfChunks(),
                messages.stream()
                        .map(CheckTaskMessage::getCheckTaskId)
                        .map(CheckTaskEntity.Id::toString)
                        .collect(Collectors.joining(", "))
        );

        readerCache.modify(cache -> shardInMessageWriter.writeChunkMessages(cache, distribution.getMessagesToWrite()));

        trace.step("chunk_messages_sent");

        exporter.exportMessageAutocheckTestResults(messages);

        trace.step("test_results_exported");

        this.statistics.getMain().onResultsProcessed(distribution.getNumberOfResults());

        return distribution;
    }

    private List<CheckTaskMessage> convert(
            List<TaskMessages.TaskMessage> protoTaskMessages,
            ReaderSettings.SkipSettings skipSettings
    ) {
        var taskMessages = new ArrayList<CheckTaskMessage>(protoTaskMessages.size());
        for (var message : protoTaskMessages) {
            var fullTaskId = message.getFullTaskId();

            try {
                MessageValidator.validate(fullTaskId);
            } catch (ReaderValidationException e) {
                this.statistics.getMain().onValidationError();

                if (skipSettings.isValidationError()) {
                    log.warn(
                            "Skipping validation error: {}, fullTaskId: {}, message: {}",
                            e.getMessage(), fullTaskId, message.getMessagesCase()
                    );
                    continue;
                } else {
                    throw e;
                }
            }
            var checkId = CheckEntity.Id.of(fullTaskId.getIterationId().getCheckId());
            if (checkId.sampleOut(defaultShardingSettings)) {
                continue;
            }

            var skipped = this.readerCache.skippedChecks().get(SkippedCheckEntity.Id.of(checkId));

            var checkOptional = this.readerCache.checks().get(checkId);
            if (checkOptional.isEmpty() && this.mustWaitForRegistration && skipped.isEmpty()) {
                log.info("Will wait for missing check: {}", checkId);
                checkOptional = waitCheck(checkId);
            }

            if (checkOptional.isEmpty()) {
                this.statistics.getMain().onMissingError();

                if (skipSettings.isMissing()) {
                    log.warn("Skipping missing check: {}", checkId);

                    if (this.mustWaitForRegistration) {
                        checkService.skipCheck(checkId, "Check not found: " + checkId);
                    }

                    continue;
                } else {
                    throw new CheckNotFoundException(checkId.toString());
                }
            }

            var iterationId = CheckProtoMappers.toIterationId(fullTaskId.getIterationId());
            var iterationOptional = this.readerCache.iterations().get(iterationId);

            if (iterationOptional.isEmpty() && this.mustWaitForRegistration && skipped.isEmpty()) {
                log.info("Will wait for missing iteration: {}", iterationId);
                iterationOptional = waitIteration(iterationId);
            }

            if (iterationOptional.isEmpty()) {
                this.statistics.getMain().onMissingError();

                if (skipSettings.isMissing()) {
                    log.warn("Skipping missing iteration: {}", iterationId);

                    if (this.mustWaitForRegistration) {
                        checkService.skipCheck(checkId, "Iteration not found: " + iterationId);
                    }
                    continue;
                } else {
                    throw new IterationNotFoundException(iterationId.toString());
                }
            }

            var taskId = new CheckTaskEntity.Id(iterationId, fullTaskId.getTaskId());
            var taskOptional = this.readerCache.checkTasks().get(taskId);
            if (taskOptional.isEmpty() && this.mustWaitForRegistration && skipped.isEmpty()) {
                log.info("Will wait for missing task: {}", taskId);
                taskOptional = waitTask(taskId);
            }

            if (taskOptional.isEmpty()) {
                this.statistics.getMain().onMissingError();

                if (skipSettings.isMissing()) {
                    log.warn("Skipping missing task: {}", taskId);
                    if (this.mustWaitForRegistration) {
                        checkService.skipCheck(checkId, "Task not found: " + iterationId);
                    }
                    continue;
                } else {
                    throw new CheckTaskNotFoundException(taskId.toString());
                }
            }

            var iteration = iterationOptional.get();
            var task = taskOptional.get();

            if (CheckStatusUtils.isCancelled(iteration.getStatus())) {
                log.info(
                        "iteration {} is cancelled in status: {}, skipping message {}",
                        iteration.getId(), iteration.getStatus(), message.getMessagesCase()
                );
                continue;
            }

            if (CheckStatusUtils.isCancelled(task.getStatus())) {
                log.info(
                        "task {} is cancelled in status: {}, skipping message {}",
                        iteration.getId(), task.getStatus(), message.getMessagesCase()
                );
                continue;
            }

            if (CheckStatusUtils.isCompleted(iteration.getStatus())) {
                onSkipIteration(message, iteration);
                continue;
            }

            if (CheckStatusUtils.isCompleted(task.getStatus())) {
                onSkipFinishedTask(message, task);
                continue;
            }

            var shardingSettings = checkOptional.get().getShardingSettings();

            taskMessages.add(
                    new CheckTaskMessage(
                            task.getId(),
                            task.getType(),
                            message,
                            iteration.getChunkShift(),
                            shardingSettings == null ? defaultShardingSettings : shardingSettings
                    )
            );
        }

        return taskMessages;
    }

    private Optional<CheckEntity> waitCheck(CheckEntity.Id checkId) {
        return OptionalWaiter.waitForNoneEmpty(
                () -> this.readerCache.checks().getFresh(checkId),
                checkId::toString,
                1,
                waitForRegistrationSeconds
        );
    }

    private Optional<CheckIterationEntity> waitIteration(CheckIterationEntity.Id iterationId) {
        return OptionalWaiter.waitForNoneEmpty(
                () -> this.readerCache.iterations().getFresh(iterationId),
                iterationId::toString,
                1,
                waitForRegistrationSeconds
        );
    }

    private Optional<CheckTaskEntity> waitTask(CheckTaskEntity.Id taskId) {
        return OptionalWaiter.waitForNoneEmpty(
                () -> this.readerCache.checkTasks().getFresh(taskId),
                taskId::toString,
                1,
                waitForRegistrationSeconds
        );
    }

    private void onSkipFinishedTask(TaskMessages.TaskMessage message, CheckTaskEntity task) {
        log.info(
                "task {} is finished in status: {}, skipping message {} ",
                task.getId(), task.getStatus(), message.getMessagesCase()
        );

        if (message.getMessagesCase().equals(MessagesCase.AUTOCHECK_TEST_RESULTS)) {
            this.statistics.getMain().onFinishedStateError();
        }
    }

    private void onSkipIteration(TaskMessages.TaskMessage message, CheckIterationEntity iteration) {
        log.info(
                "iteration {} is finished in status: {}, skipping message {}",
                iteration.getId(), iteration.getStatus(), message.getMessagesCase()
        );

        if (!iteration.getStatus().equals(Common.CheckStatus.COMPLETED_WITH_FATAL_ERROR) &&
                message.getMessagesCase().equals(MessagesCase.AUTOCHECK_TEST_RESULTS)) {
            this.statistics.getMain().onFinishedStateError();
        }
    }
}
