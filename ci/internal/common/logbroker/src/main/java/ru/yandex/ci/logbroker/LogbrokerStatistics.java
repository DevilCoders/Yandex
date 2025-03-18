package ru.yandex.ci.logbroker;

import java.util.concurrent.ExecutorService;

import io.micrometer.core.instrument.Gauge;

public interface LogbrokerStatistics {
    void onReadsCommited(int size);

    void onReadReceived();

    void onPartitionLocked(String topic);

    void onPartitionUnlocked(String topic);

    void onReadProcessed();

    void onReadFailed();

    void onDataReceived(int partition, int compressedLength, int decompressedLength);

    void onReadQueueDrain(int amount);

    void onCommitFailed();

    void register(Gauge.Builder<?> gauge);

    ExecutorService monitor(ExecutorService executorService, String name);
}
