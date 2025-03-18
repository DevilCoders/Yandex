package ru.yandex.ci.logbroker;

import java.util.List;
import java.util.concurrent.BlockingQueue;

import ru.yandex.ci.util.queue.QueueWorker;

public class LogbrokerReadWorker extends QueueWorker<LogbrokerPartitionRead> {
    private final LogbrokerReadProcessor processor;
    private final LogbrokerStatistics statistics;

    public LogbrokerReadWorker(
            LogbrokerReadProcessor processor,
            BlockingQueue<LogbrokerPartitionRead> queue,
            int drainLimit,
            LogbrokerStatistics statistics
    ) {
        super(queue, drainLimit);
        this.processor = processor;
        this.statistics = statistics;
    }

    @Override
    public void onDrain(int amount) {
        this.statistics.onReadQueueDrain(amount);
    }

    @Override
    public void process(List<LogbrokerPartitionRead> items) {
        this.processor.process(items);
    }

    @Override
    public void onFailed() {
        this.statistics.onReadFailed();
    }
}
