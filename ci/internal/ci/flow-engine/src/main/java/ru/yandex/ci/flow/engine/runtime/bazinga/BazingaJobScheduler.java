package ru.yandex.ci.flow.engine.runtime.bazinga;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.joda.time.Duration;
import org.joda.time.Instant;

import ru.yandex.bolts.collection.Option;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.FullJobLaunchId;
import ru.yandex.ci.flow.engine.runtime.JobScheduler;
import ru.yandex.commune.bazinga.BazingaTaskManager;
import ru.yandex.commune.bazinga.impl.JobId;
import ru.yandex.misc.random.Random2;

@Slf4j
@RequiredArgsConstructor
public class BazingaJobScheduler implements JobScheduler {

    @Nonnull
    private final BazingaTaskManager taskManager;

    @Override
    public void scheduleFlow(String project,
                             LaunchId launchId,
                             FullJobLaunchId fullJobLaunchId) {
        var parameters = FlowTaskParameters.create(fullJobLaunchId);
        var task = new FlowTask(parameters);
        var now = Instant.now();
        var jobId = taskManager.schedule(
                task,
                task.getTaskCategory(),
                now,
                task.priority(),
                false,
                Option.empty(),
                new JobId(now, Option.of(Random2.R.nextInt())),
                null
        );

        log.info("Bazinga job scheduled, id: {}", jobId);
    }

    @Override
    public void scheduleStageRecalc(String project,
                                    FlowLaunchId flowLaunchId) {
        var parameters = new StageRecalcTaskParameters(flowLaunchId);
        var task = new StageRecalcTask(parameters);

        var now = Instant.now();
        var jobId = taskManager.schedule(
                task,
                task.getTaskCategory(),
                now.plus(Duration.standardSeconds(15)),
                task.priority(),
                false,
                Option.empty(),
                new JobId(now, Option.of(Random2.R.nextInt())),
                null
        );

        log.info("Bazinga job scheduled, id: {}", jobId);
    }
}
