package ru.yandex.ci.common.bazinga.monitoring;

import java.util.concurrent.TimeUnit;
import java.util.stream.Stream;

import javax.annotation.Nullable;

import com.google.common.base.Stopwatch;
import io.micrometer.core.instrument.Gauge;
import io.micrometer.core.instrument.MeterRegistry;
import org.joda.time.Duration;
import org.joda.time.Instant;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.bolts.collection.Option;
import ru.yandex.commune.bazinga.BazingaControllerApp;
import ru.yandex.commune.bazinga.impl.JobStatus;
import ru.yandex.commune.bazinga.impl.OnetimeTaskState;
import ru.yandex.commune.bazinga.ydb.storage.YdbBazingaStorage;

public class OnetimeJobFailurePercent {
    private static final Logger log = LoggerFactory.getLogger(OnetimeJobFailurePercent.class);

    private final BazingaControllerApp controllerApp;
    private final YdbBazingaStorage bazingaStorage;
    private long failureCount;

    private final int maxJobAgeMinutes = 30;

    public OnetimeJobFailurePercent(
            BazingaControllerApp controllerApp,
            YdbBazingaStorage bazingaStorage,
            @Nullable MeterRegistry registry
    ) {
        this.controllerApp = controllerApp;
        this.bazingaStorage = bazingaStorage;
        if (registry != null) {
            Gauge.builder("bazinga_onetime_failure_count", () -> this.failureCount)
                    .register(registry);
        }
    }

    public void run() {
        log.info("OnetimeJobFailurePercent started");

        var stopwatch = Stopwatch.createStarted();
        if (controllerApp.getBazingaController().isMaster()) {
            failureCount = check();
        } else {
            failureCount = 0;
        }

        log.info("run took {}ms", stopwatch.elapsed(TimeUnit.MILLISECONDS));
    }

    public long getFailureCount() {
        return failureCount;
    }

    private long check() {
        var from = Instant.now().minus(Duration.standardMinutes(maxJobAgeMinutes));

        return bazingaStorage.findOnetimeTaskStates().stream()
                .map(OnetimeTaskState::getTaskId)
                .flatMap(taskId -> Stream.of(JobStatus.FAILED, JobStatus.EXPIRED)
                        .map(status -> bazingaStorage.findOnetimeJobCount(taskId, status, from, Option.empty()))
                ).mapToInt(Integer::valueOf)
                .sum();
    }
}
