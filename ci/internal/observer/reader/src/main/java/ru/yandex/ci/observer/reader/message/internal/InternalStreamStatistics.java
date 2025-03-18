package ru.yandex.ci.observer.reader.message.internal;

import io.micrometer.core.instrument.MeterRegistry;
import io.prometheus.client.CollectorRegistry;
import lombok.experimental.Delegate;

import ru.yandex.ci.logbroker.LogbrokerStatisticsImpl;
import ru.yandex.ci.storage.core.message.StreamProcessingStatistics;
import ru.yandex.ci.storage.core.message.StreamProcessingStatisticsImpl;
import ru.yandex.ci.storage.core.micrometr.StorageMetrics;

@SuppressWarnings("MissingOverride")
public class InternalStreamStatistics extends LogbrokerStatisticsImpl implements StreamProcessingStatistics {
    public static final String STREAM_NAME = "internal";

    @Delegate
    private final StreamProcessingStatistics processing;

    public InternalStreamStatistics(MeterRegistry meterRegistry, CollectorRegistry collectorRegistry) {
        super(StorageMetrics.PREFIX, STREAM_NAME, meterRegistry, collectorRegistry);

        this.processing = new StreamProcessingStatisticsImpl(STREAM_NAME, meterRegistry);
    }
}
