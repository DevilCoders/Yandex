package ru.yandex.ci.storage.core.message;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import io.micrometer.core.instrument.Counter;
import io.micrometer.core.instrument.MeterRegistry;

import ru.yandex.ci.metrics.CommonMetricNames;
import ru.yandex.ci.storage.core.micrometr.StorageMetrics;

public class StreamProcessingStatisticsImpl implements StreamProcessingStatistics {
    private final String stream;
    private final MeterRegistry meterRegistry;

    private final Counter parseErrors;
    private final Counter validationErrors;
    private final Counter missingErrors;
    private final Counter finishedStateErrors;

    private final Map<Enum<?>, Counter> messageProcessed;

    public StreamProcessingStatisticsImpl(String stream, MeterRegistry meterRegistry) {
        this.stream = stream;
        this.meterRegistry = meterRegistry;

        this.parseErrors = Counter
                .builder(StorageMetrics.ERRORS)
                .tag(StorageMetrics.ERROR_TYPE, stream + "_" + StorageMetrics.ERROR_PARSE)
                .tag(StorageMetrics.ERROR_LEVEL, CommonMetricNames.ERROR_LEVEL_ERROR)
                .register(meterRegistry);

        this.validationErrors = Counter
                .builder(StorageMetrics.ERRORS)
                .tag(StorageMetrics.ERROR_TYPE, stream + "_" + StorageMetrics.ERROR_VALIDATION)
                .tag(StorageMetrics.ERROR_LEVEL, CommonMetricNames.ERROR_LEVEL_ERROR)
                .register(meterRegistry);

        this.missingErrors = Counter
                .builder(StorageMetrics.ERRORS)
                .tag(StorageMetrics.ERROR_TYPE, stream + "_" + StorageMetrics.ERROR_MISSING)
                .tag(StorageMetrics.ERROR_LEVEL, CommonMetricNames.ERROR_LEVEL_WARN)
                .register(meterRegistry);

        this.finishedStateErrors = Counter
                .builder(StorageMetrics.ERRORS)
                .tag(StorageMetrics.ERROR_TYPE, stream + "_" + StorageMetrics.ERROR_FINISHED_STATE)
                .tag(StorageMetrics.ERROR_LEVEL, CommonMetricNames.ERROR_LEVEL_WARN)
                .register(meterRegistry);

        this.messageProcessed = new ConcurrentHashMap<>();
    }

    @Override
    public void onMessageProcessed(Enum<?> messageCase, int value) {
        if (value == 0) {
            return;
        }

        this.messageProcessed.computeIfAbsent(
                        messageCase,
                        (c) -> Counter.builder(StorageMetrics.PREFIX + stream + "_messages_processed")
                                .tag(StorageMetrics.MESSAGE_CASE, c.name())
                                .register(meterRegistry)
                )
                .increment(value);
    }

    @Override
    public void onParseError() {
        this.parseErrors.increment();
    }

    @Override
    public void onValidationError() {
        this.validationErrors.increment();
    }

    @Override
    public void onMissingError() {
        this.missingErrors.increment();
    }

    @Override
    public void onFinishedStateError() {
        this.finishedStateErrors.increment();
    }
}
