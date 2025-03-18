package ru.yandex.ci.engine.autocheck.jobs.misc;

import java.util.Random;
import java.util.UUID;
import java.util.concurrent.TimeUnit;

import ci.tasklets.misc.sleep.Sleep;
import com.google.common.base.Preconditions;
import com.google.common.base.Strings;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.time.DurationParser;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.ExecutorInfo;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Consume;

@Slf4j
@ExecutorInfo(
        title = "Sleep job",
        description = "Sleep configured amount of time +-rand(0, jitter)"
)
@Consume(name = "config", proto = Sleep.Config.class)
public class SleepJob implements JobExecutor {
    public static final UUID ID = UUID.fromString("e766d88a-ea72-43ad-b647-cd30b1ff5b2d");

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        var config = context.resources().consume(Sleep.Config.class);
        log.info("Starting sleep job with config {}", config);
        long sleepTimeMillis = getSleepTimeMillis(config);
        log.info("Sleep time {}ms", sleepTimeMillis);
        context.actions().delayJobStart(
                sleepTimeMillis,
                (jobContext, remainingMillis) -> context.progress().updateText(
                        "Remaining sleep time %ds".formatted(TimeUnit.MILLISECONDS.toSeconds(remainingMillis))
                )
        );
    }

    private long getSleepTimeMillis(Sleep.Config config) {
        if (config.getExecutionTimeMs() > 0) {
            log.info("Using deprecated ExecutionTimeMs. Jutter is not supported");
            return config.getExecutionTimeMs();
        }

        Preconditions.checkArgument(!Strings.isNullOrEmpty(config.getSleepTime()), "sleep_time is not set");
        long sleepTimeMillis = DurationParser.parse(config.getSleepTime()).toMillis();

        int jitterMillis = 0;
        if (!Strings.isNullOrEmpty(config.getJitterTime())) {
            int baseJitterMillis = (int) DurationParser.parse(config.getJitterTime()).toMillis();
            jitterMillis = new Random().nextInt(baseJitterMillis * 2) - baseJitterMillis;
            log.info("Randomized jitter is {} ms", jitterMillis);
        }

        sleepTimeMillis += jitterMillis;
        return sleepTimeMillis;
    }
}
