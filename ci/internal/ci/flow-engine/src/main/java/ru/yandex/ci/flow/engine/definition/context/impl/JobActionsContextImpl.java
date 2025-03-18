package ru.yandex.ci.flow.engine.definition.context.impl;

import java.time.Instant;
import java.util.List;
import java.util.concurrent.TimeUnit;
import java.util.function.BiConsumer;

import ru.yandex.ci.flow.Repeater;
import ru.yandex.ci.flow.engine.definition.context.JobActionsContext;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.runtime.exceptions.JobManualFailException;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.utils.InstantUtils;

public class JobActionsContextImpl implements JobActionsContext {
    private final JobContextImpl jobContext;

    public JobActionsContextImpl(JobContextImpl jobContext) {
        this.jobContext = jobContext;
    }

    @Override
    public void delayJobStart(long delayMillis, BiConsumer<JobContext, Long> remainingDelayCallback)
            throws InterruptedException {
        JobLaunch lastLaunch = jobContext.getJobState().getLastLaunch();

        Instant startTimestamp = lastLaunch == null ?
                Instant.now() : lastLaunch.getLastStatusChange().getDate();

        long remainingDelayMillis = InstantUtils.calculateRemainingTimeMillis(
                startTimestamp, Instant.now(), delayMillis
        );

        if (remainingDelayMillis == 0) {
            return;
        }

        Repeater.repeat(
                        () -> remainingDelayCallback.accept(
                                jobContext, InstantUtils.calculateRemainingTimeMillis(startTimestamp, Instant.now(),
                                        delayMillis)
                        )
                )
                .timeout(remainingDelayMillis, TimeUnit.MILLISECONDS)
                .run();
    }

    @Override
    public void failJob(String reason, List<SupportType> supportInfo) {
        throw new JobManualFailException(reason, supportInfo);
    }

    @Override
    public void failJob(String reason, List<SupportType> supportInfo, Throwable cause) {
        throw new JobManualFailException(reason, supportInfo, cause);
    }
}
