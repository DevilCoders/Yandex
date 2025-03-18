package ru.yandex.ci.common.bazinga.monitoring;


import java.time.Duration;

import io.micrometer.core.instrument.MeterRegistry;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.bazinga.AbstractOnetimeTask;
import ru.yandex.commune.bazinga.scheduler.EmptyParameters;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;

@Slf4j
public class HeartbeatOneTimeTask extends AbstractOnetimeTask<EmptyParameters> {

    private MeterRegistry registry;

    public HeartbeatOneTimeTask(MeterRegistry registry) {
        super(EmptyParameters.class);
        this.registry = registry;
    }

    public HeartbeatOneTimeTask() {
        super(new EmptyParameters());
    }

    @Override
    public int priority() {
        return -10;
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofSeconds(15);
    }

    @Override
    protected void execute(EmptyParameters parameters, ExecutionContext context) {
        log.info("heart beat");
        registry.counter("bazinga_heartbeat").increment();
    }
}
