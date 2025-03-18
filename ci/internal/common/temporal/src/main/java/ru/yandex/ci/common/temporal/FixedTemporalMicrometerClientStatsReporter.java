package ru.yandex.ci.common.temporal;

import java.util.Map;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import com.uber.m3.tally.Buckets;
import com.uber.m3.util.Duration;
import io.micrometer.core.instrument.DistributionSummary;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.Tag;
import io.micrometer.core.instrument.Timer;
import io.temporal.common.reporter.MicrometerClientStatsReporter;

public class FixedTemporalMicrometerClientStatsReporter extends MicrometerClientStatsReporter {

    private final MeterRegistry registry;

    public FixedTemporalMicrometerClientStatsReporter(MeterRegistry registry) {
        super(registry);
        this.registry = registry;
    }

    @Override
    public void reportTimer(String name, Map<String, String> tags, Duration interval) {

        Timer.builder(name)
                .tags(getTags(tags))
                .publishPercentileHistogram(true)
                .register(registry)
                .record(interval.getNanos(), TimeUnit.NANOSECONDS);
    }

    private static double toSeconds(Duration duration) {
        return ((double) duration.getNanos()) / Duration.NANOS_PER_SECOND;
    }

    @SuppressWarnings("deprecation")
    @Override
    public void reportHistogramDurationSamples(String name, Map<String, String> tags, Buckets buckets,
                                               Duration bucketLowerBound, Duration bucketUpperBound, long samples) {


        var bucketsMillis = buckets.getDurationUpperBounds().stream()
                .mapToDouble(FixedTemporalMicrometerClientStatsReporter::toSeconds)
                .filter(value -> value != 0)
                .toArray();
        var summary = DistributionSummary.builder(name)
                .tags(getTags(tags))
                .baseUnit("seconds")
                .serviceLevelObjectives(bucketsMillis)
                .register(registry);

        for (int i = 0; i < samples; i++) {
            summary.record(toSeconds(bucketLowerBound));
        }
    }

    private Iterable<Tag> getTags(Map<String, String> tags) {
        return tags.entrySet().stream()
                .map(entry -> Tag.of(entry.getKey(), entry.getValue()))
                .collect(Collectors.toList());
    }

}
