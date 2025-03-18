package ru.yandex.monlib.metrics;

import java.lang.management.GarbageCollectorMXBean;
import java.lang.management.ManagementFactory;
import java.util.List;

import ru.yandex.monlib.metrics.encode.text.MetricTextEncoder;
import ru.yandex.monlib.metrics.registry.MetricRegistry;


/**
 * @author Sergey Polovko
 */
public final class JvmGc {
    private JvmGc() {}

    public static void addMetrics(MetricRegistry registry) {
        List<GarbageCollectorMXBean> gcMXBeans = ManagementFactory.getGarbageCollectorMXBeans();
        for (GarbageCollectorMXBean gcMXBean : gcMXBeans) {
            MetricRegistry gcRegistry = registry.subRegistry("gc", gcMXBean.getName());
            gcRegistry.lazyRate("jvm.gc.count", gcMXBean::getCollectionCount);
            gcRegistry.lazyRate("jvm.gc.timeMs", gcMXBean::getCollectionTime);
        }
    }

    // self test
    public static void main(String[] args) {
        MetricRegistry registry = new MetricRegistry();
        addMetrics(registry);
        try (MetricTextEncoder e = new MetricTextEncoder(System.out, true)) {
            registry.accept(0, e);
        }
    }
}
