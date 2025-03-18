package ru.yandex.ci.storage.shard.message;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

import io.micrometer.core.instrument.Gauge;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.storage.core.PostProcessor;
import ru.yandex.ci.storage.core.TaskMessages;
import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.db.model.task_result.TestResult;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffByHashEntity;
import ru.yandex.ci.storage.core.micrometr.StorageMetrics;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.shard.ShardStatistics;
import ru.yandex.ci.storage.shard.message.writer.PostProcessorWriter;
import ru.yandex.ci.util.queue.QueueExecutor;
import ru.yandex.ci.util.queue.QueueWorker;
import ru.yandex.ci.util.queue.SyncQueueExecutor;
import ru.yandex.ci.util.queue.ThreadPerQueueExecutor;

@Slf4j
public class PostProcessorDeliveryService {

    private final ShardStatistics statistics;
    private final PostProcessorWriter postProcessorWriter;

    private final int queueDrainLimit;
    private final String queueSizeMetricName;
    private final QueueExecutor<PostProcessMessage> executor;

    public PostProcessorDeliveryService(
            ShardStatistics statistics,
            PostProcessorWriter postProcessorWriter,
            int queueDrainLimit,
            boolean syncMode
    ) {
        this.executor = syncMode ?
                new SyncQueueExecutor<>(this::process) :
                new ThreadPerQueueExecutor<>("post-processor-delivery", this::createQueueWorker);

        this.statistics = statistics;

        this.queueDrainLimit = queueDrainLimit;
        this.postProcessorWriter = postProcessorWriter;

        this.queueSizeMetricName = StorageMetrics.PREFIX + "post_process_queue_size";

        statistics.register(
                Gauge.builder(queueSizeMetricName, this.executor::getQueueSize)
                        .tag(StorageMetrics.QUEUE, "all")
        );
    }

    public void enqueue(PostProcessMessage message) {
        executor.enqueue("single", message);
    }

    public void process(List<PostProcessMessage> items) {
        var results = items.stream().flatMap(x -> x.getResults().stream())
                .filter(this::isTrunkOrBranch).toList();

        // not using stream.grouping by to keep order
        var diffs = new HashMap<ChunkAggregateEntity.Id, Map<TestEntity.Id, TestDiffByHashEntity>>();
        items.stream().flatMap(x -> x.getDiffs().stream())
                .filter(diff -> !diff.getId().getTestId().isAggregate())
                .forEach(
                        diff -> diffs
                                .computeIfAbsent(diff.getId().getAggregateId(), k -> new HashMap<>())
                                .put(diff.getId().getTestId(), diff)
                );

        log.info(
                "Processing {} messages, {} trunk or branch results, {} diffs",
                items.size(), results.size(), diffs.size()
        );

        var postMessages = new ArrayList<PostProcessor.ResultForPostProcessor>(results.size());

        for (var result : results) {
            var aggregateId = new ChunkAggregateEntity.Id(result.getId().getIterationId(), result.getChunkId());
            var aggregateDiffs = diffs.get(aggregateId);

            var status = result.getStatus();
            if (aggregateDiffs != null) {
                var diff = aggregateDiffs.get(result.getTestId());

                status = diff == null ? result.getStatus() :
                        result.isRight() ? diff.getRight() : diff.getLeft();
            }

            var message = PostProcessor.ResultForPostProcessor.newBuilder()
                    .setId(CheckProtoMappers.toProtoAutocheckTestId(result.getId().getFullTestId()))
                    .setBranch(result.getBranch())
                    .setRevision(result.getRevision())
                    .setRevisionNumber(result.getRevisionNumber())
                    .setChunkHid(result.getAutocheckChunkId() == null ? 0 : result.getAutocheckChunkId())
                    .setFullTaskId(CheckProtoMappers.toProtoCheckTaskId(result.getId().getFullTaskId()))
                    .setAutocheckPartition(result.getId().getPartition())
                    .setResultType(result.getResultType())
                    .setTestStatus(status)
                    .setUid(result.getUid())
                    .setPath(result.getPath())
                    .setName(result.getName())
                    .setOwners(
                            TaskMessages.Owners.newBuilder()
                                    .addAllGroups(result.getOwners().getGroups())
                                    .addAllLogins(result.getOwners().getLogins())
                                    .build()
                    )
                    .setSubtestName(result.getSubtestName())
                    .addAllTags(result.getTags())
                    .putAllMetrics(result.getMetrics())
                    .setCreated(ProtoConverter.convert(result.getCreated()))
                    .setIsRight(result.isRight())
                    .setOldSuiteId(result.getOldSuiteId())
                    .setOldId(result.getOldTestId())
                    .setService(result.getService())
                    .build();

            postMessages.add(message);
        }

        if (!postMessages.isEmpty()) {
            postProcessorWriter.writeResults(postMessages);
        }

        items.stream().map(PostProcessMessage::getCommitCallback).filter(Objects::nonNull).forEach(Runnable::run);
    }

    private boolean isTrunkOrBranch(TestResult result) {
        var branch = ArcBranch.ofBranchName(result.getBranch());
        return branch.isTrunk() || branch.isRelease();
    }

    private QueueWorker<PostProcessMessage> createQueueWorker(
            String queueName, LinkedBlockingQueue<PostProcessMessage> queue
    ) {
        statistics.register(Gauge.builder(queueSizeMetricName, queue::size).tag(StorageMetrics.QUEUE, queueName));
        return new PostProcessorWorker(queue, queueDrainLimit);
    }

    class PostProcessorWorker extends QueueWorker<PostProcessMessage> {
        PostProcessorWorker(
                BlockingQueue<PostProcessMessage> queue,
                int drainLimit
        ) {
            super(queue, drainLimit);
        }

        @Override
        public void onDrain(int amount) {
        }

        @Override
        public void process(List<PostProcessMessage> items) {
            PostProcessorDeliveryService.this.process(items);
        }

        @Override
        public void onFailed() {
            PostProcessorDeliveryService.this.statistics.onPostProcessorWorkerError();
        }
    }
}
