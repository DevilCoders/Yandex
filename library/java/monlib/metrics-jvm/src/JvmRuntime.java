package ru.yandex.monlib.metrics;

import java.lang.management.ManagementFactory;

import ru.yandex.monlib.metrics.encode.text.MetricTextEncoder;
import ru.yandex.monlib.metrics.registry.MetricRegistry;


/**
 * @author Sergey Polovko
 */
public final class JvmRuntime {
    private JvmRuntime() {}

    public static void addMetrics(MetricRegistry registry) {
        registry.lazyGaugeInt64("jvm.runtime.processors", () -> Runtime.getRuntime().availableProcessors());
        registry.lazyGaugeInt64("jvm.runtime.freeMemory", () -> Runtime.getRuntime().freeMemory());
        registry.lazyGaugeInt64("jvm.runtime.totalMemory", () -> Runtime.getRuntime().totalMemory());
        registry.lazyGaugeInt64("jvm.runtime.maxMemory", () -> Runtime.getRuntime().maxMemory());
        registry.lazyCounter("jvm.runtime.uptime", () -> ManagementFactory.getRuntimeMXBean().getUptime());

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
