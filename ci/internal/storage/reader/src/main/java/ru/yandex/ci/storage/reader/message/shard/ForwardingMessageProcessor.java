package ru.yandex.ci.storage.reader.message.shard;

import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import yandex.cloud.repository.db.exception.UnavailableException;

import ru.yandex.ci.storage.core.Actions;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.ShardOut;
import ru.yandex.ci.storage.core.ShardOut.ShardForwardingMessage.MessageCase;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.metrics.Metric;
import ru.yandex.ci.storage.core.db.model.check_iteration.metrics.Metrics;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.core.utils.TimeTraceService;
import ru.yandex.ci.storage.reader.cache.ReaderCache;
import ru.yandex.ci.storage.reader.check.CheckFinalizationService;
import ru.yandex.ci.storage.reader.check.ReaderCheckService;
import ru.yandex.ci.storage.reader.message.main.ReaderStatistics;
import ru.yandex.ci.util.Retryable;

import static ru.yandex.ci.storage.core.Common.CheckStatus.COMPLETING;

@Slf4j
@RequiredArgsConstructor
public class ForwardingMessageProcessor {
    @Nonnull
    private final ReaderCheckService checkService;

    @Nonnull
    private final CheckFinalizationService checkFinalizationService;

    @Nonnull
    private final ReaderCache readerCache;

    @Nonnull
    private final ReaderStatistics statistics;

    public void process(List<ShardOut.ShardForwardingMessage> messages, TimeTraceService.Trace trace) {
        var messagesByCase =
                messages.stream()
                        .map(ForwardingMessage::new)
                        .collect(Collectors.groupingBy(x -> x.getMessage().getMessageCase()));

        log.info(
                "Processing {} forwarding messages, by case: {}",
                messages.size(),
                messagesByCase.entrySet().stream()
                        .map(x -> "%s - %d".formatted(x.getKey(), x.getValue().size()))
                        .collect(Collectors.joining(", "))
        );

        Retryable.retryUntilInterruptedOrSucceeded(
                () -> process(messagesByCase, trace),
                (e) -> {
                    if (e instanceof UnavailableException) {
                        log.warn("Db unavailable {}", e.getMessage());
                        this.statistics.onDbUnavailableError();
                    } else {
                        log.error("Failed", e);
                        this.statistics.getShard().onReadFailed();
                    }
                },
                true
        );
    }

    private void process(Map<MessageCase, List<ForwardingMessage>> messagesByCase, TimeTraceService.Trace trace) {
        this.processPessimizeMessages(messagesByCase.getOrDefault(MessageCase.PESSIMIZE, List.of()));
        this.processMetricsMessages(messagesByCase.getOrDefault(MessageCase.METRIC, List.of()));
        this.processTraceMessages(messagesByCase.getOrDefault(MessageCase.TRACE, List.of()));
        trace.step("forwarding_simple");

        this.processTestTypeFinishedMessages(messagesByCase.getOrDefault(MessageCase.TEST_TYPE_FINISHED, List.of()));
        trace.step("forwarding_test_type_finished");

        this.processTestTypeFinishedMessages(
                messagesByCase.getOrDefault(MessageCase.TEST_TYPE_SIZE_FINISHED, List.of())
        );
        trace.step("forwarding_test_type_size_finished");

        this.processCancelMessages(messagesByCase.getOrDefault(MessageCase.CANCEL, List.of()));
        trace.step("forwarding_cancel");

        this.processFinishedMessages(messagesByCase.getOrDefault(MessageCase.FINISHED, List.of()));
        trace.step("forwarding_finished");
    }

    private void processMetricsMessages(List<ForwardingMessage> messages) {
        if (messages.isEmpty()) {
            return;
        }

        for (var message : messages) {
            var taskId = message.getTaskId();
            log.info("Processing metrics for {}, type: {}", taskId, message.getMessage().getMessageCase());

            var metricProto = message.getMessage().getMetric();
            var metric = new Metric(metricProto.getValue(), metricProto.getAggregate(), metricProto.getSize());

            this.checkService.processMetrics(
                    taskId,
                    message.getPartition(),
                    Metrics.withSingleMetric(metricProto.getName(), metric)
            );
        }
    }

    private void processTestTypeFinishedMessages(List<ForwardingMessage> messages) {
        if (messages.isEmpty()) {
            return;
        }

        readerCache.modifyWithDbTx(cache -> processTestTypeFinishedMessagesInTx(cache, messages));

        messages.forEach(
                message -> this.checkFinalizationService.startFinalizationForCompletedChunks(
                        message.getTaskId().getIterationId()
                )
        );
    }

