package ru.yandex.monlib.metrics;

import java.lang.management.ManagementFactory;
import java.lang.management.ThreadInfo;
import java.lang.management.ThreadMXBean;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.Executor;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ForkJoinPool;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import ru.yandex.monlib.metrics.encode.text.MetricTextEncoder;
import ru.yandex.monlib.metrics.labels.Labels;
import ru.yandex.monlib.metrics.registry.MetricRegistry;


/**
 * @author Sergey Polovko
 */
public final class JvmThreads {
    private JvmThreads() {}

    public static void addMetrics(MetricRegistry registry) {
        ThreadMXBean threadMXBean = ManagementFactory.getThreadMXBean();
        registry.lazyGaugeInt64("jvm.threads.total", threadMXBean::getThreadCount);
        registry.lazyGaugeInt64("jvm.threads.daemons", threadMXBean::getDaemonThreadCount);
        registry.lazyGaugeInt64("jvm.threads.max", threadMXBean::getPeakThreadCount);
        registry.lazyGaugeInt64("jvm.threads.started", threadMXBean::getPeakThreadCount);

        for (Thread.State state : Thread.State.values()) {
            registry.lazyGaugeDouble("jvm.threads.count",
                Labels.of("state", state.name()),
                () -> getThreadsCount(threadMXBean, state));
        }
    }

    public static void addExecutorMetrics(String name, Executor executor, MetricRegistry registry) {
        MetricRegistry tpRegistry = registry.subRegistry("threadPool", name);
        if (executor instanceof ForkJoinPool) {
            ForkJoinPool forkJoinPool = (ForkJoinPool) executor;
            tpRegistry.lazyGaugeInt64("jvm.threadPool.size", forkJoinPool::getPoolSize);
            tpRegistry.lazyGaugeInt64("jvm.threadPool.maxSize", forkJoinPool::getParallelism);
            tpRegistry.lazyGaugeInt64("jvm.threadPool.runningThreads", forkJoinPool::getRunningThreadCount);
            tpRegistry.lazyGaugeInt64("jvm.threadPool.activeThreads", forkJoinPool::getActiveThreadCount);
            tpRegistry.lazyRate("jvm.threadPool.stealCount", forkJoinPool::getStealCount);
            tpRegistry.lazyGaugeInt64("jvm.threadPool.queuedTasks", forkJoinPool::getQueuedTaskCount);
            tpRegistry.lazyGaugeInt64("jvm.threadPool.submittedTasks", forkJoinPool::getQueuedSubmissionCount);
        } else if (executor instanceof ThreadPoolExecutor) {
            ThreadPoolExecutor threadPool = (ThreadPoolExecutor) executor;
            tpRegistry.lazyGaugeInt64("jvm.threadPool.size", threadPool::getPoolSize);
            tpRegistry.lazyGaugeInt64("jvm.threadPool.maxSize", threadPool::getMaximumPoolSize);
            tpRegistry.lazyGaugeInt64("jvm.threadPool.coreSize", threadPool::getCorePoolSize);
            tpRegistry.lazyGaugeInt64("jvm.threadPool.activeThreads", threadPool::getActiveCount);
            tpRegistry.lazyRate("jvm.threadPool.submittedTasks", threadPool::getTaskCount);
            tpRegistry.lazyRate("jvm.threadPool.completedTasks", threadPool::getCompletedTaskCount);
        }
    }

    private static int getThreadsCount(ThreadMXBean threadMXBean, Thread.State state) {
        int count = 0;
        ThreadInfo[] threadsInfo = threadMXBean.getThreadInfo(threadMXBean.getAllThreadIds());
        for (ThreadInfo threadInfo : threadsInfo) {
            if (threadInfo != null && threadInfo.getThreadState() == state) {
                count++;
            }
        }
        return count;
    }

    // self test
    public static void main(String[] args) throws Exception {
        MetricRegistry registry = new MetricRegistry();
        addMetrics(registry);

        addExecutorMetrics("common", ForkJoinPool.commonPool(), registry);
        CompletableFuture.runAsync(() -> {}).join();

        ExecutorService threadPool = Executors.newFixedThreadPool(3);
        addExecutorMetrics("myPool", threadPool, registry);
        threadPool.submit(() -> {});
        threadPool.shutdown();
        threadPool.awaitTermination(200, TimeUnit.MILLISECONDS);

        try (MetricTextEncoder e = new MetricTextEncoder(System.out, true)) {
            registry.accept(0, e);
        }
    }
}
