package ru.yandex.ci.flow.engine.runtime.bazinga;

import java.time.Clock;
import java.time.Instant;

import javax.annotation.Nullable;

import lombok.RequiredArgsConstructor;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.Logger;

import ru.yandex.bolts.collection.Option;
import ru.yandex.ci.flow.engine.runtime.FullJobLaunchId;
import ru.yandex.commune.bazinga.BazingaTaskManager;
import ru.yandex.commune.bazinga.impl.FullJobId;
import ru.yandex.commune.bazinga.impl.JobId;
import ru.yandex.misc.random.Random2;

@RequiredArgsConstructor
public class JobWaitingSchedulerImpl implements JobWaitingScheduler {
    private static final Logger log = LogManager.getLogger();

    private final BazingaTaskManager taskManager;
    private final Clock clock;

    @Override
    public void schedule(FullJobLaunchId fullJobLaunchId, @Nullable Instant date) {
        if (date == null) {
            date = Instant.now(clock);
        }
        JobScheduleTaskParameters parameters = JobScheduleTaskParameters.create(fullJobLaunchId);
        JobScheduleTask task = new JobScheduleTask(parameters);

        var scheduledAt = org.joda.time.Instant.ofEpochMilli(date.toEpochMilli());
        FullJobId bazingaJobId = taskManager.schedule(
                task,
                task.getTaskCategory(),
                scheduledAt,
                task.priority(),
                false,
                Option.empty(),
                new JobId(scheduledAt, Option.of(Random2.R.nextInt())),
                null
        );

        log.info("Bazinga job scheduled, id: " + bazingaJobId);
    }

    @Override
    public void retry(JobScheduleTask jobScheduleTask, Instant date) {
        JobScheduleTask newTask = JobScheduleTask.retry(jobScheduleTask);
        FullJobId bazingaJobId = taskManager.schedule(
                newTask,
                newTask.getTaskCategory(),
                new org.joda.time.Instant(date.toEpochMilli()),
                newTask.priority(),
                true
        );
        var newTaskParameters = (JobScheduleTaskParameters) newTask.getParameters();
        log.info("Bazinga job scheduled, id: {} for {}", bazingaJobId, newTaskParameters);
    }
}
