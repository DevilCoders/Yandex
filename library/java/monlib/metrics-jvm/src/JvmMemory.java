package ru.yandex.monlib.metrics;

import java.lang.management.BufferPoolMXBean;
import java.lang.management.ManagementFactory;
import java.lang.management.MemoryMXBean;
import java.lang.management.MemoryPoolMXBean;
import java.util.List;
import java.util.regex.Pattern;

import ru.yandex.monlib.metrics.encode.text.MetricTextEncoder;
import ru.yandex.monlib.metrics.registry.MetricRegistry;


/**
 * @author Sergey Polovko
 */
public final class JvmMemory {
    private JvmMemory() {}

    private static final Pattern QUOTE = Pattern.compile("'");

    public static void addMetrics(MetricRegistry registry) {
        MemoryMXBean memoryMXBean = ManagementFactory.getMemoryMXBean();

        addHeapGauges(memoryMXBean, registry);
        addOffHeapGauges(memoryMXBean, registry);

        // pools
        List<MemoryPoolMXBean> memoryPoolMXBeans = ManagementFactory.getMemoryPoolMXBeans();
        for (MemoryPoolMXBean memoryPoolMXBean : memoryPoolMXBeans) {
            addMemoryPoolGauges(memoryPoolMXBean, registry);
        }

        // buffer pools
        List<BufferPoolMXBean> platformMXBeans = ManagementFactory.getPlatformMXBeans(BufferPoolMXBean.class);
        for (BufferPoolMXBean platformMXBean : platformMXBeans) {
            addDirectBufferPoolGauges(platformMXBean, registry);
        }
    }

    private static void addHeapGauges(MemoryMXBean memoryMXBean, MetricRegistry registry) {
        MetricRegistry heapGroup = registry.subRegistry("memory", "heap");
        heapGroup.lazyGaugeInt64("jvm.memory.init", () -> memoryMXBean.getHeapMemoryUsage().getInit());
        heapGroup.lazyGaugeInt64("jvm.memory.used", () -> memoryMXBean.getHeapMemoryUsage().getUsed());
        heapGroup.lazyGaugeInt64("jvm.memory.committed", () -> memoryMXBean.getHeapMemoryUsage().getCommitted());
        heapGroup.lazyGaugeInt64("jvm.memory.max", () -> memoryMXBean.getHeapMemoryUsage().getMax());
    }

    private static void addOffHeapGauges(MemoryMXBean memoryMXBean, MetricRegistry registry) {
        MetricRegistry offHeapGroup = registry.subRegistry("memory", "off-heap");
        offHeapGroup.lazyGaugeInt64("jvm.memory.init", () -> memoryMXBean.getNonHeapMemoryUsage().getInit());
        offHeapGroup.lazyGaugeInt64("jvm.memory.used", () -> memoryMXBean.getNonHeapMemoryUsage().getUsed());
        offHeapGroup.lazyGaugeInt64("jvm.memory.committed", () -> memoryMXBean.getNonHeapMemoryUsage().getCommitted());
        offHeapGroup.lazyGaugeInt64("jvm.memory.max", () -> memoryMXBean.getNonHeapMemoryUsage().getMax());
    }

    private static void addMemoryPoolGauges(MemoryPoolMXBean memoryPoolMXBean, MetricRegistry registry) {
        MetricRegistry poolGroup = registry.subRegistry("pool", sanitizeName(memoryPoolMXBean.getName()));
        poolGroup.lazyGaugeInt64("jvm.memory.init", () -> memoryPoolMXBean.getUsage().getInit());
        poolGroup.lazyGaugeInt64("jvm.memory.used", () -> memoryPoolMXBean.getUsage().getUsed());
        poolGroup.lazyGaugeInt64("jvm.memory.committed", () -> memoryPoolMXBean.getUsage().getCommitted());
        poolGroup.lazyGaugeInt64("jvm.memory.max", () -> memoryPoolMXBean.getUsage().getMax());
    }

    private static void addDirectBufferPoolGauges(BufferPoolMXBean platformMXBean, MetricRegistry registry) {
        MetricRegistry poolGroup = registry.subRegistry("pool", sanitizeName(platformMXBean.getName()));
        poolGroup.lazyGaugeInt64("jvm.buffer.count", platformMXBean::getCount);
        poolGroup.lazyGaugeInt64("jvm.buffer.used", platformMXBean::getMemoryUsed);
        poolGroup.lazyGaugeInt64("jvm.buffer.capacity", platformMXBean::getTotalCapacity);
    }

    /**
     * Removes ' char from the given name.
     *
     * e.g:
     *   CodeHeap_'non-nmethods'
     *   CodeHeap_'non-profiled_nmethods'
     *   CodeHeap_'profiled_nmethods'
     */
    private static String sanitizeName(String name) {
        return QUOTE.matcher(name).replaceAll("");
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
