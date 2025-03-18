package ru.yandex.ci.storage.core.message.events;

import io.micrometer.core.instrument.Counter;
import io.micrometer.core.instrument.MeterRegistry;
import io.prometheus.client.CollectorRegistry;
import lombok.experimental.Delegate;

import ru.yandex.ci.logbroker.LogbrokerStatisticsImpl;
import ru.yandex.ci.metrics.CommonMetricNames;
import ru.yandex.ci.storage.core.message.StreamProcessingStatistics;
import ru.yandex.ci.storage.core.message.StreamProcessingStatisticsImpl;
import ru.yandex.ci.storage.core.micrometr.StorageMetrics;

@SuppressWarnings("MissingOverride")
public class EventsStreamStatistics extends LogbrokerStatisticsImpl implements StreamProcessingStatistics {
    public static final String STREAM_NAME = "events";

    @Delegate
    private final StreamProcessingStatistics processing;

    Counter eventWorkerErrors;

    public EventsStreamStatistics(MeterRegistry meterRegistry, CollectorRegistry collectorRegistry) {
        super(StorageMetrics.PREFIX, STREAM_NAME, meterRegistry, collectorRegistry);

        this.processing = new StreamProcessingStatisticsImpl(STREAM_NAME, meterRegistry);

        this.eventWorkerErrors = Counter
                .builder(StorageMetrics.ERRORS)
                .tag(StorageMetrics.ERROR_TYPE, "events")
                .tag(StorageMetrics.ERROR_LEVEL, CommonMetricNames.ERROR_LEVEL_ERROR)
                .register(meterRegistry);
    }

    public void onEventWorkerError() {
        this.eventWorkerErrors.increment();
    }
}
