package ru.yandex.ci.storage.reader.message.main;

import io.micrometer.core.instrument.Counter;
import io.micrometer.core.instrument.MeterRegistry;
import io.prometheus.client.CollectorRegistry;
import io.prometheus.client.Histogram;

import ru.yandex.ci.metrics.CommonMetricNames;
import ru.yandex.ci.storage.core.micrometr.StorageMetrics;

public class MainStreamStatistics extends ru.yandex.ci.storage.core.message.main.MainStreamStatistics {
    private final Counter metricErrors;

    private final Counter resultsProcessed;

    private final Histogram resultsDistribution;


    public MainStreamStatistics(MeterRegistry meterRegistry, CollectorRegistry collectorRegistry) {
        super(meterRegistry, collectorRegistry);

        this.metricErrors = Counter
                .builder(StorageMetrics.ERRORS)
                .tag(StorageMetrics.ERROR_TYPE, "metric_inconsistency")
                .tag(StorageMetrics.ERROR_LEVEL, CommonMetricNames.ERROR_LEVEL_WARN)
                .register(meterRegistry);

        this.resultsProcessed = Counter
                .builder(StorageMetrics.RESULTS_PROCESSED)
                .register(meterRegistry);

        this.resultsDistribution = Histogram.build()
                .name(StorageMetrics.PREFIX + "results_distribution")
                .help("Result distribution")
                .buckets(new double[]{1, 4, 16, 64, 128, 256, 512, 1024, 2048})
                .register(collectorRegistry);
    }


    public void onResultsProcessed(int numberOfResults) {
        this.resultsProcessed.increment(numberOfResults);
    }

    public void onResultsDistributed(int numberOfResults) {
        this.resultsDistribution.observe(numberOfResults);
    }

    public void onMetricInconsistency() {
        this.metricErrors.increment();
    }
}