    private void processTestTypeFinishedMessagesInTx(
            ReaderCache.Modifiable cache, List<ForwardingMessage> messages
    ) {
        // warm up cache
        cache.checkTasks().getFresh(messages.stream().map(ForwardingMessage::getTaskId).collect(Collectors.toSet()));

        for (var message : messages) {
            var taskId = message.getTaskId();

            Actions.TestType testType;
            Actions.TestTypeSizeFinished.Size testSize;
            if (message.getMessage().getMessageCase().equals(MessageCase.TEST_TYPE_FINISHED)) {
                testType = message.getMessage().getTestTypeFinished().getTestType();
                testSize = null;
            } else {
                testType = message.getMessage().getTestTypeSizeFinished().getTestType();
                testSize = message.getMessage().getTestTypeSizeFinished().getSize();
            }
            log.info(
                    "Processing test type finished for {}/{}, type: {}, size {}",
                    taskId, message.getPartition(), testType, testSize == null ? "-" : testSize
            );

            this.checkFinalizationService.processTestTypeFinishedInTx(
                    cache, taskId, message.getPartition(), testType, testSize
            );
        }
    }

    private void processTraceMessages(List<ForwardingMessage> messages) {
        if (messages.isEmpty()) {
            return;
        }

        this.processDistbuildStarted(
                messages.stream()
                        .filter(x -> x.getMessage().getTrace().getType().equals("distbuild/started"))
                        .collect(Collectors.toList())
        );
    }

    private void processDistbuildStarted(List<ForwardingMessage> messages) {
        if (messages.isEmpty()) {
            return;
        }

        readerCache.modifyWithDbTx(cache -> processDistbuildStartedInTx(cache, messages));
    }

    private void processDistbuildStartedInTx(ReaderCache.Modifiable cache, List<ForwardingMessage> messages) {
        // warm up cache
        cache.checkTasks().getFresh(messages.stream().map(ForwardingMessage::getTaskId).collect(Collectors.toSet()));

        for (var message : messages) {
            this.checkService.processDistbuildStartedInTx(cache, message.getTaskId());
        }
    }

    private void processPessimizeMessages(List<ForwardingMessage> messages) {
        if (messages.isEmpty()) {
            return;
        }

        for (var message : messages) {
            var iterationId = message.getTaskId().getIterationId();
            log.info("Processing pessimize for {}", iterationId);
            this.checkService.processPessimize(iterationId, message.getMessage().getPessimize().getInfo());
        }
    }

    private void processFinishedMessages(List<ForwardingMessage> messages) {
        if (messages.isEmpty()) {
            return;
        }

        readerCache.modifyWithDbTx(cache -> processPartitionFinishedInTx(messages, cache));

        for (var message : messages) {
            // relying on cache after cache update in previous section.
            var iteration = this.readerCache.iterations().getOrThrow(message.getTaskId().getIterationId());

            if (iteration.getStatus().equals(COMPLETING)) {
                completeIteration(iteration);
            }
        }
    }

    private void completeIteration(CheckIterationEntity iteration) {
        var testTypes = iteration.getTestTypeStatistics().getNotCompleted();

        log.info(
                "Completing iteration {}, not completed types: {}, status: {}",
                iteration.getId(),
                testTypes,
                iteration.getTestTypeStatistics().printStatus()
        );

        if (testTypes.isEmpty()) {
            checkFinalizationService.finishIteration(iteration.getId());
        } else {
            testTypes = iteration.getTestTypeStatistics().getRunning().stream()
                    .filter(type -> iteration.getTestTypeStatistics().get(type).getRegisteredTasks() > 0)
                    .collect(Collectors.toSet());

            if (!testTypes.isEmpty()) {
                log.error(
                        "[Logic error] Some test types for {} still running after all tasks finished: [{}]",
                        iteration.getId(), testTypes
                );

                this.statistics.onLogicError();
            }
        }
    }

    private void processPartitionFinishedInTx(List<ForwardingMessage> messages, ReaderCache.Modifiable cache) {
        // warm up cache
        cache.checkTasks().getFresh(messages.stream().map(ForwardingMessage::getTaskId).collect(Collectors.toSet()));

        for (var message : messages) {
            log.info(
                    "Processing partition finished, task: {}, partition: {}",
                    message.getTaskId(), message.getPartition()
            );

            checkFinalizationService.processPartitionFinishedInTx(cache, message.getTaskId(), message.getPartition());
        }
    }

    private void processCancelMessages(List<ForwardingMessage> messages) {
        if (messages.isEmpty()) {
            return;
        }

        for (var message : messages) {
            var iterationId = CheckProtoMappers.toIterationId(message.getMessage().getCancel().getIterationId());
            var affectedChunks = this.readerCache.chunkAggregatesGroupedByIteration()
                    .getNotCompletedChunkIds(iterationId);

            log.info("Cancelling iteration {}, affected chunks: {}", iterationId, affectedChunks);

            this.checkFinalizationService.processIterationCancelled(iterationId);

            if (!affectedChunks.isEmpty()) {
                readerCache.modify(cache -> this.checkFinalizationService.sendChunkFinish(
                        cache, iterationId, affectedChunks, Common.ChunkAggregateState.CAS_CANCELLED
                ));

            }
        }
    }

    @Value
    private static class ForwardingMessage {
        CheckTaskEntity.Id taskId;
        int partition;
        ShardOut.ShardForwardingMessage message;

        ForwardingMessage(ShardOut.ShardForwardingMessage message) {
            this.taskId = CheckProtoMappers.toTaskId(message.getFullTaskId());
            this.partition = message.getPartition();
            this.message = message;
        }
    }
}
