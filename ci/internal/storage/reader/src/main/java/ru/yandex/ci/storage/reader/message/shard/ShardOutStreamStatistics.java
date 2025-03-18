package ru.yandex.ci.storage.reader.message.shard;

import io.micrometer.core.instrument.MeterRegistry;
import io.prometheus.client.CollectorRegistry;
import lombok.experimental.Delegate;

import ru.yandex.ci.logbroker.LogbrokerStatisticsImpl;
import ru.yandex.ci.storage.core.message.StreamProcessingStatistics;
import ru.yandex.ci.storage.core.message.StreamProcessingStatisticsImpl;
import ru.yandex.ci.storage.core.micrometr.StorageMetrics;

@SuppressWarnings("MissingOverride")
public class ShardOutStreamStatistics extends LogbrokerStatisticsImpl implements StreamProcessingStatistics {
    public static final String STREAM_NAME = "shard_out";

    @Delegate
    private final StreamProcessingStatistics processing;

    public ShardOutStreamStatistics(MeterRegistry meterRegistry, CollectorRegistry collectorRegistry) {
        super(StorageMetrics.PREFIX, STREAM_NAME, meterRegistry, collectorRegistry);

        this.processing = new StreamProcessingStatisticsImpl(STREAM_NAME, meterRegistry);
    }
}
