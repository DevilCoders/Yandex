package ru.yandex.ci.storage.post_processor.processing;

import java.util.List;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

import io.micrometer.core.instrument.Gauge;

import ru.yandex.ci.storage.core.micrometr.StorageMetrics;
import ru.yandex.ci.storage.core.utils.CiHashCode;
import ru.yandex.ci.storage.post_processor.PostProcessorStatistics;
import ru.yandex.ci.util.queue.QueueExecutor;
import ru.yandex.ci.util.queue.QueueWorker;
import ru.yandex.ci.util.queue.SyncQueueExecutor;
import ru.yandex.ci.util.queue.ThreadPerQueueExecutor;

public class MetricsProcessorPool {
    private final PostProcessorStatistics statistics;
    private final int numberOfQueues;
    private final MetricsProcessor metricsProcessor;

    private final int queueDrainLimit;
    private final String queueSizeMetricName;
    private final QueueExecutor<ResultMessage> executor;

    public MetricsProcessorPool(
            PostProcessorStatistics statistics,
            MetricsProcessor metricsProcessor,
            int queueCapacity,
            int queueDrainLimit,
            int numberOfQueues
    ) {
        this.executor = numberOfQueues == 0 ?
                new SyncQueueExecutor<>(this::process) :
                new ThreadPerQueueExecutor<>("metrics-processor", queueCapacity, this::createQueueWorker);
        this.statistics = statistics;
        this.queueDrainLimit = queueDrainLimit;
        this.numberOfQueues = numberOfQueues;
        this.metricsProcessor = metricsProcessor;

        this.queueSizeMetricName = StorageMetrics.PREFIX + "metrics_processor_queue_size";

        statistics.register(
                Gauge.builder(queueSizeMetricName, this.executor::getQueueSize)
                        .tag(StorageMetrics.QUEUE, "all")
        );
    }

    public void enqueue(ResultMessage message) {
        var queueNumber = this.numberOfQueues == 0 ?
                0 : Math.abs(CiHashCode.hashCode(message.getResult().getId().getTestId()) % this.numberOfQueues);
        executor.enqueue(String.valueOf(queueNumber), message);
    }

    private void process(List<ResultMessage> messages) {
        this.metricsProcessor.process(messages);
    }

    private QueueWorker<ResultMessage> createQueueWorker(
            String queueName, LinkedBlockingQueue<ResultMessage> queue
    ) {
        statistics.register(
                Gauge.builder(queueSizeMetricName, queue::size)
                        .tag(StorageMetrics.QUEUE, queueName)
        );
        return new MetricsProcessorWorker(queue, queueDrainLimit);
    }

    class MetricsProcessorWorker extends QueueWorker<ResultMessage> {
        MetricsProcessorWorker(
                BlockingQueue<ResultMessage> queue,
                int drainLimit
        ) {
            super(queue, drainLimit);
        }

        @Override
        public void onDrain(int amount) {
        }

        @Override
        public void process(List<ResultMessage> items) {
            MetricsProcessorPool.this.process(items);
        }

        @Override
        public void onFailed() {
            MetricsProcessorPool.this.statistics.onPostProcessorWorkerError();
        }
    }
}
