package ru.yandex.ci.storage.core.message.main;

import io.micrometer.core.instrument.MeterRegistry;
import io.prometheus.client.CollectorRegistry;
import lombok.experimental.Delegate;

import ru.yandex.ci.logbroker.LogbrokerStatisticsImpl;
import ru.yandex.ci.storage.core.message.StreamProcessingStatistics;
import ru.yandex.ci.storage.core.message.StreamProcessingStatisticsImpl;
import ru.yandex.ci.storage.core.micrometr.StorageMetrics;

@SuppressWarnings("MissingOverride")
public class MainStreamStatistics extends LogbrokerStatisticsImpl implements StreamProcessingStatistics {
    public static final String STREAM_NAME = "main";

    @Delegate
    private final StreamProcessingStatisticsImpl processing;

    public MainStreamStatistics(MeterRegistry meterRegistry, CollectorRegistry collectorRegistry) {
        super(StorageMetrics.PREFIX, STREAM_NAME, meterRegistry, collectorRegistry);

        this.processing = new StreamProcessingStatisticsImpl(STREAM_NAME, meterRegistry);
    }

}
