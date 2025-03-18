package ru.yandex.ci.storage.shard.message;

import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.Executors;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

import com.google.common.util.concurrent.ThreadFactoryBuilder;
import com.google.common.util.concurrent.Uninterruptibles;
import io.micrometer.core.instrument.Gauge;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.db.model.chunk_aggregate.ChunkAggregateEntity;
import ru.yandex.ci.storage.core.message.shard_out.ShardOutMessageWriter;
import ru.yandex.ci.storage.core.micrometr.StorageMetrics;
import ru.yandex.ci.storage.shard.ShardStatistics;
import ru.yandex.ci.storage.shard.cache.ShardCache;
import ru.yandex.ci.util.queue.QueueWorker;

@Slf4j
public class AggregateReporter {
    private final ShardStatistics statistics;
    private final ShardOutMessageWriter writer;
    private final ShardCache shardCache;
    private final BlockingQueue<ChunkAggregateEntity.Id> queue;
    private final boolean syncMode;

    public AggregateReporter(
            ShardCache shardCache,
            ShardOutMessageWriter writer,
            ShardStatistics statistics,
            int drainLimit,
            boolean syncMode
    ) {
        this.shardCache = shardCache;
        this.writer = writer;
        this.statistics = statistics;
        this.queue = new LinkedBlockingQueue<>();
        this.syncMode = syncMode;

        statistics.register(
                Gauge.builder(StorageMetrics.PREFIX + "aggregate_reporter_queue_size", queue::size)
        );

        if (!syncMode) {
            Executors.newSingleThreadExecutor(
                    new ThreadFactoryBuilder().setNameFormat("aggregate-reporter-%d").build()
            ).execute(new AggregateReporterWorker(drainLimit));
        }
    }

    public void enqueue(Collection<ChunkAggregateEntity.Id> aggregateIds) {
        if (this.syncMode) {
            this.process(aggregateIds);
        } else {
            queue.addAll(aggregateIds);
        }
    }

    private void process(Collection<ChunkAggregateEntity.Id> items) {
        var ids = new HashSet<>(items);

        var aggregates = shardCache.chunkAggregates().getIfPresent(ids);

        if (aggregates.size() < ids.size()) {
            log.info(
                    "Some aggregates not found in cache, requested {}, found {}",
                    ids.size(), aggregates.size()
            );

            if (aggregates.isEmpty()) {
                return;
            }
        }

        writer.writeAggregates(aggregates);

        log.info("Reported {} aggregates", aggregates.size());
        statistics.onAggregateReported(aggregates.size());
    }

    class AggregateReporterWorker extends QueueWorker<ChunkAggregateEntity.Id> {
        AggregateReporterWorker(int drainLimit) {
            super(queue, drainLimit);
        }

        @Override
        public void onDrain(int amount) {
            statistics.onAggregateReporterDrain(amount);
        }

        @Override
        public void process(List<ChunkAggregateEntity.Id> items) {
            AggregateReporter.this.process(items);

            if (items.size() < this.drainLimit) {
                // Delay further processing to reduce number of writes for same aggregate.
                Uninterruptibles.sleepUninterruptibly(10, TimeUnit.SECONDS);
            }
        }

        @Override
        public void onFailed() {
            statistics.onAggregateReporterWorkerError();
        }
    }
}
