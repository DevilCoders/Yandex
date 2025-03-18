package ru.yandex.ci.common.temporal.worflow;

import java.time.Duration;
import java.util.concurrent.atomic.AtomicInteger;

import io.micrometer.core.instrument.Gauge;
import io.micrometer.core.instrument.MeterRegistry;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.temporal.TemporalService;
import ru.yandex.ci.common.temporal.config.ActivityCronImplementation;
import ru.yandex.ci.common.temporal.ydb.TemporalDb;
import ru.yandex.ci.common.temporal.ydb.TemporalLaunchQueueEntity;

@Slf4j
public class TemporalLostWorkflowActivityImpl implements TemporalLostWorkflowActivity, ActivityCronImplementation {

    private static final int MAX_ITERATION = 100;
    private static final int FETCH_LIMIT = 100;
    private static final Duration LOST_TIMEOUT = Duration.ofMinutes(5);
    private static final String METRIC_NAME = "temporal_lost_workflows";
    private static final String TAG_NAME = "counter";

    private final TemporalService temporalService;
    private final TemporalDb temporalDb;

    private final AtomicInteger lostWorkflowsLaunched = new AtomicInteger();
    private final AtomicInteger lostWorkflowsLaunchExceptions = new AtomicInteger();

    public TemporalLostWorkflowActivityImpl(TemporalService temporalService, TemporalDb temporalDb,
                                            MeterRegistry meterRegistry) {
        this.temporalService = temporalService;
        this.temporalDb = temporalDb;
        Gauge.builder(METRIC_NAME, lostWorkflowsLaunched::get)
                .tag(TAG_NAME, "launched")
                .register(meterRegistry);

        Gauge.builder(METRIC_NAME, lostWorkflowsLaunchExceptions::get)
                .tag(TAG_NAME, "launch_exceptions")
                .register(meterRegistry);
    }

    @Override
    public void launchLostWorkflows() {
        log.info("Processing lost workflows");
        for (int i = 0; i < MAX_ITERATION; i++) {
            int processed = runIteration();
            log.info("Processed {} lost workflows", processed);
            if (processed < FETCH_LIMIT) {
                log.info("No lost workflows left to process");
                return;
            }
        }
        log.info("Processing finished. Some lost still exists");
    }

    private int runIteration() {
        var lostWorkflows = temporalDb.readOnly().run(
                () -> temporalDb.temporalLaunchQueue().getLostWorkflows(LOST_TIMEOUT, FETCH_LIMIT)
        );
        log.info("Got {} lost tasks", lostWorkflows.size());
        for (TemporalLaunchQueueEntity lostWorkflow : lostWorkflows) {
            log.info("Starting lost workflow: {}", lostWorkflow.getId());
            try {
                temporalService.startLaunchQueueEntity(lostWorkflow);
                lostWorkflowsLaunched.incrementAndGet();
            } catch (Exception e) {
                log.error("Failed to start lost workflow {}. Skipping for now.", lostWorkflow.getId());
                lostWorkflowsLaunchExceptions.incrementAndGet();
            }
        }
        return lostWorkflows.size();
    }

}
