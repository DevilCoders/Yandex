package ru.yandex.ci.storage.reader.message.main;

import javax.annotation.Nullable;

import io.micrometer.core.instrument.Counter;
import io.micrometer.core.instrument.MeterRegistry;
import lombok.Value;

import ru.yandex.ci.metrics.CommonMetricNames;
import ru.yandex.ci.metrics.EmptyCounter;
import ru.yandex.ci.storage.core.message.events.EventsStreamStatistics;
import ru.yandex.ci.storage.core.micrometr.StorageMetrics;
import ru.yandex.ci.storage.reader.message.shard.ShardOutStreamStatistics;

@Value
public class ReaderStatistics {
    MainStreamStatistics main;
    ShardOutStreamStatistics shard;
    EventsStreamStatistics events;
    Counter bulkInsertErrors;
    Counter dbUnavailableError;
    Counter logicErrors;

    public ReaderStatistics(
            MainStreamStatistics main,
            ShardOutStreamStatistics shard,
            EventsStreamStatistics events,
            @Nullable MeterRegistry meterRegistry
    ) {
        this.main = main;
        this.shard = shard;
        this.events = events;

        if (meterRegistry != null) {
            this.bulkInsertErrors = Counter
                    .builder(StorageMetrics.ERRORS)
                    .tag(StorageMetrics.ERROR_TYPE, "db_bulk_insert")
                    .tag(StorageMetrics.ERROR_LEVEL, CommonMetricNames.ERROR_LEVEL_WARN)
                    .register(meterRegistry);

            this.dbUnavailableError = Counter
                    .builder(StorageMetrics.ERRORS)
                    .tag(StorageMetrics.ERROR_TYPE, "db_unavailable")
                    .tag(StorageMetrics.ERROR_LEVEL, CommonMetricNames.ERROR_LEVEL_WARN)
                    .register(meterRegistry);

            this.logicErrors = Counter
                    .builder(StorageMetrics.ERRORS)
                    .tag(StorageMetrics.ERROR_TYPE, "logic")
                    .tag(StorageMetrics.ERROR_LEVEL, CommonMetricNames.ERROR_LEVEL_WARN)
                    .register(meterRegistry);
        } else {
            this.bulkInsertErrors = new EmptyCounter();
            this.dbUnavailableError = new EmptyCounter();
            this.logicErrors = new EmptyCounter();
        }
    }

    public void onBulkInsertError() {
        this.bulkInsertErrors.increment();
    }

    public void onLogicError() {
        this.logicErrors.increment();
    }

    public void onDbUnavailableError() {
        this.dbUnavailableError.increment();
    }
}
