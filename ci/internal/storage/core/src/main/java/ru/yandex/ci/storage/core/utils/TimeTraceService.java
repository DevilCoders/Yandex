package ru.yandex.ci.storage.core.utils;

import java.time.Clock;
import java.time.Duration;
import java.time.Instant;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;
import java.util.stream.Collectors;

import io.prometheus.client.CollectorRegistry;
import io.prometheus.client.Histogram;
import lombok.Getter;

import ru.yandex.ci.storage.core.micrometr.StorageMetrics;

@Getter
public class TimeTraceService {
    private static final String STEP_LABEL = "step";

    private final Clock clock;
    private final ConcurrentMap<String, Histogram> histograms = new ConcurrentHashMap<>();
    private final CollectorRegistry collectorRegistry;

    public TimeTraceService(Clock clock, CollectorRegistry collectorRegistry) {
        this.clock = clock;
        this.collectorRegistry = collectorRegistry;
    }

    public Trace createTrace(String histogram) {
        return new Trace(clock, histograms.computeIfAbsent(histogram, (key) -> create(key, collectorRegistry)));
    }

    private Histogram create(String name, CollectorRegistry collectorRegistry) {
        return Histogram.build()
                .name(StorageMetrics.PREFIX + name)
                .help("Timings in seconds")
                .labelNames(STEP_LABEL)
                .buckets(new double[]{0.1, 1, 2, 4, 8, 16, 32, 64, 128})
                .register(collectorRegistry);
    }

    public static class Trace {
        private final Histogram histogram;
        private final Clock clock;
        private final Map<String, Double> stepTimings = new LinkedHashMap<>();

        private Instant lastStepTime;

        private Trace(Clock clock, Histogram histogram) {
            this.histogram = histogram;
            this.clock = clock;
            this.lastStepTime = Instant.now(clock);
        }

        private Instant now() {
            return Instant.now(clock);
        }

        public synchronized void step(String name) {
            var currentTime = now();
            var stepDuration = Duration.between(lastStepTime, currentTime).toMillis() / 1000.0;
            histogram.labels(name).observe(stepDuration);
            stepTimings.put(name, stepDuration);
            this.lastStepTime = currentTime;
        }

        public synchronized String logString() {
            return stepTimings.entrySet().stream()
                    .map(entry -> "%s [%s s]".formatted(entry.getKey(), entry.getValue()))
                    .collect(Collectors.joining(", "));
        }
    }
}
