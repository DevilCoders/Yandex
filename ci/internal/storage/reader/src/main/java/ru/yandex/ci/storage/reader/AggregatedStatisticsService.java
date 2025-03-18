package ru.yandex.ci.storage.reader;

import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.function.Function;
import java.util.stream.Collectors;

import com.google.common.annotations.VisibleForTesting;
import io.micrometer.core.instrument.Gauge;
import io.micrometer.core.instrument.MeterRegistry;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.storage_statistics.StorageStatistics;
import ru.yandex.ci.storage.core.db.model.storage_statistics.StorageStatisticsEntity;

public class AggregatedStatisticsService {
    private static final Logger log = LoggerFactory.getLogger(AggregatedStatisticsService.class);

    private final CiStorageDb db;

    private final AtomicInteger activeChecks = new AtomicInteger();
    private final AtomicInteger activeIterations = new AtomicInteger();
    private final AtomicInteger activeLeftTasks = new AtomicInteger();
    private final AtomicInteger activeRightTasks = new AtomicInteger();

    public AggregatedStatisticsService(CiStorageDb db, MeterRegistry meterRegistry) {
        this.db = db;

        Gauge.builder("storage_checks", activeChecks::get).tag("state", "active").register(meterRegistry);
        Gauge.builder("storage_iterations", activeIterations::get).tag("state", "active").register(meterRegistry);
        Gauge.builder("storage_tasks", activeLeftTasks::get)
                .tag("state", "active")
                .tag("side", "left")
                .register(meterRegistry);

        Gauge.builder("storage_tasks", activeRightTasks::get)
                .tag("state", "active")
                .tag("side", "right")
                .register(meterRegistry);
    }

    public void start() {
        while (!Thread.interrupted()) {
            try {
                update();
                Thread.sleep(TimeUnit.SECONDS.toMillis(10));
            } catch (RuntimeException e) {
                log.error("Fail to update statistics", e);
            } catch (InterruptedException e) {
                log.info("interrupted");
                return;
            }
        }
    }

    public int getActiveChecks() {
        return activeChecks.get();
    }

    public int getActiveIterations() {
        return this.activeIterations.get();
    }

    public int getActiveLeftTasks() {
        return this.activeLeftTasks.get();
    }

    public int getActiveRightTasks() {
        return this.activeRightTasks.get();
    }

    @VisibleForTesting
    void update() {
        this.db.currentOrReadOnly(this::updateInTx);
    }

    private void updateInTx() {
        var statistics = this.db.storageStatistics().getAll().stream()
                .collect(Collectors.toMap(StorageStatisticsEntity::getId, Function.identity()));

        activeChecks.set(
                (int) statistics.getOrDefault(StorageStatistics.ACTIVE_CHECKS, StorageStatisticsEntity.EMPTY).getValue()
        );

        activeIterations.set(
                (int) statistics
                        .getOrDefault(StorageStatistics.ACTIVE_ITERATIONS, StorageStatisticsEntity.EMPTY)
                        .getValue()
        );

        activeLeftTasks.set(
                (int) statistics
                        .getOrDefault(StorageStatistics.ACTIVE_LEFT_TASKS, StorageStatisticsEntity.EMPTY)
                        .getValue()
        );

        activeRightTasks.set(
                (int) statistics
                        .getOrDefault(StorageStatistics.ACTIVE_RIGHT_TASKS, StorageStatisticsEntity.EMPTY)
                        .getValue()
        );

    }
}
